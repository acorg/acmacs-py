
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-py/py.hh"

// ----------------------------------------------------------------------

namespace acmacs_py
{
    static inline void titers_modify(acmacs::chart::TitersModify& titers, const std::string& look_for, const std::string& replacement, bool verbose)
    {
        const auto replacements = titers.replace_all(std::regex{look_for}, replacement);
        if (verbose) {
            if (!replacements.empty()) {
                AD_INFO("{} titer replacements done", replacements.size());
                for (const auto& rep : replacements)
                    fmt::print(stderr, "    ag:{:3d} sr:{:3d} titer:{}\n", rep.antigen, rep.serum, rep.titer);
            }
            else
                AD_WARNING("No titer replacement performed: no titer match for \"{}\"", look_for);
        }
    }

    static inline void set_titer(acmacs::chart::TitersModify& titers, size_t antigen_no, size_t serum_no, std::string_view titer)
    {
        titers.titer(antigen_no, serum_no, acmacs::chart::Titer{titer});
    }

} // namespace acmacs_py

// ----------------------------------------------------------------------

void acmacs_py::titers(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::chart;

    py::class_<TitersModify, std::shared_ptr<TitersModify>>(mdl, "Titers")                                               //
        .def("number_of_layers", &TitersModify::number_of_layers)                                                        //
        .def("remove_layers", &TitersModify::remove_layers, py::doc("remove layers, e.g. to modify titers"))             //
        .def("titer", py::overload_cast<size_t, size_t>(&TitersModify::titer, py::const_), "antigen_no"_a, "serum_no"_a) //
        .def("set_titer", &set_titer, "antigen_no"_a, "serum_no"_a, "titer"_a)                                           //
        .def("layers_with_antigen", &TitersModify::layers_with_antigen, "antigen_no"_a)                                  //
        .def("layers_with_serum", &TitersModify::layers_with_serum, "serum_no"_a)                                        //
        .def("modify", &titers_modify,                                                                                   //
             "look_for"_a, "replacement"_a, "verbose"_a = false,                                                         //
             py::doc(R"(\
look_for is regular expression,
replacement is replacement with substitutions:
    $1 - match of the first subexpression
    $2 - match of the second subexpression
    ...
    $` - prefix before match
    $' - suffix after match
Usage:
    chart.titers().modify(look_for=">", replacement="$`$'", verbose=True)
)"))                                                                                                                     //
        ;

    py::class_<Titer>(mdl, "Titer")                                                                       //
        .def("__str__", [](const Titer& titer) { return *titer; })                                        //
        .def("is_invalid", py::overload_cast<>(&Titer::is_invalid, py::const_))                           //
        .def("is_dont_care", py::overload_cast<>(&Titer::is_dont_care, py::const_))                       //
        .def("is_regular", py::overload_cast<>(&Titer::is_regular, py::const_))                           //
        .def("is_less_than", py::overload_cast<>(&Titer::is_less_than, py::const_))                       //
        .def("is_more_than", py::overload_cast<>(&Titer::is_more_than, py::const_))                       //
        .def("logged", py::overload_cast<>(&Titer::logged, py::const_))                                   //
        .def("logged_with_thresholded", py::overload_cast<>(&Titer::logged_with_thresholded, py::const_)) //
        .def("logged_for_column_bases", py::overload_cast<>(&Titer::logged_for_column_bases, py::const_)) //
        .def("value", py::overload_cast<>(&Titer::value, py::const_))                                     //
        ;
}

// ----------------------------------------------------------------------
