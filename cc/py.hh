#pragma once

#include "acmacs-base/pybind11.hh"

// ----------------------------------------------------------------------

namespace acmacs_py
{
    void chart(py::module_& mdl);      // py-chart.cc
    void chart_util(py::module_& mdl); // py-chart-util.cc
    void titers(py::module_& mdl);     // py-titers.cc
    void antigen(py::module_& mdl);    // py-antigen.cc
    void common(py::module_& mdl);     // py-common.cc
    void merge(py::module_& mdl);      // py-merge.cc
    void mapi(py::module_& mdl);       // py-mapi.cc
    void draw(py::module_& mdl);       // py-draw.cc
    void seqdb(py::module_& mdl);      // py-seqdb.cc

    namespace DEPRECATED
    {
        void antigen_indexes(py::module_& mdl); // py-antigen.cc
    }

} // namespace acmacs_py

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
