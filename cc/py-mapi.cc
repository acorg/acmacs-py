#include "acmacs-base/quicklook.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-map-draw/draw.hh"
// #include "acmacs-chart-2/common.hh"

#include "acmacs-py/py.hh"

// ----------------------------------------------------------------------

void acmacs_py::mapi(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::chart;

    py::class_<ChartDraw>(mdl, "ChartDraw")                                                                               //
        .def(py::init<ChartModifyP, size_t>(), "chart"_a, "projection_no"_a = 0)                                                //
        .def("calculate_viewport", &ChartDraw::calculate_viewport)                                                        //
        .def("viewport", &ChartDraw::viewport, "by"_a = "acmacs_py")                                                      //
        .def("transformation", [](const ChartDraw& chart_draw) { return chart_draw.chart(0).modified_transformation(); }) //
        .def(
            "draw",
            [](const ChartDraw& chart_draw, const std::string& filename, double size, bool open) {
                chart_draw.draw(filename, size);
                if (open)
                    acmacs::open(filename);
            },
            "filename"_a, "size"_a = 800.0, "open"_a = true) //
        ;

} // acmacs_py::mapi

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
