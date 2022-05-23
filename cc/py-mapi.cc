#include "acmacs-base/rjson-v3.hh"
#include "acmacs-base/quicklook.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-base/log.hh"
#include "acmacs-base/color-distinct.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/selected-antigens-sera.hh"
#include "acmacs-chart-2/grid-test.hh"
#include "acmacs-chart-2/serum-circle.hh"
#include "seqdb-3/compare.hh"
#include "acmacs-map-draw/draw.hh"
#include "acmacs-map-draw/mapi-procrustes.hh"
#include "acmacs-map-draw/point-style.hh"
#include "acmacs-map-draw/mapi-procrustes.hh"
#include "acmacs-map-draw/map-elements-v2.hh"
#include "acmacs-map-draw/figure.hh"
#include "acmacs-py/py.hh"

// ----------------------------------------------------------------------

namespace acmacs_py
{
    class procrustes_error : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    struct PointLegend
    {
        std::string format;
        bool show;
        bool show_if_none_selected;
        bool replace;
    };

    // ----------------------------------------------------------------------

    static inline ChartDraw chart_draw(acmacs::chart::ChartModifyP chart, size_t projection_no)
    {
        using namespace std::string_view_literals;
        ChartDraw chart_draw{chart, projection_no};
        chart_draw.settings(acmacs::mapi::Settings::env_put_antigen_serum_names::no).load_from_conf({"mapi.json"sv, "clades.json"sv, "vaccines.json"sv});
        return chart_draw;
    }

    // ----------------------------------------------------------------------

    static inline std::pair<acmacs::mapi::distances_t, acmacs::chart::ProcrustesData> procrustes_arrows(ChartDraw& chart_draw, const acmacs::chart::CommonAntigensSera& common,
                                                                                                        acmacs::chart::ChartModifyP secondary_chart, size_t secondary_projection_no, bool scaling,
                                                                                                        double threshold, double line_width, double arrow_width, double arrow_outline_width,
                                                                                                        const std::string& outline, const std::string& arrow_fill, const std::string& arrow_outline)
    {
        if (!secondary_chart)
            secondary_chart = chart_draw.chart_ptr();
        if (secondary_chart && secondary_projection_no >= secondary_chart->number_of_projections())
            throw procrustes_error{fmt::format("invalid secondary chart projection number {} (chart has just {} projection(s))", secondary_projection_no, secondary_chart->number_of_projections())};

        const acmacs::mapi::ArrowPlotSpec arrow_plot_spec{
            .threshold = threshold,
            .line_width = Pixels{line_width},
            .arrow_width = Pixels{arrow_width},
            .arrow_outline_width = Pixels{arrow_outline_width},
            .outline = acmacs::color::Modifier{outline},
            .arrow_fill = acmacs::color::Modifier{arrow_fill},
            .arrow_outline = acmacs::color::Modifier{arrow_outline},
        };

        const auto pc_data = procrustes_arrows(chart_draw, *secondary_chart->projection(secondary_projection_no), common,
                                               scaling ? acmacs::chart::procrustes_scaling_t::yes : acmacs::chart::procrustes_scaling_t::no, arrow_plot_spec);

        auto& titl = chart_draw.map_elements().find_or_add<map_elements::v1::Title>("title");
        if (titl.number_of_lines() == 0)
            titl.add_line(chart_draw.chart().make_name(chart_draw.chart(0).projection_no()));
        titl.add_line(secondary_chart->make_name(secondary_projection_no));
        titl.add_line(fmt::format("RMS: {:.6f}", pc_data.second.rms));
        return pc_data;
    }

    // ----------------------------------------------------------------------

    static inline void modify_style(acmacs::mapi::point_style_t& style, const std::string& fill, const std::string& outline, double outline_width, bool show, const std::string& shape, double size,
                                    double aspect, double rotation)
    {
        style.fill(acmacs::mapi::make_modifier_or_passage(fill));
        style.outline(acmacs::mapi::make_modifier_or_passage(outline));
        style.style.shown(show);
        if (!shape.empty())
            style.style.shape(acmacs::PointShape{shape});
        if (size >= 0.0)
            style.style.size(Pixels{size});
        if (outline_width >= 0.0)
            style.style.outline_width(Pixels{outline_width});
        if (aspect > 0.0)
            style.style.aspect(Aspect{aspect});
        if (rotation > 1e-5)
            style.style.rotation(RotationRadiansOrDegrees(rotation));
    }

    // ----------------------------------------------------------------------

    template <typename Selected> static void modify_label(ChartDraw& chart_draw, std::shared_ptr<Selected> selected, std::shared_ptr<acmacs::draw::PointLabel> label)
    {
        if (label) {
            size_t base_index{0};
            if constexpr (std::is_same_v<Selected, acmacs::chart::SelectedSera> || std::is_same_v<Selected, acmacs::chart::SelectedSeraModify>)
                base_index = chart_draw.chart().number_of_antigens();
            for (auto index : selected->indexes) {
                auto& plabel = chart_draw.add_label(index + base_index);
                plabel = *label;
                plabel.display_name(acmacs::chart::format_antigen_serum<typename Selected::AntigensSeraType>(plabel.display_name(), chart_draw.chart(), index, acmacs::chart::collapse_spaces_t::yes));
            }
        }
    }

    // ----------------------------------------------------------------------

    template <typename Selected> static void modify_legend(ChartDraw& chart_draw, std::shared_ptr<Selected> selected, const acmacs::mapi::point_style_t& style, std::shared_ptr<PointLegend> legend)
    {
        if (legend) {
            auto& legend_element = chart_draw.map_elements().find_or_add<map_elements::v1::LegendPointLabel>("legend-point-label");

            const auto label = fmt::substitute(legend->format, std::make_pair("count", selected ? selected->size() : 0ul));
            if (legend->replace)
                legend_element.remove_line(label);
            if (legend->show && (!selected || !selected->empty() || legend->show_if_none_selected)) {
                legend_element.add_line(style.style.shape(), acmacs::color::Modifier{style.style.fill()}, acmacs::color::Modifier{style.style.outline()}, style.style.outline_width(), label);
            }
        }
    }

    // ----------------------------------------------------------------------

    template <typename Selected>
    static void modify_antigens_sera(ChartDraw& chart_draw, std::shared_ptr<Selected> selected, const std::string& fill, const std::string& outline, double outline_width, bool show,
                                     const std::string& shape, double size, double aspect, double rotation, const std::string& order, std::shared_ptr<acmacs::draw::PointLabel> label,
                                     std::shared_ptr<PointLegend> legend)
    {
        if (!selected)
            selected = std::make_shared<Selected>(chart_draw.chart(0).modified_chart_ptr());
        acmacs::mapi::point_style_t style;
        modify_style(style, fill, outline, outline_width, show, shape, size, aspect, rotation);
        chart_draw.modify(*selected, style.style, drawing_order_from(order));
        modify_label(chart_draw, selected, label);
        if constexpr (std::is_same_v<Selected, acmacs::chart::SelectedSeraModify>) {
            if (shape.empty())
                style.style.shape(acmacs::PointShape::Box);
        }
        modify_legend(chart_draw, selected, style, legend);
    }

    static void modify_antigens_and_sera(ChartDraw& chart_draw, std::shared_ptr<acmacs::chart::SelectedAntigensModify> antigens, std::shared_ptr<acmacs::chart::SelectedSeraModify> sera,
                                         const std::string& fill, const std::string& outline, double outline_width, bool show, const std::string& shape, double size, double aspect, double rotation,
                                         const std::string& order, std::shared_ptr<acmacs::draw::PointLabel> label, std::shared_ptr<PointLegend> legend)
    {
        modify_antigens_sera(chart_draw, antigens, fill, outline, outline_width, show, shape, size, aspect, rotation, order, label, legend);
        modify_antigens_sera(chart_draw, sera, fill, outline, outline_width, show, shape, size, aspect, rotation, order, label, legend);
    }

    // ----------------------------------------------------------------------

    static void legend_append(ChartDraw& chart_draw, const std::string& fill, const std::string& outline, double outline_width, bool show, const std::string& shape, double size, double aspect,
                              double rotation, std::shared_ptr<PointLegend> legend)
    {
        acmacs::mapi::point_style_t style;
        modify_style(style, fill, outline, outline_width, show, shape, size, aspect, rotation);
        modify_legend(chart_draw, std::shared_ptr<acmacs::chart::SelectedAntigensModify>{}, style, legend);
    }

    // ----------------------------------------------------------------------

    template <typename Selected> static void move_antigens_sera(ChartDraw& chart_draw, const Selected& selected, const std::vector<double>& to, const acmacs::mapi::Figure& flip_over_line)
    {
        auto& projection = chart_draw.chart(0).modified_projection();
        if (to.size() == 2) {
            const acmacs::PointCoordinates move_to = map_elements::v2::Coordinates::viewport{acmacs::PointCoordinates{to[0], to[1]}}.get_not_transformed(chart_draw);
            for (const auto point_no : selected.points())
                projection.move_point(point_no, move_to);
        }
        if (!flip_over_line.empty()) {
            if (flip_over_line.vertices.size() != 2)
                throw std::invalid_argument{AD_FORMAT("unrecognized flip_over_line: \"{}\", list of two points expected", flip_over_line.vertices)};
            const acmacs::LineDefinedByEquation line(map_elements::v2::Coordinates::not_transformed{flip_over_line.vertices[0]}.get_not_transformed(chart_draw), map_elements::v2::Coordinates::not_transformed{flip_over_line.vertices[1]}.get_not_transformed(chart_draw));
            auto layout = projection.layout();
            for (const auto point_no : selected.points())
                projection.move_point(point_no, line.flip_over(layout->at(point_no), 1.0));
        }
    }

    // ----------------------------------------------------------------------

    static inline void compare_sequences(ChartDraw& /*chart_draw*/, const acmacs::chart::SelectedAntigensModify& set1, const acmacs::chart::SelectedAntigensModify& set2, py::object output, bool open)
    {
        acmacs::seqdb::subsets_to_compare_t<acmacs::seqdb::subset_to_compare_selected_t> to_compare{acmacs::seqdb::compare::aa};
        to_compare.subsets.emplace_back("1", set1);
        to_compare.subsets.emplace_back("2", set2);
        to_compare.make_counters();
        const std::string filename{py::str(output)};
        acmacs::seqdb::compare_sequences_generate_html(filename, to_compare);
        acmacs::open_or_quicklook(open && filename != "-" && filename != "=", false, filename);
    }

    // ----------------------------------------------------------------------

    static inline std::shared_ptr<acmacs::draw::PointLabel> point_label(bool show, const std::string& format, const std::vector<double>& offset, const std::string& color, double size,
                                                                        const std::string& weight, const std::string& slant, const std::string& font)
    {
        auto pl = std::make_shared<acmacs::draw::PointLabel>();
        pl->show(show);
        if (!format.empty())
            pl->display_name(format);
        if (!offset.empty()) {
            if (offset.size() == 2)
                pl->offset(acmacs::PointCoordinates(std::begin(offset), std::end(offset)));
            else
                AD_WARNING("invalid offset ({}), list of two floats expected", offset);
        }
        if (!color.empty())
            pl->color(acmacs::color::Modifier{color});
        if (size >= 0.0)
            pl->size(Pixels{size});
        if (!weight.empty())
            pl->weight(weight);
        if (!slant.empty())
            pl->slant(slant);
        if (!font.empty())
            pl->font_family(font);
        return pl;
    }

    // ----------------------------------------------------------------------

    static inline void legend(ChartDraw& chart_draw, bool show, const std::string& type, const std::vector<double>& offset, double label_size, double point_size, const std::vector<std::string>& title)
    {
        if (show) {
            if (type == "continent-map" || type == "continent_map") {
                switch (offset.size()) {
                    case 2:
                        chart_draw.continent_map(acmacs::PointCoordinates{offset[0], offset[1]}, /*size*/ Pixels{100.0});
                        break;
                    case 0:
                        chart_draw.continent_map();
                        break;
                    default:
                        AD_WARNING("invalid legend offset: {}", offset);
                        break;
                }
            }
            else {
                auto& legend_element = chart_draw.map_elements().find_or_add<map_elements::v1::LegendPointLabel>("legend-point-label");
                switch (offset.size()) {
                    case 2:
                        legend_element.offset(acmacs::PointCoordinates{offset[0], offset[1]});
                        break;
                    case 0:
                        break;
                    default:
                        AD_WARNING("invalid legend offset: {}", offset);
                        break;
                }
                if (label_size >= 0.0)
                    legend_element.label_size(Pixels{label_size});
                if (point_size >= 0.0)
                    legend_element.point_size(Pixels{point_size});
                auto insertion_point{legend_element.lines().begin()};
                for (const auto& title_line : title) {
                    insertion_point = std::next(legend_element.lines().emplace(insertion_point, title_line));
                }
            }
        }
        else
            chart_draw.remove_legend();
    }

    // ----------------------------------------------------------------------

    static inline void connection_lines(ChartDraw& chart_draw, std::shared_ptr<acmacs::chart::SelectedAntigensModify> antigens, std::shared_ptr<acmacs::chart::SelectedSeraModify> sera,
                                        const std::string& color, double line_width, bool report)
    {
        acmacs::mapi::ConnectionLinePlotSpec plot_spec;
        plot_spec.color.add(acmacs::color::Modifier{color});
        plot_spec.line_width = Pixels{line_width};
        acmacs::mapi::connection_lines(chart_draw, *antigens, *sera, plot_spec, report);
    }

    // ----------------------------------------------------------------------

    static inline void error_lines(ChartDraw& chart_draw, std::shared_ptr<acmacs::chart::SelectedAntigensModify> antigens, std::shared_ptr<acmacs::chart::SelectedSeraModify> sera,
                                   const std::string& more, const std::string& less, double line_width, bool report)
    {
        acmacs::mapi::ErrorLinePlotSpec plot_spec;
        plot_spec.more.add(acmacs::color::Modifier{more});
        plot_spec.less.add(acmacs::color::Modifier{less});
        plot_spec.line_width = Pixels{line_width};
        acmacs::mapi::error_lines(chart_draw, *antigens, *sera, plot_spec, report);
    }

    // ----------------------------------------------------------------------

    static inline void title(ChartDraw& chart_draw, const std::vector<std::string>& lines, bool remove_all_lines, bool show, double text_size, std::string_view text_color, std::string_view font_weight)
    {
        auto& title_element = chart_draw.map_elements().find_or_add<map_elements::v1::Title>("title");
        title_element.show(show).text_size(Pixels{text_size}).weight(font_weight).text_color(acmacs::color::Modifier(text_color));
        if (remove_all_lines)
            title_element.remove_all_lines();
        for (const auto& line : lines)
            title_element.add_line(line);
    }

    // ----------------------------------------------------------------------

    static inline map_elements::v2::Path& path_impl(ChartDraw& chart_draw, const acmacs::mapi::Figure& figure, double outline_width, std::string_view outline, std::string_view fill)
    {
        auto& path = chart_draw.map_elements().add<map_elements::v2::Path>();
        path.data() = figure;
        path.outline_width(Pixels{outline_width});
        path.outline(acmacs::color::Modifier{outline});
        path.fill(acmacs::color::Modifier{fill});
        return path;
    }

    // ----------------------------------------------------------------------

    static inline void path(ChartDraw& chart_draw, const acmacs::mapi::Figure& figure, double outline_width, std::string_view outline, std::string_view fill)
    {
        path_impl(chart_draw, figure, outline_width, outline, fill);
    }

    static inline void arrow(ChartDraw& chart_draw, const acmacs::mapi::Figure& figure, double outline_width, std::string_view outline, std::string_view fill)
    {
        auto& path_data = path_impl(chart_draw, figure, outline_width, outline, fill);
        path_data.arrows().emplace_back(1);
    }

    // ----------------------------------------------------------------------

    static inline void relax(ChartDraw& chart_draw, bool reorient)
    {
        auto& projection = chart_draw.chart(0).modified_projection();
        const auto status = projection.relax(acmacs::chart::optimization_options{});
        if (reorient) {
            acmacs::chart::CommonAntigensSera common(chart_draw.chart(0).chart());
            auto master_projection = (*chart_draw.chart(0).chart().projections())[chart_draw.chart(0).projection_no()];
            const auto procrustes_data = acmacs::chart::procrustes(*master_projection, projection, common.points(), acmacs::chart::procrustes_scaling_t::no);
            projection.transformation(procrustes_data.transformation);
        }
    }

    // ----------------------------------------------------------------------

    static inline void hemisphering_arrows(ChartDraw& chart_draw, const acmacs::chart::GridTest::Results& results, const std::string& hemi_color, const std::string& trapped_color)
    {
        acmacs::mapi::HemisphringArrowsPlotSpec plot_spec;
        plot_spec.hemisphering.add(acmacs::color::Modifier{hemi_color});
        plot_spec.trapped.add(acmacs::color::Modifier{trapped_color});
        acmacs::mapi::hemisphering_arrows(chart_draw, results, plot_spec);
    }

    // ----------------------------------------------------------------------

    template <typename CoordBuilder> static inline void build_figure_raw(acmacs::mapi::FigureRaw& figure_raw, const std::vector<std::pair<double, double>>& points)
    {
        for (const auto& [x, y] : points)
            figure_raw.vertices.push_back(CoordBuilder{acmacs::PointCoordinates{x, y}});
    }

    template <typename CoordBuilder> static inline void build_figure_raw(acmacs::mapi::FigureRaw& figure_raw, const std::pair<double, double>& v1, const std::pair<double, double>& v2)
    {
        figure_raw.vertices.push_back(CoordBuilder{acmacs::PointCoordinates{v1}});
        figure_raw.vertices.push_back(CoordBuilder{acmacs::PointCoordinates{v2.first, v1.second}});
        figure_raw.vertices.push_back(CoordBuilder{acmacs::PointCoordinates{v2}});
        figure_raw.vertices.push_back(CoordBuilder{acmacs::PointCoordinates{v1.first, v2.second}});
    }

    // ----------------------------------------------------------------------

    static inline void report_circles(size_t serum_index, const acmacs::chart::Antigens& antigens, const acmacs::chart::PointIndexList& antigen_indexes, const acmacs::chart::SerumCircle& empirical,
                                      const acmacs::chart::SerumCircle& theoretical, std::string_view forced_homologous_titer)
    {
        const auto find_data = [](const acmacs::chart::SerumCircle& data, size_t antigen_index) -> const acmacs::chart::detail::SerumCirclePerAntigen& {
            if (const auto found = find_if(std::begin(data.per_antigen()), std::end(data.per_antigen()), [antigen_index](const auto& en) { return en.antigen_no == antigen_index; });
                found != std::end(data.per_antigen())) {
                return *found;
            }
            else {
                AD_ERROR("per_antigen: {}  looking for antigen {}", data.per_antigen().size(), antigen_index);
                for (const auto& en : data.per_antigen())
                    AD_ERROR("AG {} titer:{}", en.antigen_no, en.titer);
                throw std::runtime_error{"internal error in report_circles..find_data"};
            }
        };

        fmt::print("     empir   theor   titer\n");
        if (!forced_homologous_titer.empty()) {
            const auto& empirical_data = empirical.per_antigen().front();
            const auto& theoretical_data = theoretical.per_antigen().front();
            std::string empirical_radius(6, ' '), theoretical_radius(6, ' ');
            if (empirical_data.valid())
                empirical_radius = fmt::format("{:.4f}", *empirical_data.radius);
            if (theoretical_data.valid())
                theoretical_radius = fmt::format("{:.4f}", *theoretical_data.radius);
            fmt::print("    {}  {}  {:>6s} (titer forced)\n", empirical_radius, theoretical_radius, fmt::format("{}", theoretical_data.titer));
        }
        else {
            for (const auto antigen_index : antigen_indexes) {
                const auto& empirical_data = find_data(empirical, antigen_index);
                const auto& theoretical_data = find_data(theoretical, antigen_index);
                std::string empirical_radius(6, ' '), theoretical_radius(6, ' '), empirical_report, theoretical_report;
                if (empirical_data.valid())
                    empirical_radius = fmt::format("{:.4f}", *empirical_data.radius);
                else
                    empirical_report.assign(empirical_data.report_reason());
                if (theoretical_data.valid())
                    theoretical_radius = fmt::format("{:.4f}", *theoretical_data.radius);
                else
                    theoretical_report.assign(theoretical_data.report_reason());
                fmt::print("    {}  {}  {:>6s}   AG {:4d} {:40s}", empirical_radius, theoretical_radius, fmt::format("{}", theoretical_data.titer), antigen_index,
                                  antigens[antigen_index]->name_full(), empirical_report);
                if (!empirical_report.empty())
                    fmt::print(" -- {}", empirical_report);
                else if (!theoretical_report.empty())
                    fmt::print(" -- {}", theoretical_report);
                fmt::print("\n");
            }
        }
        std::string empirical_radius(6, ' '), theoretical_radius(6, ' ');
        if (empirical.valid()) {
            empirical_radius = fmt::format("{:.4f}", empirical.radius());
            // if (hide_serum_circle(hide_if, serum_index, empirical.radius()))
            //     empirical_radius += " (hidden)";
        }
        if (theoretical.valid()) {
            theoretical_radius = fmt::format("{:.4f}", theoretical.radius());
            // if (hide_serum_circle(hide_if, serum_index, theoretical.radius()))
            //     theoretical_radius += " (hidden)";
        }
        fmt::print("  > {}  {}\n", empirical_radius, theoretical_radius);
    }

    static inline void make_circle(ChartDraw& chart_draw, size_t serum_no, Scaled radius, Color outline, Pixels outline_width, size_t dash = 0)
    {
        auto& circle = chart_draw.serum_circle(serum_no, radius);
        switch (dash) {
            case 1:
                circle.outline_dash1();
                break;
            case 2:
                circle.outline_dash2();
                break;
            case 3:
                circle.outline_dash3();
                break;
            default:
                circle.outline_no_dash();
                break;
        }
        circle.outline(outline, outline_width);
        circle.fill(TRANSPARENT);
    }

    static inline void serum_circles(ChartDraw& chart_draw, const acmacs::chart::SelectedSeraModify& sera, const acmacs::chart::SelectedAntigensModify* antigens, std::string_view outline_color,
                                     double outline_width, bool show_empirical, bool show_theoretical, bool show_fallback, double fallback_radius, double fold,
                                     std::string_view forced_homologous_titer)
    {
        const auto verb = acmacs::verbose_from(false);
        Color outline{outline_color};

        const auto& chart = chart_draw.chart(0).modified_chart();
        auto titers = chart.titers();
        auto chart_antigens = chart.antigens();
        auto layout = chart_draw.chart(0).modified_layout();

        if (!antigens || antigens->empty())
            chart.set_homologous(acmacs::chart::find_homologous::all);

        for (size_t no = 0; no < sera.size(); ++no) {
            const auto [sr_no, serum] = sera[no];
            const auto antigen_indexes = (antigens && !antigens->empty()) ? antigens->points() : serum->homologous_antigens();
            if (!layout->point_has_coordinates(sr_no + chart_antigens->size())) {
                AD_WARNING("SR {:3d} disconnected", sr_no);
            }
            else if (!antigen_indexes.empty() || !forced_homologous_titer.empty()) {
                const auto column_basis = chart.column_basis(sr_no, chart_draw.chart(0).projection_no());
                acmacs::chart::SerumCircle empirical, theoretical;
                if (!forced_homologous_titer.empty()) {
                    // empirical = acmacs::chart::serum_circle_empirical(antigen_indexes, forced_homologous_titer, sr_no, *layout, column_basis, *titers, fold, verb);
                    // theoretical = acmacs::chart::serum_circle_theoretical(antigen_indexes, forced_homologous_titer, sr_no, column_basis, fold);
                }
                else {
                    empirical = acmacs::chart::serum_circle_empirical(antigen_indexes, sr_no, *layout, column_basis, *titers, fold, verb);
                    theoretical = acmacs::chart::serum_circle_theoretical(antigen_indexes, sr_no, column_basis, *titers, fold);
                }
                report_circles(sr_no, *chart_antigens, antigen_indexes, empirical, theoretical, forced_homologous_titer);

                // std::optional<size_t> mark_antigen;
                // // AD_DEBUG("SERUM {} {}", sr_no, serum->name_full());
                if (show_empirical && empirical.valid()) {
                    make_circle(chart_draw, sr_no, Scaled{empirical.radius()}, outline, Pixels{outline_width});
                    // if (theoretical.per_antigen().front().antigen_no != static_cast<size_t>(-1))
                    //     mark_antigen = empirical.per_antigen().front().antigen_no;
                }
                if (show_theoretical && theoretical.valid()) {
                    make_circle(chart_draw, sr_no, Scaled{theoretical.radius()}, outline, Pixels{outline_width}, 1);
                    // if (!mark_antigen.has_value() && theoretical.per_antigen().front().antigen_no != static_cast<size_t>(-1))
                    //     mark_antigen = theoretical.per_antigen().front().antigen_no;
                }
                if (!empirical.valid() && !theoretical.valid() && show_fallback) {
                    make_circle(chart_draw, sr_no, Scaled{fallback_radius}, outline, Pixels{outline_width}, 2);
                }

                // // mark antigen
                // if (const auto& antigen_style = getenv("mark_antigen"sv); mark_antigen.has_value() && !antigen_style.is_null()) {
                //     const auto style = style_from(antigen_style);
                //     chart_draw().modify(*mark_antigen, style.style, drawing_order_from(antigen_style));
                //     const acmacs::chart::PointIndexList indexes{*mark_antigen};
                //     color_according_to_passage(*chart_antigens, indexes, style);
                //     if (const auto& label = substitute(antigen_style["label"sv]); !label.is_null())
                //         add_labels(indexes, 0, label);
                // }
            }
            else {
                AD_WARNING("SR {:3d}: no homologous antigens", sr_no);
                if (show_fallback)
                    make_circle(chart_draw, sr_no, Scaled{fallback_radius}, outline, Pixels{outline_width}, 3);
            }
        }
    }

} // namespace acmacs_py

// ----------------------------------------------------------------------

void acmacs_py::mapi(py::module_& mdl)
{
    using namespace pybind11::literals;

    py::class_<acmacs::mapi::Figure>(mdl, "Figure") //
                                                    // use chart_draw.figure([], close) to construct
        ;

    py::class_<ChartDraw>(mdl, "ChartDraw")                                                         //
        .def(py::init(&chart_draw), "chart"_a, "projection_no"_a = 0)                               //
        .def("chart", &ChartDraw::chart_ptr, py::doc("for exporting with plot spec modifications")) //
        .def(
            "projection", [](ChartDraw& chart_draw) -> acmacs::chart::ProjectionModify& { return chart_draw.chart(0).modified_projection(); }, py::return_value_policy::reference,
            py::doc("to relax"))                                   //
        .def("calculate_viewport", &ChartDraw::calculate_viewport) //
        .def(
            "viewport",
            [](ChartDraw& chart_draw, double x, double y, double size) {
                chart_draw.set_viewport({x, y}, size);
            },
            "x"_a, "y"_a, "size"_a)                                                                                                   //
        .def("transformation", [](const ChartDraw& chart_draw) { return chart_draw.chart(0).modified_transformation().as_vector(); }) //
        .def(
            "rotate",
            [](ChartDraw& chart_draw, double angle) {
                if (std::abs(angle) < 4.0)
                    chart_draw.rotate(angle);
                else
                    chart_draw.rotate(angle * std::acos(-1) / 180.0);
            },
            "angle"_a, py::doc("abs(angle) < 4: radians, else degrees, positive: counter-clockwise")) //
        .def(
            "flip",
            [](ChartDraw& chart_draw, std::string_view direction) {
                if (direction == "ew")
                    chart_draw.flip(0, 1);
                else if (direction == "ns")
                    chart_draw.flip(1, 0);
                else
                    throw std::invalid_argument{AD_FORMAT("unrecognized direction: \"{}\", either \"ew\" or \"ns\" expected", direction)};
            },
            "direction"_a = "ew", py::doc("direction: ew or ns"))                                      //
        .def("scale_points", &ChartDraw::scale_points, "point_scale"_a = 1.0, "outline_scale"_a = 1.0) //
        .def(
            "draw",
            [](const ChartDraw& chart_draw, py::object path, double size, bool open) {
                const std::string filename = py::str(path);
                chart_draw.draw(filename, size);
                if (open)
                    acmacs::open(filename);
            },
            "filename"_a, "size"_a = 800.0, "open"_a = true)                                                                                                                          //
        .def("legend", &legend, "show"_a = true, "type"_a = "", "offset"_a = std::vector<double>{}, "label_size"_a = -1, "point_size"_a = -1, "title"_a = std::vector<std::string>{}) //
        .def("legend_append", &legend_append, "fill"_a = "", "outline"_a = "", "outline_width"_a = -1.0, "show"_a = true, "shape"_a = "", "size"_a = -1.0, "aspect"_a = -1.0, "rotation"_a = -1e10,
             "legend"_a, py::doc("Appends a line to the legend."))                                                                                                                               //
        .def("connection_lines", &connection_lines, "antigens"_a, "sera"_a, "color"_a = "grey", "line_width"_a = 0.5, "report"_a = false)                                                        //
        .def("error_lines", &error_lines, "antigens"_a, "sera"_a, "more"_a = "red", "less"_a = "blue", "line_width"_a = 0.5, "report"_a = false)                                                 //
        .def("title", &title, "lines"_a = std::vector<std::string>{}, "remove_all_lines"_a = false, "show"_a = true, "text_size"_a = 12.0, "text_color"_a = "black", "font_weight"_a = "normal", //
             py::doc("subtitutions: {name} {virus} {virus-type} {lineage} {lineage-cap} {subset} {subset-up} {virus-type/lineage} {virus-type/lineage-subset} {virus-type-lineage-subset-short-low} "
                     "{assay-full} {assay-cap} {assay-low} {assay-no-hi-low} {assay-no-hi-cap} {lab} {lab-low} {rbc} {assay-rbc} {assay-low} {table-date} {num-ag} {num-sr} {num-layers} "
                     "{minimum-column-basis} {mcb} {stress}")) //
        .def(
            "figure",
            [](const ChartDraw& chart_draw, const std::vector<std::pair<double, double>>& points, bool close, std::string_view coordinates_relative_to) {
                acmacs::mapi::FigureRaw figure_raw{close};
                if (coordinates_relative_to == "viewport-origin")
                    acmacs_py::build_figure_raw<map_elements::v2::Coordinates::viewport>(figure_raw, points);
                else if (coordinates_relative_to == "map-not-tranformed")
                    acmacs_py::build_figure_raw<map_elements::v2::Coordinates::not_transformed>(figure_raw, points);
                else if (coordinates_relative_to == "map-tranformed")
                    acmacs_py::build_figure_raw<map_elements::v2::Coordinates::transformed>(figure_raw, points);
                else
                    throw std::invalid_argument{AD_FORMAT("unrecognized coordinates_relative_to: \"{}\"", coordinates_relative_to)};
                return acmacs::mapi::Figure{figure_raw, chart_draw};
            },
            "vertices"_a, "close"_a = true, "coordinates_relative_to"_a = "viewport-origin",                     //
            py::doc("coordinates_relative_to: \"viewport-origin\", \"map-not-tranformed\", \"map-tranformed\"")) //
        .def(
            "rectangle",
            [](const ChartDraw& chart_draw, const std::pair<double, double>& v1, const std::pair<double, double>& v2, std::string_view coordinates_relative_to) {
                acmacs::mapi::FigureRaw figure_raw{true};
                if (coordinates_relative_to == "viewport-origin")
                    acmacs_py::build_figure_raw<map_elements::v2::Coordinates::viewport>(figure_raw, v1, v2);
                else if (coordinates_relative_to == "map-not-tranformed")
                    acmacs_py::build_figure_raw<map_elements::v2::Coordinates::not_transformed>(figure_raw, v1, v2);
                else if (coordinates_relative_to == "map-tranformed")
                    acmacs_py::build_figure_raw<map_elements::v2::Coordinates::transformed>(figure_raw, v1, v2);
                else
                    throw std::invalid_argument{AD_FORMAT("unrecognized coordinates_relative_to: \"{}\"", coordinates_relative_to)};
                return acmacs::mapi::Figure{figure_raw, chart_draw};
            },
            "vertice1"_a, "vertice2"_a, "coordinates_relative_to"_a = "viewport-origin",                           //
            py::doc("coordinates_relative_to: \"viewport-origin\", \"map-not-tranformed\", \"map-tranformed\""))   //
        .def("path", &path, "figure"_a, "outline_width"_a = 1.0, "outline"_a = "pink", "fill"_a = "transparent")   //
        .def("arrow", &arrow, "figure"_a, "outline_width"_a = 1.0, "outline"_a = "pink", "fill"_a = "transparent") //
        .def(
            "circle_for_serum",
            [](ChartDraw& chart_draw, size_t serum_no, double radius, std::string_view outline, double outline_width, size_t dash) {
                make_circle(chart_draw, serum_no, Scaled{radius}, Color{outline}, Pixels{outline_width}, dash);
            },
            "serum_no"_a, "radius"_a, "outline"_a = "blue", "outline_width"_a = 1.0, "dash"_a = 0) //
        .def("remove_paths_circles", &ChartDraw::remove_paths_circles)                             //

        .def("modify", &modify_antigens_sera<acmacs::chart::SelectedAntigensModify>, //
             "select"_a, "fill"_a = "", "outline"_a = "", "outline_width"_a = -1.0, "show"_a = true, "shape"_a = "", "size"_a = -1.0, "aspect"_a = -1.0, "rotation"_a = -1e10, "order"_a = "",
             "label"_a = nullptr, "legend"_a = nullptr)                          //
        .def("modify", &modify_antigens_sera<acmacs::chart::SelectedSeraModify>, //
             "select"_a, "fill"_a = "", "outline"_a = "", "outline_width"_a = -1.0, "show"_a = true, "shape"_a = "", "size"_a = -1.0, "aspect"_a = -1.0, "rotation"_a = -1e10, "order"_a = "",
             "label"_a = nullptr, "legend"_a = nullptr) //
        .def("modify", &modify_antigens_and_sera,       //
             "antigens"_a, "sera"_a, "fill"_a = "", "outline"_a = "", "outline_width"_a = -1.0, "show"_a = true, "shape"_a = "", "size"_a = -1.0, "aspect"_a = -1.0, "rotation"_a = -1e10,
             "order"_a = "", "label"_a = nullptr, "legend"_a = nullptr)                               //
        .def("move", &move_antigens_sera<acmacs::chart::SelectedAntigensModify>,                      //
             "select"_a, "to"_a = std::vector<double>{}, "flip_over_line"_a = acmacs::mapi::Figure{}) //
        .def("move", &move_antigens_sera<acmacs::chart::SelectedSeraModify>,                          //
             "select"_a, "to"_a = std::vector<double>{}, "flip_over_line"_a = acmacs::mapi::Figure{}) //

        .def("procrustes_arrows", &procrustes_arrows, //
             "common"_a, "secondary_chart"_a = acmacs::chart::ChartModifyP{}, "secondary_projection_no"_a = 0, "scaling"_a = false, "threshold"_a = 0.005, "line_width"_a = 1.0, "arrow_width"_a = 5.0,
             "arrow_outline_width"_a = 1.0, "outline"_a = "black", "arrow_fill"_a = "black", "arrow_outline"_a = "black",     //
             py::doc("Adds procrustes arrows to the map, returns tuple (arrow_sizes, acmacs.ProcrustesData)\n"                //
                     "arrow_sizes is a list of tuples: (point_no in the primary chart, arrow size)\n"                         //
                     "if secondary_chart is None (default) - procrustes between projections of the primary chart is drawn.")) //
        .def("remove_procrustes_arrows", [](ChartDraw& chart_draw) { chart_draw.map_elements().remove("procrustes-arrow"); }) //
        .def("hemisphering_arrows", &hemisphering_arrows,                                                                     //
             "results"_a, "hemi_color"_a = "#00D0ff", "trapped_color"_a = "#ffD000")                                          //
        .def("relax", &relax, "reorient"_a = true)                                                                            //
        .def("compare_sequences", &compare_sequences, "set1"_a, "set2"_a, "output"_a, "open"_a = true)                        //
        .def("serum_circles", &serum_circles,                                                                                 //
             "sera"_a, "antigens"_a = nullptr, "outline"_a = "blue", "outline_width"_a = 1.0, "empirical"_a = true, "theoretical"_a = true, "fallback"_a = true, "fallback_radius"_a = 3.0,
             "fold"_a = 2.0, "forced_homologous_titer"_a = "") //
        ;

    py::class_<acmacs::Viewport>(mdl, "Viewport")                                                     //
        .def("__str__", [](const acmacs::Viewport& viewport) { return fmt::format("{}", viewport); }) //
        .def("origin",
             [](const acmacs::Viewport& viewport) {
                 py::list res;
                 for (acmacs::number_of_dimensions_t dim{0}; dim < viewport.origin.number_of_dimensions(); ++dim)
                     res.append(viewport.origin[dim]);
                 return res;
             })
        .def("size", [](const acmacs::Viewport& viewport) {
            py::list res{2};
            res[0] = viewport.size.width;
            res[1] = viewport.size.height;
            return res;
        });

    py::class_<acmacs::draw::PointLabel, std::shared_ptr<acmacs::draw::PointLabel>>(mdl, "PointLabel") //
        .def(py::init(&point_label),                                                                   //
             "show"_a = true, "format"_a = "", "offset"_a = std::vector<double>{}, "color"_a = "", "size"_a = -1.0, "weight"_a = "", "slant"_a = "",
             "font"_a = "") //
        ;

    py::class_<PointLegend, std::shared_ptr<PointLegend>>(mdl, "PointLegend")                     //
        .def(py::init<const std::string&, bool, bool, bool>(),                                    //
             "format"_a, "show"_a = true, "show_if_none_selected"_a = false, "replace"_a = false, //
             py::doc("format substition: {count}"))                                               //
        ;

    mdl.def("distinct_colors", &acmacs::color::distinct_s);

} // acmacs_py::mapi

// ----------------------------------------------------------------------
