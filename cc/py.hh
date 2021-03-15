#pragma once

#include "acmacs-base/pybind11.hh"

// ----------------------------------------------------------------------

namespace acmacs_py
{
    void chart(py::module_& mdl); // py-chart.cc
    void antigen(py::module_& mdl); // py-antigen.cc
    void common(py::module_& mdl); // py-common.cc
    void merge(py::module_& mdl);  // py-merge.cc

    namespace DEPRECATED
    {
        void antigen_indexes(py::module_& mdl); // py-antigen.cc
    }

}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
