#include "acmacs-base/quicklook.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-map-draw/draw.hh"
// #include "acmacs-chart-2/common.hh"

#include "acmacs-py/py.hh"

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
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
