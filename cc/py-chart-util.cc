#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/titer-data.hh"
#include "acmacs-py/py.hh"

// ----------------------------------------------------------------------

void acmacs_py::chart_util(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::chart;

    // reference-panel-plots support
    py::class_<TiterData>(mdl, "TiterData")                          //
        .def(py::init<>(), py::doc("reference-panel-plots support")) //
        .def(
            "add", [](TiterData& self, const ChartModify& chart) { self.add(chart); }, "chart"_a) //
        .def("all_antigens", &TiterData::all_antigens)                                            //
        .def("all_sera", &TiterData::all_sera)                                                    //
        // .def("__str__", [](const ProcrustesData& data) { return fmt::format("ProcrustesData(rms: {:.4f})", data.rms); }) //
        ;

} // acmacs_py::chart_util
// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
