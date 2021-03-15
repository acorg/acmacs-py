#pragma once

#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

namespace acmacs_py::DEPRECATED
{
    template <typename AgSr> struct AgSrIndexes
    {
        AgSrIndexes() = default;
        AgSrIndexes(std::shared_ptr<AgSr> a_ag_sr) : ag_sr{a_ag_sr}, indexes{a_ag_sr->all_indexes()} {}
        bool empty() const { return indexes.empty(); }

        std::shared_ptr<AgSr> ag_sr;
        acmacs::chart::Indexes indexes;
    };

    struct AntigenIndexes : public AgSrIndexes<acmacs::chart::Antigens>
    {
        using AgSrIndexes<acmacs::chart::Antigens>::AgSrIndexes;
    };
    struct SerumIndexes : public AgSrIndexes<acmacs::chart::Sera>
    {
        using AgSrIndexes<acmacs::chart::Sera>::AgSrIndexes;
    };

} // namespace acmacs_py::DEPRECATED

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
