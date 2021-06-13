#include "acmacs-base/log.hh"
#include "acmacs-py/py.hh"

// ======================================================================

PYBIND11_MODULE(acmacs, mdl)
{
    using namespace pybind11::literals;

    mdl.doc() = "Acmacs backend";
    acmacs_py::chart(mdl);
    acmacs_py::chart_util(mdl);
    acmacs_py::avidity(mdl);
    acmacs_py::antigen(mdl);
    acmacs_py::titers(mdl);
    acmacs_py::DEPRECATED::antigen_indexes(mdl);
    acmacs_py::common(mdl);
    acmacs_py::merge(mdl);
    acmacs_py::mapi(mdl);
    acmacs_py::draw(mdl);
    acmacs_py::seqdb(mdl);

    // ----------------------------------------------------------------------

    mdl.def(
        "enable_logging",                                                             //
        [](const std::string& keys) { acmacs::log::enable(std::string_view{keys}); }, //
        "keys"_a);
    mdl.def("logging_enabled", &acmacs::log::report_enabled);
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
