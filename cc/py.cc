#include "acmacs-py/py.hh"

// ======================================================================

PYBIND11_MODULE(acmacs, mdl)
{
    mdl.doc() = "Acmacs backend";
    acmacs_py::chart(mdl);
    acmacs_py::antigen(mdl);
    acmacs_py::DEPRECATED::antigen_indexes(mdl);
    acmacs_py::common(mdl);
    acmacs_py::merge(mdl);
    acmacs_py::mapi(mdl);
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
