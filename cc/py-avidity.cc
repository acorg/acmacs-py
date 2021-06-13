#include "acmacs-chart-2/avidity-test.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-py/py.hh"

void acmacs_py::avidity(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::chart;

    py::class_<avidity::Settings>(mdl, "AvidityStettings").def(py::init<double, double, double, size_t>(), "step"_a = 1.0, "min_adjust"_a = -6.0, "max_adjust"_a = 6.0, "threads"_a = 0);

    py::class_<avidity::Results>(mdl, "AvidityResults");

    mdl.def(
        "avidity_test", [](ChartModify& chart, size_t projection_no, const avidity::Settings& settings) { return avidity::test(chart, projection_no, settings, optimization_options{}); }, //
        "chart"_a, "projection_no"_a = 0, "settings"_a = avidity::Settings{});
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
