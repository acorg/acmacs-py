#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/selected-antigens-sera.hh"
#include "acmacs-chart-2/text-export.hh"
#include "acmacs-chart-2/grid-test.hh"
#include "seqdb-3/seqdb.hh"
#include "acmacs-py/py.hh"
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
            throw std::invalid_argument{fmt::format("Unrecognized clone \"type\": \"{}\"", type)};
    }

    static inline acmacs::chart::GridTest::Results grid_test1(acmacs::chart::ChartModify& chart, std::shared_ptr<acmacs::chart::SelectedAntigens> antigens,
                                                             std::shared_ptr<acmacs::chart::SelectedSera> sera, size_t projection_no, double grid_step, int threads)
    {
        acmacs::chart::GridTest test{chart, projection_no, grid_step};
        acmacs::chart::GridTest::Results results;
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
        return results;
    }

    static inline acmacs::chart::GridTest::Results grid_test2(acmacs::chart::ChartModify& chart, size_t projection_no, double grid_step, int threads)
    {
        acmacs::chart::GridTest test{chart, projection_no, grid_step};
        return test.test_all(threads);
    }

} // namespace acmacs_py

// ----------------------------------------------------------------------

void acmacs_py::chart(py::module_& mdl)
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
            [](const ChartModify& chart, const SelectedAntigens& antigens, const SelectedSera& sera, const std::string& format) {
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
            "populate_from_seqdb",                                            //
            [](ChartModify& chart) { acmacs::seqdb::get().populate(chart); }, //
            py::doc("match seqdb, set lineages and clades"))

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

        .def("grid_test", &grid_test1, "antigens"_a, "sera"_a, "projection_no"_a = 0, "grid_step"_a = 0.1, "threads"_a = 0) //
        .def("grid_test", &grid_test2, "projection_no"_a = 0, "grid_step"_a = 0.1, "threads"_a = 0) //

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
            "filename"_a, "program_name"_a = "acmacs-py")                                                                                                                         //

        .def(
            "select_antigens",                                                                                  //
            [](std::shared_ptr<ChartModify> chart) { return std::make_shared<SelectedAntigensModify>(chart); }, //
            py::doc(R"(Selects all antigens and returns SelectedAntigens object.)"))                            //
        .def(
            "select_antigens",                                                                                                                                                           //
            [](std::shared_ptr<ChartModify> chart, const std::function<bool(size_t, std::shared_ptr<Antigen>)>& func) { return std::make_shared<SelectedAntigensModify>(chart, func); }, //
            "predicate"_a,                                                                                                                                                               //
            py::doc(R"(Passed predicate (function with two args: antigen index and antigen object)
is called for each antigen, selects just antigens for which predicate
returns True, returns SelectedAntigens object.)"))                                                                                                                                       //

        .def(
            "select_sera",                                                                                  //
            [](std::shared_ptr<ChartModify> chart) { return std::make_shared<SelectedSeraModify>(chart); }, //
            py::doc(R"(Selects all sera and returns SelectedSera object.)"))                                //
        .def(
            "select_sera",                                                                                                                                                         //
            [](std::shared_ptr<ChartModify> chart, const std::function<bool(size_t, std::shared_ptr<Serum>)>& func) { return std::make_shared<SelectedSeraModify>(chart, func); }, //
            "predicate"_a,                                                                                                                                                         //
            py::doc(R"(Passed predicate (function with two args: serum index and serum object)
is called for each serum, selects just sera for which predicate
returns True, returns SelectedAntigens object.)"))                                                                                                                                 //

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

    py::class_<ProjectionModify, std::shared_ptr<ProjectionModify>>(mdl, "Projection") //
        .def(
            "stress",                                                                                                                                                      //
            [](const ProjectionModify& projection, bool recalculate) { return projection.stress(recalculate ? RecalculateStress::if_necessary : RecalculateStress::no); }, //
            "recalculate"_a = false)                                                                                                                                       //
        ;

    // ----------------------------------------------------------------------

    py::class_<acmacs::chart::GridTest::Results>(mdl, "GridTestResults") //
        .def("__str__", &acmacs::chart::GridTest::Results::report) //
        ;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
