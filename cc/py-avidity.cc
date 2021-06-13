#include "acmacs-chart-2/avidity-test.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-py/py.hh"

void acmacs_py::avidity(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::chart;

    py::class_<avidity::Settings>(mdl, "AvidityStettings")                                                                             //
        .def(py::init<double, double, double, size_t>(), "step"_a = 1.0, "min_adjust"_a = -6.0, "max_adjust"_a = 6.0, "threads"_a = 0) //
        ;

    py::class_<avidity::MostMoved>(mdl, "AvidityMostMoved")          //
        .def_readonly("antigen_no", &avidity::MostMoved::antigen_no) //
        .def_readonly("distance", &avidity::MostMoved::distance)     //
        ;

    py::class_<avidity::PerAdjust>(mdl, "AvidityPerAdjust")                                                                                      //
        .def_readonly("logged_adjust", &avidity::PerAdjust::logged_adjust)                                                                       //
        .def_readonly("distance_test_antigen", &avidity::PerAdjust::distance_test_antigen)                                                       //
        .def_readonly("angle_test_antigen", &avidity::PerAdjust::angle_test_antigen)                                                             //
        .def_readonly("average_procrustes_distances_except_test_antigen", &avidity::PerAdjust::average_procrustes_distances_except_test_antigen) //
        .def_readonly("stress_diff", &avidity::PerAdjust::stress_diff)                                                                           //
        .def_property_readonly("final_coordinates",
                               [](const avidity::PerAdjust& per_adjust) { return std::vector<double>(per_adjust.final_coordinates.begin(), per_adjust.final_coordinates.end()); }) //
        .def_readonly("most_moved", &avidity::PerAdjust::most_moved)                                                                                                               //
        ;

    py::class_<avidity::Result>(mdl, "AvidityResult")                                                                                                            //
        .def_readonly("antigen_no", &avidity::Result::antigen_no)                                                                                                //
        .def_readonly("best_logged_adjust", &avidity::Result::best_logged_adjust)                                                                                //
        .def_property_readonly("original_coordinates", [](const avidity::Result& res) { return std::vector<double>(res.original.begin(), res.original.end()); }) //
        .def_readonly("adjusts", &avidity::Result::adjusts)                                                                                                      //
        .def_property_readonly("best_adjust", [](const avidity::Result& res) { return res.best_adjust(); })                                                      //
        ;

    py::class_<avidity::Results>(mdl, "AvidityResults")                                                     //
        .def("__getitem__", [](const avidity::Results& results, size_t no) { return results.results[no]; }) //
        .def("__len__", [](const avidity::Results& results) { return results.results.size(); })             //
        .def(
            "__iter__", [](const avidity::Results& results) { return py::make_iterator(results.results.begin(), results.results.end()); }, py::keep_alive<0, 1>()) //
        .def("__bool__", [](const avidity::Results& results) { return !results.results.empty(); })                                                                 //
        .def("get", &avidity::Results::get, "antigen_no"_a)                                                                                                        //
        ;

    mdl.def(
        "avidity_test", [](ChartModify& chart, size_t projection_no, const avidity::Settings& settings) { return avidity::test(chart, projection_no, settings, optimization_options{}); }, //
        "chart"_a, "projection_no"_a = 0, "settings"_a = avidity::Settings{});
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
