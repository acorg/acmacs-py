#include "acmacs-base/quicklook.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-map-draw/draw.hh"
// #include "acmacs-chart-2/common.hh"
#include "acmacs-map-draw/mapi-procrustes.hh"

#include "acmacs-py/py.hh"

namespace acmacs_py
{
    class procrustes_error : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    static std::pair<acmacs::mapi::distances_t, acmacs::chart::ProcrustesData> procrustes_arrows(ChartDraw& chart_draw, const acmacs::chart::CommonAntigensSera& common, acmacs::chart::ChartModifyP secondary_chart, size_t secondary_projection_no,
                                  bool scaling, double threshold,
                                  double line_width, double arrow_width, double arrow_outline_width, const std::string& outline, const std::string& arrow_fill, const std::string& arrow_outline);
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
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
