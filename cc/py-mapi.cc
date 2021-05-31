#include "acmacs-base/quicklook.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/selected-antigens-sera.hh"
#include "acmacs-chart-2/grid-test.hh"
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
        chart_draw.settings().load_from_conf({"mapi.json"sv, "clades.json"sv, "vaccines.json"sv});
        return chart_draw;
    }

    // ----------------------------------------------------------------------

    static inline std::pair<acmacs::mapi::distances_t, acmacs::chart::ProcrustesData> procrustes_arrows(ChartDraw& chart_draw, const acmacs::chart::CommonAntigensSera& common,
                                                                                                        acmacs::chart::ChartModifyP secondary_chart, size_t secondary_projection_no, bool scaling,
                                                                                                        double threshold, double line_width, double arrow_width, double arrow_outline_width,
                                                                                                        const std::string& outline, const std::string& arrow_fill, const std::string& arrow_outline)
    {
        if (secondary_projection_no >= secondary_chart->number_of_projections())
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
                legend_element.add_line(acmacs::color::Modifier{style.style.fill()}, acmacs::color::Modifier{style.style.outline()}, style.style.outline_width(), label);
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

    template <typename Selected> static void move_antigens_sera(ChartDraw& chart_draw, const Selected& selected, const std::vector<double>& to)
    {
        if (to.size() == 2) {
            const acmacs::PointCoordinates move_to = map_elements::v2::Coordinates::viewport{acmacs::PointCoordinates{to[0], to[1]}}.get_not_transformed(chart_draw);
            for (const auto point_no : selected.points())
                chart_draw.chart(0).modified_projection().move_point(point_no, move_to);
        }
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

    static inline void title(ChartDraw& chart_draw, const std::vector<std::string>& lines, bool show)
    {
        auto& title_element = chart_draw.map_elements().find_or_add<map_elements::v1::Title>("title");
        title_element.show(show);
        for (const auto& line : lines)
            title_element.add_line(line);
    }

    // ----------------------------------------------------------------------

    static inline void path(ChartDraw& chart_draw, const acmacs::mapi::Figure& figure, double outline_width, const std::string& outline, const std::string& fill)
    {
        auto& path = chart_draw.map_elements().add<map_elements::v2::Path>();
        path.data() = figure;
        path.outline_width(Pixels{outline_width});
        path.outline(acmacs::color::Modifier{outline});
        path.fill(acmacs::color::Modifier{fill});
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

} // namespace acmacs_py

// ----------------------------------------------------------------------

void acmacs_py::mapi(py::module_& mdl)
{
    using namespace pybind11::literals;

    py::class_<ChartDraw>(mdl, "ChartDraw")                                                                                           //
        .def(py::init(&chart_draw), "chart"_a, "projection_no"_a = 0)                                                                 //
        .def("chart", &ChartDraw::chart_ptr, py::doc("for exporting with plot spec modifications"))                                   //
        .def("calculate_viewport", &ChartDraw::calculate_viewport)                                                                    //
        .def("viewport", &ChartDraw::viewport, "by"_a = "acmacs_py")                                                                  //
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
            "direction"_a = "ew", py::doc("direction: ew or ns")) //
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
             "legend"_a, py::doc("Appends a line to the legend."))                                                                               //
        .def("connection_lines", &connection_lines, "antigens"_a, "sera"_a, "color"_a = "grey", "line_width"_a = 0.5, "report"_a = false)        //
        .def("error_lines", &error_lines, "antigens"_a, "sera"_a, "more"_a = "red", "less"_a = "blue", "line_width"_a = 0.5, "report"_a = false) //
        .def("title", &title, "lines"_a = std::vector<std::string>{}, "show"_a = true,                                                           //
             py::doc("subtitutions: {name} {virus} {virus-type} {lineage} {lineage-cap} {subset} {subset-up} {virus-type/lineage} {virus-type/lineage-subset} {virus-type-lineage-subset-short-low} "
                     "{assay-full} {assay-cap} {assay-low} {assay-no-hi-low} {assay-no-hi-cap} {lab} {lab-low} {rbc} {assay-rbc} {assay-low} {table-date} {num-ag} {num-sr} {num-layers} "
                     "{minimum-column-basis} {mcb} {stress}")) //
        .def(
            "figure",
            [](const ChartDraw& chart_draw, const std::vector<std::pair<double, double>>& points, bool close) {
                acmacs::mapi::FigureRaw figure_raw;
                for (const auto& [x, y] : points)
                    figure_raw.vertices.push_back(map_elements::v2::Coordinates::viewport{acmacs::PointCoordinates{x, y}});
                figure_raw.close = close;
                return acmacs::mapi::Figure{figure_raw, chart_draw};
            },
            "vertices"_a, "close"_a = true)                                                                      //
        .def("path", &path, "figure"_a, "outline_width"_a = 1.0, "outline"_a = "pink", "fill"_a = "transparent") //

        .def("modify", &modify_antigens_sera<acmacs::chart::SelectedAntigensModify>, //
             "select"_a, "fill"_a = "", "outline"_a = "", "outline_width"_a = -1.0, "show"_a = true, "shape"_a = "", "size"_a = -1.0, "aspect"_a = -1.0, "rotation"_a = -1e10, "order"_a = "",
             "label"_a = nullptr, "legend"_a = nullptr)                          //
        .def("modify", &modify_antigens_sera<acmacs::chart::SelectedSeraModify>, //
             "select"_a, "fill"_a = "", "outline"_a = "", "outline_width"_a = -1.0, "show"_a = true, "shape"_a = "", "size"_a = -1.0, "aspect"_a = -1.0, "rotation"_a = -1e10, "order"_a = "",
             "label"_a = nullptr, "legend"_a = nullptr) //
        .def("modify", &modify_antigens_and_sera,       //
             "antigens"_a, "sera"_a, "fill"_a = "", "outline"_a = "", "outline_width"_a = -1.0, "show"_a = true, "shape"_a = "", "size"_a = -1.0, "aspect"_a = -1.0, "rotation"_a = -1e10,
             "order"_a = "", "label"_a = nullptr, "legend"_a = nullptr)          //
        .def("move", &move_antigens_sera<acmacs::chart::SelectedAntigensModify>, //
             "select"_a, "to"_a = std::vector<double>{})                         //
        .def("move", &move_antigens_sera<acmacs::chart::SelectedSeraModify>,     //
             "select"_a, "to"_a = std::vector<double>{})                         //

        .def("procrustes_arrows", &procrustes_arrows, //
             "common"_a, "secondary_chart"_a = acmacs::chart::ChartModifyP{}, "secondary_projection_no"_a = 0, "scaling"_a = false, "threshold"_a = 0.005, "line_width"_a = 1.0, "arrow_width"_a = 5.0,
             "arrow_outline_width"_a = 1.0, "outline"_a = "black", "arrow_fill"_a = "black", "arrow_outline"_a = "black",     //
             py::doc("Adds procrustes arrows to the map, returns tuple (arrow_sizes, acmacs.ProcrustesData)\n"                //
                     "arrow_sizes is a list of tuples: (point_no in the primary chart, arrow size)\n"                         //
                     "if secondary_chart is None (default) - procrustes between projections of the primary chart is drawn.")) //
        .def("hemisphering_arrows", &hemisphering_arrows,                                                                     //
             "results"_a, "hemi_color"_a = "#00D0ff", "trapped_color"_a = "#ffD000")                                          //
        .def("relax", &relax, "reorient"_a = true)                                                                            //
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

    py::class_<acmacs::mapi::Figure>(mdl, "Figure") //
                                                    // use chart_draw.figure([], close) to construct
        ;

} // acmacs_py::mapi

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
