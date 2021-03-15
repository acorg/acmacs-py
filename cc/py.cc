#include "acmacs-base/pybind11.hh"

// chart
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/selected-antigens-sera.hh"

// merge
#include "acmacs-chart-2/merge.hh"

// ======================================================================

inline unsigned make_info_data(bool column_bases, bool tables, bool tables_for_sera, bool antigen_dates)
{
    using namespace acmacs::chart;
    return (column_bases ? info_data::column_bases : 0)         //
           | (tables ? info_data::tables : 0)                   //
           | (tables_for_sera ? info_data::tables_for_sera : 0) //
           | (antigen_dates ? info_data::dates : 0);
}

// ----------------------------------------------------------------------

inline acmacs::chart::ChartClone::clone_data clone_type(const std::string& type)
{
    using namespace acmacs::chart;
    if (type == "titers")
        return ChartClone::clone_data::titers;
    else if (type == "projections")
        return ChartClone::clone_data::projections;
    else if (type == "plot_spec")
        return ChartClone::clone_data::plot_spec;
    else if (type == "projections_plot_spec")
        return ChartClone::clone_data::projections_plot_spec;
    else
        throw std::invalid_argument{fmt::format("Unrecognized clone \"type\": \"{}\"", type)};
}

// ----------------------------------------------------------------------

namespace DEPRECATED
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

} // namespace DEPRECATED

// ----------------------------------------------------------------------

inline void py_chart(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::chart;

    py::class_<ChartModify, std::shared_ptr<ChartModify>>(mdl, "Chart") //
        .def(py::init([](const std::string& filename) { return std::make_shared<ChartModify>(import_from_file(filename)); }), py::doc("imports chart from a file"))

        .def(
            "clone",                                                                                                                                           //
            [](ChartModify& chart, const std::string& type) -> std::shared_ptr<ChartModify> { return std::make_shared<ChartClone>(chart, clone_type(type)); }, //
            "type"_a = "titers",                                                                                                                               //
            py::doc(R"(type: "titers", "projections", "plot_spec", "projections_plot_spec")"))                                                                 //

        .def(
            "make_name",                                                            //
            [](const ChartModify& chart) { return chart.make_name(std::nullopt); }, //
            py::doc("returns name of the chart"))                                   //

        .def(
            "make_name",                                                                                   //
            [](const ChartModify& chart, size_t projection_no) { return chart.make_name(projection_no); }, //
            "projection_no"_a,                                                                             //
            py::doc("returns name of the chart with the stress of the passed projection"))                 //

        .def("subtype", [](const ChartModify& chart) { return *chart.info()->virus_type(); })                            //
        .def("subtype_short", [](const ChartModify& chart) { return std::string{chart.info()->virus_type().h_or_b()}; }) //
        .def("subset", [](const ChartModify& chart) { return chart.info()->subset(); })                                  //
        .def("assay", [](const ChartModify& chart) { return *chart.info()->assay(); })                                   //
        .def("assay_hi_or_neut", [](const ChartModify& chart) { return chart.info()->assay().hi_or_neut(); })            //
        .def("lab", [](const ChartModify& chart) { return *chart.info()->lab(); })                                       //
        .def("rbc", [](const ChartModify& chart) { return *chart.info()->rbc_species(); })                               //
        .def("date", [](const ChartModify& chart) { return *chart.info()->date(Info::Compute::Yes); })                   //
        .def(
            "lineage", [](const ChartModify& chart) { return *chart.lineage(); }, py::doc("returns chart lineage: VICTORIA, YAMAGATA")) //

        .def("description",                                 //
             &Chart::description,                           //
             py::doc("returns chart one line description")) //

        .def(
            "make_info", //
            [](const ChartModify& chart, size_t max_number_of_projections_to_show, bool column_bases, bool tables, bool tables_for_sera, bool antigen_dates) {
                return chart.make_info(max_number_of_projections_to_show, make_info_data(column_bases, tables, tables_for_sera, antigen_dates));
            },                                                                                                                                               //
            "max_number_of_projections_to_show"_a = 20, "column_bases"_a = true, "tables"_a = false, "tables_for_sera"_a = false, "antigen_dates"_a = false, //
            py::doc("returns detailed chart description"))                                                                                                   //

        .def("number_of_antigens", &Chart::number_of_antigens)
        .def("number_of_sera", &Chart::number_of_sera)
        .def("number_of_projections", &Chart::number_of_projections)

        .def(
            "relax", //
            [](ChartModify& chart, size_t number_of_dimensions, size_t number_of_optimizations, const std::string& minimum_column_basis, bool dimension_annealing, bool rough,
               size_t /*number_of_best_distinct_projections_to_keep*/) {
                if (number_of_optimizations == 0)
                    number_of_optimizations = 100;
                chart.relax(number_of_optimizations_t{number_of_optimizations}, MinimumColumnBasis{minimum_column_basis}, acmacs::number_of_dimensions_t{number_of_dimensions},
                            use_dimension_annealing_from_bool(dimension_annealing), optimization_options{optimization_precision{rough ? optimization_precision::rough : optimization_precision::fine}});
                chart.projections_modify().sort();
            }, //
            "number_of_dimensions"_a = 2, "number_of_optimizations"_a = 0, "minimum_column_basis"_a = "none", "dimension_annealing"_a = false, "rough"_a = false,
            "number_of_best_distinct_projections_to_keep"_a = 5,                                                                              //
            py::doc{"makes one or more antigenic maps from random starting layouts, adds new projections, projections are sorted by stress"}) //

        .def(
            "relax_incremental", //
            [](ChartModify& chart, size_t number_of_optimizations, bool rough, size_t number_of_best_distinct_projections_to_keep, bool remove_source_projection, bool unmovable_non_nan_points) {
                if (number_of_optimizations == 0)
                    number_of_optimizations = 100;
                chart.relax_incremental(0, number_of_optimizations_t{number_of_optimizations},
                                        optimization_options{optimization_precision{rough ? optimization_precision::rough : optimization_precision::fine}},
                                        acmacs::chart::remove_source_projection{remove_source_projection ? acmacs::chart::remove_source_projection::yes : acmacs::chart::remove_source_projection::no},
                                        acmacs::chart::unmovable_non_nan_points{unmovable_non_nan_points ? acmacs::chart::unmovable_non_nan_points::yes : acmacs::chart::unmovable_non_nan_points::no});
                chart.projections_modify().sort();
            },                                                                                                                                                                                  //
            "number_of_optimizations"_a = 0, "rough"_a = false, "number_of_best_distinct_projections_to_keep"_a = 5, "remove_source_projection"_a = true, "unmovable_non_nan_points"_a = false) //

        .def(
            "projection",                                                                                    //
            [](ChartModify& chart, size_t projection_no) { return chart.projection_modify(projection_no); }, //
            "projection_no"_a = 0)                                                                           //

        .def("remove_all_projections",                                                   //
             [](ChartModify& chart) { return chart.projections_modify().remove_all(); }) //

        .def(
            "keep_projections",                                                                               //
            [](ChartModify& chart, size_t to_keep) { return chart.projections_modify().keep_just(to_keep); }, //
            "keep"_a)                                                                                         //

        .def(
            "export",                                                                                                                                               //
            [](ChartModify& chart, const std::string& filename, const std::string& program_name) { acmacs::chart::export_factory(chart, filename, program_name); }, //
            "filename"_a, "program_name"_a)                                                                                                                         //

        .def("select_antigens",                                                                                                                                                //
             [](std::shared_ptr<ChartModify> chart) { return std::make_shared<SelectedAntigens>(chart); },                                                                          //
             py::doc(R"(Selects all antigens and returns SelectedAntigens object.)")) //
        .def("select_antigens",                                                                                                                                                //
             [](std::shared_ptr<ChartModify> chart, const std::function<bool(size_t, std::shared_ptr<Antigen>)>& func) { return std::make_shared<SelectedAntigens>(chart, func); }, //
             "predicate"_a, //
             py::doc(R"(Passed predicate (function with two args: antigen index and antigen object)
is called for each antigen, selects just antigens for which predicate
returns True, returns SelectedAntigens object.)")) //

        .def("select_sera",                                                                                                                                          //
             [](std::shared_ptr<ChartModify> chart) { return std::make_shared<SelectedSera>(chart); },                                                                        //
             py::doc(R"(Selects all sera and returns SelectedSera object.)")) //
        .def("select_sera",                                                                                                                                          //
             [](std::shared_ptr<ChartModify> chart, const std::function<bool(size_t, std::shared_ptr<Serum>)>& func) { return std::make_shared<SelectedSera>(chart, func); }, //
             "predicate"_a, //
             py::doc(R"(Passed predicate (function with two args: serum index and serum object)
is called for each serum, selects just sera for which predicate
returns True, returns SelectedAntigens object.)")) //

        // DEPRECATED

        .def(
            "antigen_indexes",                                                                                 //
            [](ChartModify& chart) { return std::make_shared<DEPRECATED::AntigenIndexes>(chart.antigens()); }, //
            py::doc(R"(DEPRECATED, use chart.select_antigens())"))                                             //
        .def(
            "serum_indexes",                                                                             //
            [](ChartModify& chart) { return std::make_shared<DEPRECATED::SerumIndexes>(chart.sera()); }, //
            py::doc(R"(DEPRECATED, use chart.select_sera())"))                                           //
        .def(
            "remove_antigens_sera",
            [](ChartModify& chart, std::shared_ptr<DEPRECATED::AntigenIndexes> antigens, std::shared_ptr<DEPRECATED::SerumIndexes> sera, bool remove_projections) {
                if (remove_projections)
                    chart.projections_modify().remove_all();
                if (antigens && !antigens->empty())
                    chart.remove_antigens(acmacs::ReverseSortedIndexes{*antigens->indexes});
                if (sera && !sera->empty())
                    chart.remove_sera(acmacs::ReverseSortedIndexes{*sera->indexes});
            },                                                                          //
            "antigens"_a = nullptr, "sera"_a = nullptr, "remove_projections"_a = false, //
            py::doc(R"(DEPRECATED, use chart.select_antigens
Usage:
    chart.remove_antigens_sera(antigens=c.antigen_indexes().filter_lineage(\"yamagata\"), sera=c.serum_indexes().filter_lineage(\"yamagata\"))
    chart.remove_antigens_sera(sera=chart.serum_indexes().filter_serum_id("A8658-14D"))
)"))                                                                                    //

        .def(
            "modify_titers",
            [](ChartModify& chart, const std::string& look_for, const std::string& replacement, bool verbose) {
                const auto replacements = chart.titers_modify().replace_all(std::regex{look_for}, replacement);
                if (verbose) {
                    if (!replacements.empty()) {
                        AD_INFO("{} titer replacements done", replacements.size());
                        for (const auto& rep : replacements)
                            fmt::print(stderr, "    ag:{:04d} sr:{:03d} titer:{}\n", rep.antigen, rep.serum, rep.titer);
                    }
                    else
                        AD_WARNING("No titer replacement performed: no titer match for \"{}\"", look_for);
                }
            },                                                  //
            "look_for"_a, "replacement"_a, "verbose"_a = false, //
            py::doc(R"(look_for is regular expression,
replacement is replacement with substitutions:
    $1 - match of the first subexpression
    $2 - match of the second subexpression
    ...
    $` - prefix before match
    $' - suffix after match
Usage:
    chart.modify_titers(look_for=">", replacement="$`$'", verbose=True)
)"))                                                            //
        ;

    // ----------------------------------------------------------------------

    py::class_<acmacs::virus::Passage>(mdl, "Passage")                                                                                  //
        .def("__str__", [](const acmacs::virus::Passage& passage) { return *passage; })                                                 //
        .def("__eq__", [](const acmacs::virus::Passage& passage, std::string_view str) { return *passage == str; })                     //
        .def("__eq__", [](const acmacs::virus::Passage& passage, const acmacs::virus::Passage& another) { return passage == another; }) //
        .def("type", &acmacs::virus::Passage::passage_type)                                                                             //
        .def("is_egg", &acmacs::virus::Passage::is_egg)                                                                                 //
        .def("is_cell", &acmacs::virus::Passage::is_cell)                                                                               //
        .def("without_date", &acmacs::virus::Passage::without_date)                                                                     //
        .def("last_number", &acmacs::virus::Passage::last_number)                                                                       //
        .def("last_type", &acmacs::virus::Passage::last_type)                                                                           //
        ;

    py::class_<acmacs::chart::Annotations>(mdl, "Annotations") //
        .def("distinct", &Annotations::distinct)               //
        ;

    // ----------------------------------------------------------------------

    py::class_<detail::AntigenSerum, std::shared_ptr<detail::AntigenSerum>>(mdl, "AntigenSerum")                                                    //
        .def("name", [](const detail::AntigenSerum& ag_sr) { return *ag_sr.name(); })                                                               //
        .def("name_full", &detail::AntigenSerum::name_full)                                                                                         //
        .def("passage", &detail::AntigenSerum::passage)                                                                                             //
        .def("lineage", [](const detail::AntigenSerum& ag_sr) { return ag_sr.lineage().to_string(); })                                              //
        .def("reassortant", [](const detail::AntigenSerum& ag_sr) { return *ag_sr.reassortant(); })                                                 //
        .def("annotations", &detail::AntigenSerum::annotations)                                                                                     //
        .def("format", [](const detail::AntigenSerum& ag_sr, const std::string& pattern) { return ag_sr.format(pattern, collapse_spaces_t::yes); }) //
        .def("is_egg", &detail::AntigenSerum::is_egg)                                                                                               //
        .def("is_cell", &detail::AntigenSerum::is_cell)                                                                                             //
        .def("passage_type", &detail::AntigenSerum::passage_type)                                                                                   //
        .def("distinct", &detail::AntigenSerum::distinct)                                                                                           //
        ;

    py::class_<Antigen, std::shared_ptr<Antigen>, detail::AntigenSerum>(mdl, "Antigen") //
        .def("date", [](const Antigen& ag) { return *ag.date(); })                      //
        .def("reference", &Antigen::reference)                                          //
        .def("lab_ids", [](const Antigen& ag) { return *ag.lab_ids(); })                //
        ;

    py::class_<Serum, std::shared_ptr<Serum>, detail::AntigenSerum>(mdl, "Serum")  //
        .def("serum_id", [](const Serum& sr) { return *sr.serum_id(); })           //
        .def("serum_species", [](const Serum& sr) { return *sr.serum_species(); }) //
        .def("homologous_antigens", &Serum::homologous_antigens)                   //
        ;

    // ----------------------------------------------------------------------

    py::class_<SelectedAntigens, std::shared_ptr<SelectedAntigens>>(mdl, "SelectedAntigens")
        .def("empty", &SelectedAntigens::empty)
        .def("size", &SelectedAntigens::size)
        .def("indexes", [](const SelectedAntigens& selected) { return *selected.indexes; })
        .def("report", &SelectedAntigens::report, "format"_a = "{no0},");

    py::class_<SelectedSera, std::shared_ptr<SelectedSera>>(mdl, "SelectedSera")
        .def("empty", &SelectedSera::empty)
        .def("size", &SelectedSera::size)
        .def("indexes", [](const SelectedSera& selected) { return *selected.indexes; })
        .def("report", &SelectedSera::report, "format"_a = "{no0},");

    // ----------------------------------------------------------------------

    py::class_<ProjectionModify, std::shared_ptr<ProjectionModify>>(mdl, "Projection") //
        .def(
            "stress",                                                                                                                                                      //
            [](const ProjectionModify& projection, bool recalculate) { return projection.stress(recalculate ? RecalculateStress::if_necessary : RecalculateStress::no); }, //
            "recalculate"_a = false)                                                                                                                                       //
        ;

    // ----------------------------------------------------------------------
    // DEPRECATED

    py::class_<DEPRECATED::AntigenIndexes, std::shared_ptr<DEPRECATED::AntigenIndexes>>(mdl, "AntigenIndexes") //
        .def("__str__", [](const DEPRECATED::AntigenIndexes& indexes) { return fmt::format("DEPRECATED::AntigenIndexes({}){}", indexes.indexes.size(), indexes.indexes); })
        .def("empty", &DEPRECATED::AntigenIndexes::empty)

        .def(
            "filter_lineage",
            [](DEPRECATED::AntigenIndexes& indexes, const std::string& lineage) {
                indexes.ag_sr->filter_lineage(indexes.indexes, acmacs::chart::BLineage{lineage});
                return indexes;
            },           //
            "lineage"_a) //
        ;

    py::class_<DEPRECATED::SerumIndexes, std::shared_ptr<DEPRECATED::SerumIndexes>>(mdl, "DEPRECATED_SerumIndexes") //
        .def("__str__", [](const DEPRECATED::SerumIndexes& indexes) { return fmt::format("DEPRECATED::SerumIndexes({}){}", indexes.indexes.size(), indexes.indexes); })
        .def("empty", &DEPRECATED::SerumIndexes::empty)

        .def(
            "filter_lineage",
            [](DEPRECATED::SerumIndexes& indexes, const std::string& lineage) {
                indexes.ag_sr->filter_lineage(indexes.indexes, acmacs::chart::BLineage{lineage});
                return indexes;
            },           //
            "lineage"_a) //
        .def(
            "filter_serum_id",
            [](DEPRECATED::SerumIndexes& indexes, const std::string& serum_id) {
                indexes.ag_sr->filter_serum_id(indexes.indexes, serum_id);
                return indexes;
            },            //
            "serum_id"_a) //
        ;
}

// ======================================================================

inline void py_merge(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::chart;

    // TODO: antigens only, sera only
    py::class_<CommonAntigensSera>(mdl, "CommonAntigensSera") //
        .def(py::init([](const ChartModify& primary, const ChartModify& secondary, const std::string& match_level) {
                 return new CommonAntigensSera(primary, secondary, CommonAntigensSera::match_level(match_level));
             }),
             "primary"_a, "secondary"_a, "match_level"_a = "auto",
             py::doc(R"(match_level: "strict", "relaxed", "ignored", "auto")")) //_
        .def(py::init([](const ChartModify& primary, const ChartModify& secondary, common::antigen_selector_t antigen_entry_extractor, common::serum_selector_t serum_entry_extractor, const std::string& match_level) {
            return new CommonAntigensSera(primary, secondary, antigen_entry_extractor, serum_entry_extractor, CommonAntigensSera::match_level(match_level));
             }),
            "primary"_a, "secondary"_a, "antigen_entry_extractor"_a, "serum_entry_extractor"_a, "match_level"_a = "auto",
             py::doc(R"(match_level: "strict", "relaxed", "ignored", "auto")")) //_
        .def(py::init([](const Chart& chart) { return new CommonAntigensSera(chart); }), "chart"_a,
             py::doc(R"(for procrustes between projections of the same chart)")) //_
        .def("report", &CommonAntigensSera::report, "indent"_a = 0);

    py::class_<ProcrustesData>(mdl, "ProcrustesData") //
        .def_readonly("rms", &ProcrustesData::rms)    //
        ;

    py::class_<common::AntigenEntry>(mdl, "common_AntigenEntry") //
        .def(py::init<size_t, const Antigen&>())                 //
        .def_property(
            "name", [](const common::AntigenEntry& en) { return *en.name; }, [](common::AntigenEntry& en, const std::string& new_name) { en.make_orig(); en.name = new_name; }) //
        .def("full_name", &common::AntigenEntry::full_name)                                                                                                     //
        .def_property(
            "reassortant", [](const common::AntigenEntry& en) { return *en.reassortant; },
            [](common::AntigenEntry& en, const std::string& new_reassortant) { en.make_orig(); en.reassortant = acmacs::virus::Reassortant{new_reassortant}; }) //
        .def_property(
            "annotations", [](const common::AntigenEntry& en) { return *en.annotations; },
            [](common::AntigenEntry& en, const std::vector<std::string>& new_annotations) { en.make_orig(); en.annotations = Annotations{new_annotations}; }) //
        .def_property(
            "passage", [](const common::AntigenEntry& en) { return *en.passage; },
            [](common::AntigenEntry& en, const std::string& new_passage) { en.make_orig(); en.passage = acmacs::virus::Passage{new_passage}; }) //
        ;

    py::class_<common::SerumEntry>(mdl, "common_SerumEntry") //
        .def(py::init<size_t, const Serum&>())                 //
        .def_property(
            "name", [](const common::SerumEntry& en) { return *en.name; }, [](common::SerumEntry& en, const std::string& new_name) { en.make_orig(); en.name = new_name; }) //
        .def("full_name", &common::SerumEntry::full_name)                                                                                                     //
        .def_property(
            "reassortant", [](const common::SerumEntry& en) { return *en.reassortant; },
            [](common::SerumEntry& en, const std::string& new_reassortant) { en.make_orig(); en.reassortant = acmacs::virus::Reassortant{new_reassortant}; }) //
        .def_property(
            "annotations", [](const common::SerumEntry& en) { return *en.annotations; },
            [](common::SerumEntry& en, const std::vector<std::string>& new_annotations) { en.make_orig(); en.annotations = Annotations{new_annotations}; }) //
        .def_property(
            "serum_id", [](const common::SerumEntry& en) { return *en.serum_id; },
            [](common::SerumEntry& en, const std::string& new_serum_id) { en.make_orig(); en.serum_id = SerumId{new_serum_id}; }) //
        .def_property(
            "passage", [](const common::SerumEntry& en) { return *en.passage; },
            [](common::SerumEntry& en, const std::string& new_passage) { en.make_orig(); en.passage = acmacs::virus::Passage{new_passage}; }) //
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

    py::class_<MergeReport>(mdl, "MergeReport");

} // py_merge

// ----------------------------------------------------------------------

// https://pybind11.readthedocs.io/en/latest/faq.html#how-can-i-reduce-the-build-time

PYBIND11_MODULE(acmacs, mdl)
{
    mdl.doc() = "Acmacs backend";
    py_chart(mdl);
    py_merge(mdl);
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
