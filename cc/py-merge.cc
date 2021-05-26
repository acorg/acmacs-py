#include "acmacs-chart-2/merge.hh"

#include "acmacs-py/py.hh"

// ----------------------------------------------------------------------

void acmacs_py::merge(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::chart;

    py::class_<ProcrustesData>(mdl, "ProcrustesData")                                                                    //
        .def("__str__", [](const ProcrustesData& data) { return fmt::format("ProcrustesData(rms: {:.4f})", data.rms); }) //
        .def_readonly("rms", &ProcrustesData::rms)                                                                       //
        ;

    mdl.def(
        "procrustes",
        [](const Projection& primary, const Projection& secondary, const CommonAntigensSera& common, bool scaling) {
            return procrustes(primary, secondary, common.points(), scaling ? procrustes_scaling_t::yes : procrustes_scaling_t::no);
        },
        "primary"_a, "secondary"_a, "common"_a, "scaling"_a = false);

    mdl.def(
        "merge",
        [](std::shared_ptr<ChartModify> chart1, std::shared_ptr<ChartModify> chart2, const std::string& merge_type, const std::string& match, bool a_combine_cheating_assays, bool a_remove_distinct) {
            CommonAntigensSera::match_level_t match_level{CommonAntigensSera::match_level_t::automatic};
            if (match == "auto" || match == "automatic")
                match_level = CommonAntigensSera::match_level_t::automatic;
            else if (match == "strict")
                match_level = CommonAntigensSera::match_level_t::strict;
            else if (match == "relaxed")
                match_level = CommonAntigensSera::match_level_t::relaxed;
            else if (match == "ignored")
                match_level = CommonAntigensSera::match_level_t::ignored;
            else
                throw std::invalid_argument{fmt::format("Unrecognized \"match\": \"{}\"", match)};

            projection_merge_t merge_typ{projection_merge_t::type1};
            if (merge_type == "type1" || merge_type == "tables-only")
                merge_typ = projection_merge_t::type1;
            else if (merge_type == "type2" || merge_type == "incremental")
                merge_typ = projection_merge_t::type2;
            else if (merge_type == "type3")
                merge_typ = projection_merge_t::type3;
            else if (merge_type == "type4")
                merge_typ = projection_merge_t::type4;
            else if (merge_type == "type5")
                merge_typ = projection_merge_t::type5;
            else
                throw std::invalid_argument{fmt::format("Unrecognized \"merge_type\": \"{}\"", merge_type)};

            return merge(*chart1, *chart2,
                         MergeSettings{match_level, merge_typ, a_combine_cheating_assays ? combine_cheating_assays::yes : combine_cheating_assays::no,
                                       a_remove_distinct ? remove_distinct::yes : remove_distinct::no});
        },                                                                                                                      //
        "chart1"_a, "chart2"_a, "type"_a, "match"_a = "auto", "combine_cheating_assays"_a = false, "remove_distinct"_a = false, //
        py::doc(R"(merges two charts
type: "type1" ("tables-only"), "type2" ("incremental"), "type3", "type4", "type5"
      see https://github.com/acorg/acmacs-chart-2/blob/master/doc/merge-types.org
match: "strict", "relaxed", "ignored", "automatic" ("auto")
)"));

    py::class_<MergeReport>(mdl, "MergeReport") //
        .def(
            "report_common", [](const MergeReport& report, size_t indent) { return report.common.report(indent); }, "indent"_a = 0) //
        ;

} // acmacs_py::merge

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
