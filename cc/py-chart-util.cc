#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/titer-data.hh"
#include "acmacs-py/py.hh"

// ----------------------------------------------------------------------

void acmacs_py::chart_util(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::chart;

    // reference-panel-plots support
    py::class_<ReferencePanelPlotData>(mdl, "ReferencePanelPlotData") //
        .def(py::init<>(), py::doc("reference-panel-plots support"))  //
        .def(
            "add", [](ReferencePanelPlotData& self, const ChartModify& chart) { self.add(chart); }, "chart"_a) //
        .def("antigens", &ReferencePanelPlotData::antigens, "min_tables"_a)                                    //
        .def("sera", &ReferencePanelPlotData::sera, "min_tables"_a)                                            //
        ;

} // acmacs_py::chart_util
// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
