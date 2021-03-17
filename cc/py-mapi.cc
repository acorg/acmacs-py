#include "acmacs-base/quicklook.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/selected-antigens-sera.hh"
#include "acmacs-map-draw/draw.hh"
#include "acmacs-map-draw/mapi-procrustes.hh"
#include "acmacs-map-draw/point-style.hh"
#include "acmacs-py/py.hh"

// ----------------------------------------------------------------------

namespace acmacs_py
{
    class procrustes_error : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    static std::pair<acmacs::mapi::distances_t, acmacs::chart::ProcrustesData> procrustes_arrows(ChartDraw& chart_draw, const acmacs::chart::CommonAntigensSera& common, acmacs::chart::ChartModifyP secondary_chart, size_t secondary_projection_no,
                                  bool scaling, double threshold,
                                  double line_width, double arrow_width, double arrow_outline_width, const std::string& outline, const std::string& arrow_fill, const std::string& arrow_outline);
    static void modify_antigens(ChartDraw& chart_draw, std::shared_ptr<acmacs::chart::SelectedAntigens> selected, const std::string& fill, const std::string& outline, double outline_width,
                                bool show, const std::string& shape, double size, double aspect, double rotation, const std::string& order, std::shared_ptr<acmacs::draw::PointLabel> label);
    static std::shared_ptr<acmacs::draw::PointLabel> point_label(bool show, const std::string& format, const std::vector<double>& offset, const std::string& color, double size, const std::string& weight, const std::string& slant, const std::string& font);

   // "label": <Label>,
   // "legend": {"show": true, "label": "name ({count})", "replace": false, "show_if_none_selected": false},
}

// ----------------------------------------------------------------------

void acmacs_py::mapi(py::module_& mdl)
{
    using namespace pybind11::literals;

    py::class_<ChartDraw>(mdl, "ChartDraw")                                                                                           //
        .def(py::init<acmacs::chart::ChartModifyP, size_t>(), "chart"_a, "projection_no"_a = 0)                                       //
        .def("calculate_viewport", &ChartDraw::calculate_viewport)                                                                    //
        .def("viewport", &ChartDraw::viewport, "by"_a = "acmacs_py")                                                                  //
        .def("transformation", [](const ChartDraw& chart_draw) { return chart_draw.chart(0).modified_transformation().as_vector(); }) //
        .def(
            "draw",
            [](const ChartDraw& chart_draw, const std::string& filename, double size, bool open) {
                chart_draw.draw(filename, size);
                if (open)
                    acmacs::open(filename);
            },
            "filename"_a, "size"_a = 800.0, "open"_a = true) //
        .def("procrustes_arrows", &procrustes_arrows,        //
             "common"_a, "secondary_chart"_a = acmacs::chart::ChartModifyP{}, "secondary_projection_no"_a = 0, "scaling"_a = false, "threshold"_a = 0.005, "line_width"_a = 1.0, "arrow_width"_a = 5.0,
             "arrow_outline_width"_a = 1.0, "outline"_a = "black", "arrow_fill"_a = "black", "arrow_outline"_a = "black",     //
             py::doc("Adds procrustes arrows to the map, returns tuple (arrow_sizes, acmacs.ProcrustesData)\n"                //
                     "arrow_sizes is a list of tuples: (point_no in the primary chart, arrow size)\n"                         //
                     "if secondary_chart is None (default) - procrustes between projections of the primary chart is drawn.")) //
        .def("modify", &modify_antigens,                                                                                      //
             "select"_a = nullptr, "fill"_a = "", "outline"_a = "", "outline_width"_a = -1.0, "show"_a = true, "shape"_a = "", "size"_a = -1.0, "aspect"_a = -1.0, "rotation"_a = -1e10, "order"_a = "",
             "label"_a = nullptr) //
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
        .def(py::init(&point_label),                        //
             "show"_a = true, "format"_a = "{abbreviated_name_with_passage_type}", "offset"_a = std::vector<double>{}, "color"_a = "", "size"_a = -1.0, "weight"_a = "", "slant"_a = "",
             "font"_a = "") //
        ;

} // acmacs_py::mapi

// ----------------------------------------------------------------------

std::pair<acmacs::mapi::distances_t, acmacs::chart::ProcrustesData> acmacs_py::procrustes_arrows(ChartDraw& chart_draw, const acmacs::chart::CommonAntigensSera& common, acmacs::chart::ChartModifyP secondary_chart, size_t secondary_projection_no, bool scaling,
                                  double threshold, double line_width, double arrow_width, double arrow_outline_width, const std::string& outline, const std::string& arrow_fill,
                                  const std::string& arrow_outline)
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

    return procrustes_arrows(chart_draw, *secondary_chart->projection(secondary_projection_no), common, scaling ? acmacs::chart::procrustes_scaling_t::yes : acmacs::chart::procrustes_scaling_t::no,
                      arrow_plot_spec);

} // acmacs_py::procrustes_arrows

// ----------------------------------------------------------------------

void acmacs_py::modify_antigens(ChartDraw& chart_draw, std::shared_ptr<acmacs::chart::SelectedAntigens> selected, const std::string& fill, const std::string& outline, double outline_width, bool show,
                                const std::string& shape, double size, double aspect, double rotation, const std::string& order, std::shared_ptr<acmacs::draw::PointLabel> label)
{
    if (!selected)
        selected = std::make_shared<acmacs::chart::SelectedAntigens>(chart_draw.chart(0).chart_ptr());
    acmacs::mapi::point_style_t style;
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

    chart_draw.modify(selected->indexes, style.style, drawing_order_from(order));

    // if (!color_according_to_passage(*chart_draw().chart().antigens(), indexes, style) && !color_according_to_aa_at_pos(indexes, style)) {
    //     if (const auto& legend = getenv("legend"sv); !legend.is_null())
    //         add_legend(indexes, style.style, legend);
    // }

    if (label) {
        if (label->show()) {
            for (auto index : selected->indexes) {
                auto& plabel = chart_draw.add_label(index);
                plabel = *label;
                plabel.index(index);
                plabel.display_name(acmacs::chart::format_antigen(plabel.display_name(), chart_draw.chart(), index, acmacs::chart::collapse_spaces_t::yes));
            }
        }
        else {
            for (auto index : selected->indexes)
                chart_draw.remove_label(index);
        }
    }

} // acmacs_py::modify_antigens

// ----------------------------------------------------------------------

std::shared_ptr<acmacs::draw::PointLabel> acmacs_py::point_label(bool show, const std::string& format, const std::vector<double>& offset, const std::string& color, double size,
                                                                 const std::string& weight, const std::string& slant, const std::string& font)
{
    auto pl = std::make_shared<acmacs::draw::PointLabel>();
    pl->show(show).display_name(format);
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

} // acmacs_py::point_label

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
