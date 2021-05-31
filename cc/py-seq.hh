#include "acmacs-chart-2/point-index-list.hh"
#include "seqdb-3/seqdb.hh"
#include "seqdb-3/aa-at-pos.hh"

// ----------------------------------------------------------------------

namespace acmacs_py
{
    template <typename AgSr> bool all_criteria_matched(const AgSr& antigen, const acmacs::seqdb::amino_acid_at_pos1_eq_list_t& criteria)
    {
        const acmacs::seqdb::sequence_aligned_t seq{antigen.sequence_aa()};
        for (const auto& pos1_aa_eq : criteria) {
            if (std::get<bool>(pos1_aa_eq) != (seq.at(std::get<acmacs::seqdb::pos1_t>(pos1_aa_eq)) == std::get<char>(pos1_aa_eq)))
                return false;
        }
        return true;
    }

    template <typename AgSr> void select_by_aa(acmacs::chart::PointIndexList& indexes, const AgSr& antigens, const std::vector<std::string>& criteria)
    {
        const auto crits = acmacs::seqdb::extract_aa_at_pos1_eq_list(criteria);
        const auto not_all_criteria_matched_pred = [&crits, &antigens](auto index) { return !all_criteria_matched(*antigens.at(index), crits); };
        indexes.get().erase(std::remove_if(indexes.begin(), indexes.end(), not_all_criteria_matched_pred), indexes.end());
    }

    // ----------------------------------------------------------------------

    template <typename AgSr> void deselect_by_aa(acmacs::chart::PointIndexList& indexes, const AgSr& antigens, const std::vector<std::string>& criteria)
    {
        const auto crits = acmacs::seqdb::extract_aa_at_pos1_eq_list(criteria);
        const auto all_criteria_matched_pred = [&crits, &antigens](auto index) { return all_criteria_matched(*antigens.at(index), crits); };
        indexes.get().erase(std::remove_if(indexes.begin(), indexes.end(), all_criteria_matched_pred), indexes.end());
    }

    // ----------------------------------------------------------------------

    template <typename AgSr> void deselect_not_sequenced(acmacs::chart::PointIndexList& indexes, const AgSr& antigens)
    {
        indexes.get().erase(std::remove_if(indexes.begin(), indexes.end(), [&antigens](auto index) { return antigens.at(index)->sequence_aa().empty(); }), indexes.end());
    }

} // namespace acmacs_py

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
