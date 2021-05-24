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
} // namespace acmacs_py

// ----------------------------------------------------------------------

void acmacs_py::titers(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::chart;

    py::class_<TitersModify, std::shared_ptr<TitersModify>>(mdl, "Titers")                                   //
        .def("number_of_layers", &TitersModify::number_of_layers)                                            //
        .def("remove_layers", &TitersModify::remove_layers, py::doc("remove layers, e.g. to modify titers")) //
        .def("modify", &titers_modify,                                                                       //
             "look_for"_a, "replacement"_a, "verbose"_a = false,                                             //
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
)"))                                                                                                         //
        ;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
