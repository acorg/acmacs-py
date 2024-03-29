#include "acmacs-base/log.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/selected-antigens-sera.hh"
#include "acmacs-chart-2/text-export.hh"
#include "acmacs-chart-2/grid-test.hh"
#include "acmacs-py/py.hh"
#include "acmacs-py/py-seq.hh"
#include "acmacs-py/py-antigen-indexes.hh"

// ----------------------------------------------------------------------

namespace acmacs_py
{
    static inline unsigned make_info_data(bool column_bases, bool tables, bool tables_for_sera, bool antigen_dates)
    {
        using namespace acmacs::chart;
        return (column_bases ? info_data::column_bases : 0)         //
               | (tables ? info_data::tables : 0)                   //
               | (tables_for_sera ? info_data::tables_for_sera : 0) //
               | (antigen_dates ? info_data::dates : 0);
    }

    // ----------------------------------------------------------------------

    static inline acmacs::chart::ChartClone::clone_data clone_type(const std::string& type)
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
            throw std::invalid_argument{AD_FORMAT("Unrecognized clone \"type\": \"{}\"", type)};
    }

    static inline acmacs::chart::GridTest::Results grid_test(acmacs::chart::ChartModify& chart, std::shared_ptr<acmacs::chart::SelectedAntigensModify> antigens,
                                                             std::shared_ptr<acmacs::chart::SelectedSeraModify> sera, size_t projection_no, double grid_step, int threads, int move_relax)
    {
        const size_t total_attempts = move_relax > 0 ? static_cast<size_t>(move_relax) : 1ul;
        acmacs::chart::GridTest::Results results;
        size_t grid_projections = 0;
        for (size_t attempt = 0; attempt < total_attempts; ++attempt) {
            acmacs::chart::GridTest test{chart, projection_no, grid_step};
            if (!antigens && !sera) {
                results = test.test_all(threads);
            }
            else {
                acmacs::chart::Indexes points_to_test;
                if (antigens)
                    points_to_test = antigens->indexes;
                if (sera)
                    ranges::for_each(sera->indexes, [number_of_antigens = chart.number_of_antigens(), &points_to_test](auto index) { points_to_test.insert(index + number_of_antigens); });
                results = test.test(*points_to_test, threads);
            }
            if (move_relax) {
                auto projection = test.make_new_projection_and_relax(results, acmacs::verbose::yes);
                ++grid_projections;
                projection->comment("grid-test-" + acmacs::to_string(attempt));
                projection_no = projection->projection_no();
                if (ranges::all_of(results, [](const auto& result) { return result.diagnosis != acmacs::chart::GridTest::Result::trapped; }))
                    break;
            }
        }
        return results;
    }

    struct PlotSpecRef
    {
        std::shared_ptr<acmacs::chart::PlotSpecModify> plot_spec;
        size_t number_of_antigens;

        acmacs::PointStyle antigen(size_t antigen_no) const { return plot_spec->style(antigen_no); }
        acmacs::PointStyle serum(size_t serum_no) const { return plot_spec->style(number_of_antigens + serum_no); }
    };

    // ----------------------------------------------------------------------

    template <typename Selected> static inline std::map<char, std::shared_ptr<Selected>> antigens_sera_by_aa_at_pos(std::shared_ptr<acmacs::chart::ChartModify> chart, size_t pos1)
    {
        using namespace acmacs::chart;
        acmacs::seqdb::populate(*chart);
        const acmacs::seqdb::pos1_t pos{pos1};
        auto antigens = [&chart]() {
            if constexpr (std::is_same_v<Selected, SelectedAntigensModify>)
                return chart->antigens();
            else
                return chart->sera();
        }();

        std::map<char, std::shared_ptr<Selected>> result;
        for (const auto ag_no : range_from_0_to(antigens->size())) {
            const auto aa = acmacs::seqdb::sequence_aligned_t{antigens->at(ag_no)->sequence_aa()}.at(pos);
            const auto [it, added] = result.try_emplace(aa, std::make_shared<Selected>(chart, Selected::None));
            it->second->indexes.insert(ag_no);
        }
        return result;
    }

} // namespace acmacs_py

// ----------------------------------------------------------------------

void acmacs_py::chart(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::chart;

    py::class_<ChartModify, std::shared_ptr<ChartModify>>(mdl, "Chart") //
        .def(py::init([](py::object path) { return std::make_shared<ChartModify>(import_from_file(py::str(path))); }), "filename"_a, py::doc("imports chart from a file"))

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
        .def(
            "table_as_text", //
            [](const ChartModify& chart, int layer_no, bool sort) {
                const auto layer{layer_no >= 0 ? std::optional<size_t>{static_cast<size_t>(layer_no)} : std::nullopt};
                return acmacs::chart::export_table_to_text(chart, layer, sort);
            },                                                                                                                                                      //
            "layer"_a = -1, "sort"_a = false,                                                                                                                       //
            py::doc("returns table as text\nif layer >= 0 shows corresponding layer\nif sort is True sort antigens/sera to be able to compare with another table")) //
        .def(
            "names_as_text",                                                                                                                  //
            [](std::shared_ptr<ChartModify> chart, const std::string& format) { return acmacs::chart::export_names_to_text(chart, format); }, //
            "format"_a = "{ag_sr} {no0} {name_full}{ }{species}{ }{date_in_brackets}{ }{lab_ids}{ }{ref}\n",                                  //
            py::doc("returns antigen and /serum names as text"))                                                                              //
        .def(
            "names_as_text", //
            [](const ChartModify& chart, const SelectedAntigensModify& antigens, const SelectedSeraModify& sera, const std::string& format) {
                return acmacs::chart::export_names_to_text(chart, format, antigens, sera);
            },                                                                                                                       //
            "antigens"_a, "sera"_a, "format"_a = "{ag_sr} {no0} {name_full}{ }{species}{ }{date_in_brackets}{ }{lab_ids}{ }{ref}\n", //
            py::doc("returns antigen and /serum names as text for pre-selected antigens/sera"))                                      //

        .def("subtype", [](const ChartModify& chart) { return *chart.info()->virus_type(); })                            //
        .def("subtype_short", [](const ChartModify& chart) { return std::string{chart.info()->virus_type().h_or_b()}; }) //
        .def("subset", [](const ChartModify& chart) { return chart.info()->subset(); })                                  //
        .def("assay", [](const ChartModify& chart) { return *chart.info()->assay(); })                                   //
        .def("assay_hi_or_neut", [](const ChartModify& chart) { return chart.info()->assay().hi_or_neut(); })            //
        .def("lab", [](const ChartModify& chart) { return *chart.info()->lab(); })                                       //
        .def("rbc", [](const ChartModify& chart) { return *chart.info()->rbc_species(); })                               //
        .def("assay_rbc",
             [](const ChartModify& chart) {
                 const auto assay = chart.info()->assay().short_name();
                 if (assay == "HI")
                     return fmt::format("HI-{}", *chart.info()->rbc_species());
                 else
                     return assay;
             })                                                                                        //
        .def("date", [](const ChartModify& chart) { return *chart.info()->date(Info::Compute::Yes); }) //
        .def(
            "lineage", [](const ChartModify& chart) { return *chart.lineage(); }, py::doc("returns chart lineage: VICTORIA, YAMAGATA")) //
        .def("subtype_lineage",
             [](const ChartModify& chart) {
                 const auto subtype = chart.info()->virus_type().h_or_b();
                 if (subtype == "B")
                     return fmt::format("B{}", chart.lineage());
                 else
                     return std::string{subtype};
             }) //

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
        .def("__str__",                                                                                                                                      //
             [](const ChartModify& chart) { return chart.make_info(10, make_info_data(false, false, false, false)); })                                       //

        .def("number_of_antigens", &Chart::number_of_antigens)
        .def("number_of_sera", &Chart::number_of_sera)
        .def("number_of_projections", &Chart::number_of_projections)

        .def(
            "populate_from_seqdb",                                            //
            [](ChartModify& chart) { acmacs::seqdb::get().populate(chart); }, //
            py::doc("match seqdb, set lineages and clades"))

        .def(
            "relax", //
            [](ChartModify& chart, size_t number_of_dimensions, size_t number_of_optimizations, const std::string& minimum_column_basis, bool dimension_annealing, bool rough,
               size_t /*number_of_best_distinct_projections_to_keep*/, std::shared_ptr<SelectedAntigensModify> antigens_to_disconnect, std::shared_ptr<SelectedSeraModify> sera_to_disconnect) {
                if (number_of_optimizations == 0)
                    number_of_optimizations = 100;
                acmacs::chart::DisconnectedPoints disconnect;
                if (antigens_to_disconnect && !antigens_to_disconnect->empty())
                    disconnect.extend(antigens_to_disconnect->points());
                if (sera_to_disconnect && !sera_to_disconnect->empty())
                    disconnect.extend(sera_to_disconnect->points());
                chart.relax(number_of_optimizations_t{number_of_optimizations}, MinimumColumnBasis{minimum_column_basis}, acmacs::number_of_dimensions_t{number_of_dimensions},
                            use_dimension_annealing_from_bool(dimension_annealing), optimization_options{optimization_precision{rough ? optimization_precision::rough : optimization_precision::fine}},
                            disconnect);
                chart.projections_modify().sort();
            },                                                                                                                                                    //
            "number_of_dimensions"_a = 2, "number_of_optimizations"_a = 0, "minimum_column_basis"_a = "none", "dimension_annealing"_a = false, "rough"_a = false, //
            "unused_number_of_best_distinct_projections_to_keep"_a = 5, "disconnect_antigens"_a = nullptr, "disconnect_sera"_a = nullptr,                         //
            py::doc{"makes one or more antigenic maps from random starting layouts, adds new projections, projections are sorted by stress"})                     //

        .def(
            "relax_incremental", //
            [](ChartModify& chart, size_t projection_no, size_t number_of_optimizations, bool rough, size_t /*number_of_best_distinct_projections_to_keep*/, bool remove_source_projection,
               bool unmovable_non_nan_points) {
                if (number_of_optimizations == 0)
                    number_of_optimizations = 100;
                chart.relax_incremental(projection_no, number_of_optimizations_t{number_of_optimizations},
                                        optimization_options{optimization_precision{rough ? optimization_precision::rough : optimization_precision::fine}},
                                        acmacs::chart::remove_source_projection{remove_source_projection ? acmacs::chart::remove_source_projection::yes : acmacs::chart::remove_source_projection::no},
                                        acmacs::chart::unmovable_non_nan_points{unmovable_non_nan_points ? acmacs::chart::unmovable_non_nan_points::yes : acmacs::chart::unmovable_non_nan_points::no});
                chart.projections_modify().sort();
            }, //
            "projection_no"_a = 0, "number_of_optimizations"_a = 0, "rough"_a = false, "number_of_best_distinct_projections_to_keep"_a = 5, "remove_source_projection"_a = true,
            "unmovable_non_nan_points"_a = false) //

        .def("grid_test", &grid_test, "antigens"_a = nullptr, "sera"_a = nullptr, "projection_no"_a = 0, "grid_step"_a = 0.1, "threads"_a = 0, "move_relax"_a = 0) //

        .def(
            "projection",                                                                                    //
            [](ChartModify& chart, size_t projection_no) { return chart.projection_modify(projection_no); }, //
            "projection_no"_a = 0)                                                                           //

        .def("remove_all_projections",                                                   //
             [](ChartModify& chart) { return chart.projections_modify().remove_all(); }) //
        .def(
            "remove_all_projections_except",                                                                                    //
            [](ChartModify& chart, size_t to_keep) { return chart.projections_modify().remove_all_except(to_keep); }, "keep"_a) //
        .def(
            "remove_projection",                                                                                                  //
            [](ChartModify& chart, size_t to_remove) { return chart.projections_modify().remove(to_remove); }, "projection_no"_a) //
        .def(
            "keep_projections",                                                                               //
            [](ChartModify& chart, size_t to_keep) { return chart.projections_modify().keep_just(to_keep); }, //
            "keep"_a)                                                                                         //

        .def(
            "orient_to",
            [](ChartModify& chart, const ChartModify& master, size_t projection_no) {
                acmacs::chart::CommonAntigensSera common(master, chart, CommonAntigensSera::match_level_t::strict);
                const auto procrustes_data = acmacs::chart::procrustes(*master.projection(0), *chart.projection(projection_no), common.points(), acmacs::chart::procrustes_scaling_t::no);
                chart.projection_modify(projection_no)->transformation(procrustes_data.transformation);
            },                                 //
            "master"_a, "projection_no"_a = 0) //

        .def(
            "export", //
            [](ChartModify& chart, py::object path, py::object program_name) {
                const std::string path_s = py::str(path), pn_s = py::str(program_name);
                acmacs::chart::export_factory(chart, path_s, pn_s);
            },                                            //
            "filename"_a, "program_name"_a = "acmacs-py") //

        .def("antigen", &ChartModify::antigen, "antigen_no"_a) //
        .def("serum", &ChartModify::serum, "serum_no"_a)       //

        .def(
            "select_antigens", //
            [](std::shared_ptr<ChartModify> chart, const std::function<bool(const SelectionData<Antigen>&)>& func, size_t projection_no, bool report) {
                auto selected = std::make_shared<SelectedAntigensModify>(chart, func, projection_no);
                AD_PRINT_L(report, [&selected]() { return selected->report("{ag_sr} {no0:{num_digits}d} {name_full_passage}\n"); });
                return selected;
            },                                                        //
            "predicate"_a, "projection_no"_a = 0, "report"_a = false, //
            py::doc("Passed predicate (function with one arg: SelectionDataAntigen object)\n"
                    "is called for each antigen, selects just antigens for which predicate\n"
                    "returns True, returns SelectedAntigens object.")) //
        .def(
            "select_antigens_by_aa", //
            [](std::shared_ptr<ChartModify> chart, const std::vector<std::string>& criteria, bool report) {
                auto selected = std::make_shared<SelectedAntigensModify>(chart);
                acmacs::seqdb::populate(*chart);
                acmacs_py::select_by_aa(selected->indexes, *chart->antigens(), criteria);
                AD_PRINT_L(report, [&selected]() { return selected->report("{ag_sr} {no0:{num_digits}d} {name_full_passage}\n"); });
                return selected;
            },                                                                                                        //
            "criteria"_a, "report"_a = false,                                                                         //
            py::doc(R"(Criteria is a list of strings, e.g. ["156K", "!145K"], all criteria is the list must match)")) //
        .def(
            "select_antigens_by_clade", //
            [](std::shared_ptr<ChartModify> chart, const std::vector<std::string>& clades, bool report) {
                auto selected = std::make_shared<SelectedAntigensModify>(chart);
                acmacs::seqdb::populate(*chart);
                const auto pred = [&clades, antigens = chart->antigens()](auto index) { return antigens->at(index)->clades().exists_any_of(clades); };
                selected->indexes.get().erase(std::remove_if(selected->indexes.begin(), selected->indexes.end(), pred), selected->indexes.end());
                AD_PRINT_L(report, [&selected]() { return selected->report("{ag_sr} {no0:{num_digits}d} {name_full_passage}\n"); });
                return selected;
            },                                                                                                //
            "clades"_a, "report"_a = false,                                                                   //
            py::doc(R"(Select antigens with a clade from clades, one or more entries in clades must match)")) //
        .def(
            "select_all_antigens",                                                                              //
            [](std::shared_ptr<ChartModify> chart) { return std::make_shared<SelectedAntigensModify>(chart); }, //
            py::doc(R"(Selects all antigens and returns SelectedAntigens object.)"))                            //
        .def(
            "select_no_antigens",                                                                                                             //
            [](std::shared_ptr<ChartModify> chart) { return std::make_shared<SelectedAntigensModify>(chart, SelectedAntigensModify::None); }, //
            py::doc(R"(Selects no antigens and returns SelectedAntigens object.)"))                                                           //

        .def("antigens_by_aa_at_pos", &acmacs_py::antigens_sera_by_aa_at_pos<SelectedAntigensModify>, "pos"_a,
             py::doc(R"(Returns dict with AA at passed pos as keys and SelectedAntigens as values.)")) //
        .def("sera_by_aa_at_pos", &acmacs_py::antigens_sera_by_aa_at_pos<SelectedSeraModify>, "pos"_a,
             py::doc(R"(Returns dict with AA at passed pos as keys and SelectedSera as values.)")) //

        .def(
            "select_sera", //
            [](std::shared_ptr<ChartModify> chart, const std::function<bool(const SelectionData<Serum>&)>& func, size_t projection_no, bool report) {
                auto selected = std::make_shared<SelectedSeraModify>(chart, func, projection_no);
                AD_PRINT_L(report, [&selected]() { return selected->report("{ag_sr} {no0:{num_digits}d} {name_full_passage}\n"); });
                return selected;
            },                                                        //
            "predicate"_a, "projection_no"_a = 0, "report"_a = false, //
            py::doc("Passed predicate (function with one arg: SelectionDataSerum object)\n"
                    "is called for each serum, selects just sera for which predicate\n"
                    "returns True, returns SelectedSera object.")) //
        .def(
            "select_sera_by_aa", //
            [](std::shared_ptr<ChartModify> chart, const std::vector<std::string>& criteria, bool report) {
                auto selected = std::make_shared<SelectedSeraModify>(chart);
                acmacs::seqdb::populate(*chart);
                acmacs_py::select_by_aa(selected->indexes, *chart->sera(), criteria);
                AD_PRINT_L(report, [&selected]() { return selected->report("{ag_sr} {no0:{num_digits}d} {name_full_passage}\n"); });
                return selected;
            },                                                                                                         //
            "criteria"_a, "report"_a = false,                                                                          //
            py::doc("Criteria is a list of strings, e.g. [\"156K\", \"!145K\"], all criteria is the list must match")) //
        .def(
            "select_sera_by_clade", //
            [](std::shared_ptr<ChartModify> chart, const std::vector<std::string>& clades, bool report) {
                auto selected = std::make_shared<SelectedSeraModify>(chart);
                acmacs::seqdb::populate(*chart);
                const auto pred = [&clades, sera = chart->sera()](auto index) { return sera->at(index)->clades().exists_any_of(clades); };
                selected->indexes.get().erase(std::remove_if(selected->indexes.begin(), selected->indexes.end(), pred), selected->indexes.end());
                AD_PRINT_L(report, [&selected]() { return selected->report("{ag_sr} {no0:{num_digits}d} {name_full_passage}\n"); });
                return selected;
            },                                                                                            //
            "clades"_a, "report"_a = false,                                                               //
            py::doc(R"(Select sera with a clade from clades, one or more entries in clades must match)")) //
        .def(
            "select_all_sera",                                                                              //
            [](std::shared_ptr<ChartModify> chart) { return std::make_shared<SelectedSeraModify>(chart); }, //
            py::doc(R"(Selects all sera and returns SelectedSera object.)"))                                //
        .def(
            "select_no_sera",                                                                                                         //
            [](std::shared_ptr<ChartModify> chart) { return std::make_shared<SelectedSeraModify>(chart, SelectedSeraModify::None); }, //
            py::doc(R"(Selects no sera and returns SelectedSera object.)"))                                                           //

        .def("titers", &ChartModify::titers_modify_ptr, py::doc("returns Titers oject"))

        .def("column_basis", &ChartModify::column_basis, "serum_no"_a, "projection_no"_a = 0, py::doc("return column_basis for the passed serum"))
        .def(
            "column_bases", [](const ChartModify& chart, std::string_view minimum_column_basis) { return chart.column_bases(MinimumColumnBasis{minimum_column_basis})->data(); },
            "minimum_column_basis"_a, py::doc("get column bases")) //
        .def(
            "column_bases", [](ChartModify& chart, const std::vector<double>& column_bases) { chart.forced_column_bases_modify(ColumnBasesData{column_bases}); }, "column_bases"_a,
            py::doc("set forced column bases")) //

        .def("plot_spec", [](ChartModify& chart) { return PlotSpecRef{.plot_spec = chart.plot_spec_modify_ptr(), .number_of_antigens = chart.number_of_antigens()}; }) //

        .def("combine_projections", &ChartModify::combine_projections, "merge_in"_a) //

        .def(
            "remove_antigens_sera",
            [](ChartModify& chart, std::shared_ptr<SelectedAntigensModify> antigens, std::shared_ptr<SelectedSeraModify> sera, bool remove_projections) {
                if (remove_projections)
                    chart.projections_modify().remove_all();
                if (antigens && !antigens->empty())
                    chart.remove_antigens(acmacs::ReverseSortedIndexes{*antigens->indexes});
                if (sera && !sera->empty())
                    chart.remove_sera(acmacs::ReverseSortedIndexes{*sera->indexes});
            },                                                                          //
            "antigens"_a = nullptr, "sera"_a = nullptr, "remove_projections"_a = false, //
            py::doc(R"(
Usage:
    chart.remove_antigens_sera(antigens=chart.select_antigens(lambda ag: ag.lineage == "VICTORIA"), sera=chart.select_sera(lambda sr: sr.lineage == "VICTORIA"))
)"))                                                                                    //
        .def(
            "keep_antigens_sera",
            [](ChartModify& chart, std::shared_ptr<SelectedAntigensModify> antigens, std::shared_ptr<SelectedSeraModify> sera, bool remove_projections) {
                if (remove_projections)
                    chart.projections_modify().remove_all();
                if (antigens && !antigens->empty()) {
                    acmacs::ReverseSortedIndexes antigens_to_remove(chart.number_of_antigens());
                    antigens_to_remove.remove(*antigens->indexes);
                    // AD_INFO("antigens_to_remove:  {} {}", antigens_to_remove.size(), antigens_to_remove);
                    chart.remove_antigens(antigens_to_remove);
                }
                if (sera && !sera->empty()) {
                    acmacs::ReverseSortedIndexes sera_to_remove(chart.number_of_sera());
                    sera_to_remove.remove(*sera->indexes);
                    // AD_INFO("sera_to_remove:  {} {}", sera_to_remove.size(), sera_to_remove);
                    chart.remove_sera(sera_to_remove);
                }
            },                                                                          //
            "antigens"_a = nullptr, "sera"_a = nullptr, "remove_projections"_a = false, //
            py::doc(R"(
Usage:
    chart.remove_antigens_sera(antigens=chart.select_antigens(lambda ag: ag.lineage == "VICTORIA"), sera=chart.select_sera(lambda sr: sr.lineage == "VICTORIA"))
)"))                                                                                    //

        // ----------------------------------------------------------------------
        // DEPRECATED
        // ----------------------------------------------------------------------

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
            py::doc(R"(DEPRECATED, use chart.remove_antigens_sera(antigens=chart.select_antigens(...), sera=chart.select_sera(...))
Usage:
    chart.remove_antigens_sera(antigens=chart.antigen_indexes().filter_lineage("yamagata"), sera=chart.serum_indexes().filter_lineage("yamagata"))
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
            py::doc(R"(DEPRECATED, use chart.titers().modify(...)
look_for is regular expression,
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

    py::class_<acmacs::Transformation>(mdl, "Transformation")                                                           //
        .def("__str__", [](const acmacs::Transformation& transformation) { return fmt::format("{}", transformation); }) //
        ;

    py::class_<acmacs::Area>(mdl, "Area")                                                                     //
        .def("__str__", [](const acmacs::Area& transformation) { return fmt::format("{}", transformation); }) //
        .def_property_readonly("min", [](const acmacs::Area& area) { return area.min.as_vector(); })          //
        .def_property_readonly("max", [](const acmacs::Area& area) { return area.max.as_vector(); })          //
        .def("area", &acmacs::Area::area) //
        ;

    mdl.def("intersection", &acmacs::intersection, "area1"_a, "area2"_a);

    py::class_<ProjectionModify, std::shared_ptr<ProjectionModify>>(mdl, "Projection") //
        .def(
            "stress",                                                                                                                                                      //
            [](const ProjectionModify& projection, bool recalculate) { return projection.stress(recalculate ? RecalculateStress::if_necessary : RecalculateStress::no); }, //
            "recalculate"_a = false)                                                                                                                                       //
        .def(
            "relax",
            [](ProjectionModify& projection, bool rough) {
                projection.relax(acmacs::chart::optimization_options{optimization_precision{rough ? optimization_precision::rough : optimization_precision::fine}});
            },
            "rough"_a = false)                                                                                                                               //
        .def_property_readonly("no", &ProjectionModify::projection_no)                                                                                       //
        .def("transformation", py::overload_cast<>(&ProjectionModify::transformation, py::const_))                                                           //
        .def("layout", [](const ProjectionModify& proj) { return proj.layout()->as_vector_of_vectors_double(); })                                            //
        .def("transformed_layout", [](const ProjectionModify& proj) { return proj.transformed_layout()->as_vector_of_vectors_double(); })                    //
        .def("connect_all_disconnected", &ProjectionModify::connect_all_disconnected, py::doc("reconnected points still have NaN coordinates after call"))   //
        .def("reset_unmovable", &ProjectionModify::reset_unmovable, py::doc("make all unmovable points movable")) //
        .def("comment", py::overload_cast<>(&ProjectionModify::comment, py::const_))                                                                         //
        .def("comment", py::overload_cast<std::string>(&ProjectionModify::comment))                                                                          //
        ;

    // ----------------------------------------------------------------------

    py::class_<acmacs::chart::GridTest::Results>(mdl, "GridTestResults")                                  //
        .def("__str__", [](const acmacs::chart::GridTest::Results& results) { return results.report(); }) //
        .def(
            "report", [](const acmacs::chart::GridTest::Results& results, const acmacs::chart::ChartModify& chart) { return results.report(chart); }, "chart"_a) //
        .def(
            "json", [](const acmacs::chart::GridTest::Results& results, const acmacs::chart::ChartModify& chart) { return results.export_to_json(chart, 0); }, "chart"_a) //
        ;

    // ----------------------------------------------------------------------

    py::class_<PlotSpecRef>(mdl, "PlotSpec")                   //
        .def("antigen", &PlotSpecRef::antigen, "antigen_no"_a) //
        .def("serum", &PlotSpecRef::serum, "serum_no"_a)       //
        ;

    using namespace acmacs;

    py::class_<PointStyle>(mdl, "PointStyle")                                             //
        .def("shown", py::overload_cast<>(&PointStyle::shown, py::const_))                //
        .def("fill", py::overload_cast<>(&PointStyle::fill, py::const_))                  //
        .def("outline", py::overload_cast<>(&PointStyle::outline, py::const_))            //
        .def("outline_width", [](const PointStyle& ps) { return *ps.outline_width(); })   //
        .def("size", [](const PointStyle& ps) { return *ps.size(); })                     //
        .def("diameter", [](const PointStyle& ps) { return *ps.diameter(); })             //
        .def("rotation", [](const PointStyle& ps) { return *ps.rotation(); })             //
        .def("aspect", [](const PointStyle& ps) { return *ps.aspect(); })                 //
        .def("shape", [](const PointStyle& ps) { return fmt::format("{}", ps.shape()); }) //
        // .def("label", py::overload_cast<>(&PointStyle::label, py::const_)) //
        .def("label_text", py::overload_cast<>(&PointStyle::label_text, py::const_)) //
        ;
}

// ----------------------------------------------------------------------
