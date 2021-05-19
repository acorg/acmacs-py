#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/reference-panel-plot-data.hh"
#include "acmacs-draw/reference-panel-plot.hh"
#include "acmacs-py/py.hh"

// ----------------------------------------------------------------------

void acmacs_py::chart_util(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::chart;
    using namespace acmacs::draw;

    // reference-panel-plots support
    py::class_<ReferencePanelPlotData>(mdl, "ReferencePanelPlotData") //
        .def(py::init<>())                                            //
        .def(
            "add", [](ReferencePanelPlotData& self, const ChartModify& chart) { self.add(chart); }, "chart"_a)      //
        .def("antigens", &ReferencePanelPlotData::antigens, "min_tables"_a)                                         //
        .def("sera", &ReferencePanelPlotData::sera, "min_tables"_a)                                                 //
        .def("make_antigen_serum_table", &ReferencePanelPlotData::make_antigen_serum_table, "antigens"_a, "sera"_a) //
        ;

    // py::class_<ReferencePanelPlotData::AntigenSerumData>(mdl, "ReferencePanelPlotData_AntigenSerumData");
    py::class_<ReferencePanelPlotData::ASTable>(mdl, "ReferencePanelPlotData_ASTable");

    py::class_<ReferencePanelPlot>(mdl, "ReferencePanelPlot")                                                                                       //
        .def(py::init<>())                                                                                                                          //
        .def("parameters", py::overload_cast<>(&ReferencePanelPlot::parameters), py::return_value_policy::reference) //
        .def(
            "plot", [](const ReferencePanelPlot& plot, py::object path, const ReferencePanelPlotData::ASTable& data) { plot.plot(std::string{py::str(path)}, data); }, "filename"_a,
            "reference_panel_plot_data"_a) //
        ;

    py::class_<ReferencePanelPlot::Parameters>(mdl, "ReferencePanelPlot_Parameters") //
        .def(
            "title",
            [](ReferencePanelPlot::Parameters& parameters, const std::string& title) -> ReferencePanelPlot::Parameters& {
                parameters.title = title;
                return parameters;
            },
            "title"_a) //
        .def(
            "cell_label_scale",
            [](ReferencePanelPlot::Parameters& parameters, double scale) -> ReferencePanelPlot::Parameters& {
                parameters.cell_label_scale = scale;
                return parameters;
            },
            "scale"_a, py::doc("size of the label (antigen, serum name, titer value) within cell")) //
        .def(
            "cell_padding_scale",
            [](ReferencePanelPlot::Parameters& parameters, double scale) -> ReferencePanelPlot::Parameters& {
                parameters.cell_padding_scale = scale;
                return parameters;
            },
            "scale"_a, py::doc("size of the padding within cell")) //
        ;

} // acmacs_py::chart_util
// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
