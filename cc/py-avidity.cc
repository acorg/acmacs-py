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
        .def("__str__", [](const avidity::PerAdjust& per_adjust) { return fmt::format("{}", per_adjust); })                                                                        //
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

    mdl.def(
        "avidity_test",
        [](ChartModify& chart, size_t projection_no, size_t antigen_no, double logged_adjust, bool add_new_projection_to_chart) {
            return avidity::test(chart, *chart.projection_modify(projection_no), antigen_no, logged_adjust, optimization_options{}, add_new_projection_to_chart);
        }, //
        "chart"_a, "projection_no"_a = 0, "antigen_no"_a, "logged_adjust"_a, "add_new_projection_to_chart"_a);

    mdl.def("avidity_move_antigens", &avidity::move_antigens, "chart"_a, "projection_no"_a = 0, "results"_a,
            py::doc("creates new projection based on avidity test results, projection is added to the chart and returned"));

    mdl.def(
        "avidity_relax", //
        [](ChartModify& chart, size_t number_of_optimizations, size_t number_of_dimensions, std::string_view minimum_column_basis, bool rough, const std::vector<double>& logged_avidity_adjusts) {
            if (number_of_optimizations == 0)
                number_of_optimizations = 100;
            avidity::relax(chart, number_of_optimizations_t{number_of_optimizations}, acmacs::number_of_dimensions_t{number_of_dimensions}, MinimumColumnBasis{minimum_column_basis},
                           AvidityAdjusts{}.from_logged(logged_avidity_adjusts), optimization_options{optimization_precision{rough ? optimization_precision::rough : optimization_precision::fine}});
            chart.projections_modify().sort();
        }, //
        "chart"_a, "number_of_optimizations"_a = 0, "number_of_dimensions"_a = 2, "minimum_column_basis"_a = "none", "rough"_a = false, "logged_avidity_adjusts"_a,
        py::doc{"makes one or more antigenic maps from random starting layouts with the passed avidity adjusts, adds new projections, projections are sorted by stress"});
}

    // ----------------------------------------------------------------------
    /// Local Variables:
    /// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
    /// End:
