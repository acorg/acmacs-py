#include "acmacs-base/date.hh"
#include "acmacs-whocc-data/labs.hh"
#include "seqdb-3/seqdb.hh"
#include "acmacs-py/py.hh"

namespace acmacs_py
{
    inline static std::string fix_date(std::string_view source)
    {
        return source.empty() ? std::string{} : date::display(date::from_string(source, date::allow_incomplete::yes), date::allow_incomplete::yes);
    };

     inline static acmacs::seqdb::subset::sorting sorting_order(const acmacs::lowercase& desc)
     {
            if (desc == acmacs::lowercase{"none"})
                return acmacs::seqdb::subset::sorting::none;
            if (desc == acmacs::lowercase{"name"})
                return acmacs::seqdb::subset::sorting::name_asc;
            if (desc == acmacs::lowercase{"-name"})
                return acmacs::seqdb::subset::sorting::name_desc;
            if (desc == acmacs::lowercase{"date"})
                return acmacs::seqdb::subset::sorting::date_asc;
            if (desc == acmacs::lowercase{"-date"})
                return acmacs::seqdb::subset::sorting::date_desc;
            AD_WARNING("unrecognized soriting: {}", desc);
            return acmacs::seqdb::subset::sorting::name_asc;
     }

} // namespace acmacs_py

// ----------------------------------------------------------------------

void acmacs_py::seqdb(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::seqdb;

    py::class_<Seqdb>(mdl, "Seqdb") //
        .def(
            "all", [](const Seqdb& seqdb, bool with_issues) { return seqdb.all().with_issues(with_issues); }, "with_issues"_a = false,
            py::doc("issues: not aligned, having insertions, too short, garbage at the beginning or end"))                                   //
        .def("select_by_seq_id", py::overload_cast<std::string_view>(&Seqdb::select_by_seq_id, py::const_), "seq_id"_a)                      //
        .def("select_by_seq_id", py::overload_cast<const std::vector<std::string_view>&>(&Seqdb::select_by_seq_id, py::const_), "seq_ids"_a) //
        .def("select_by_name", py::overload_cast<std::string_view>(&Seqdb::select_by_name, py::const_), "name"_a)                            //
        .def("select_by_name", py::overload_cast<const std::vector<std::string_view>&>(&Seqdb::select_by_name, py::const_), "names"_a)       //
        .def("select_by_regex", py::overload_cast<std::string_view>(&Seqdb::select_by_regex, py::const_), "regex"_a)                         //
        .def(
            "select_by_lab_ids", [](const Seqdb& seqdb, const std::vector<std::string>& lab_ids) { return seqdb.select_by_lab_ids(acmacs::chart::LabIds{lab_ids}); }, "lab_ids"_a) //
        ;

    mdl.def("seqdb", &Seqdb::get, py::return_value_policy::reference);

    py::class_<subset>(mdl, "Seqdb_subset")                            //
        .def("size", &subset::size)                                    //
        .def("__len__", &subset::size)                                 //
        .def("__getitem__", &subset::operator[])                       //
        .def("__bool__", [](const subset& ss) { return !ss.empty(); }) //
        .def(
            "__iter__", [](subset& ss) { return py::make_iterator(ss.begin(), ss.end()); }, py::keep_alive<0, 1>()) //
        .def("clone",
             [](const subset& ss) {
                 return subset{ss.begin(), ss.end()};
             }) //
        .def(
            "append",
            [](subset& ss, const subset& another) -> subset& {
                ss.append(another);
                return ss;
            },
            "another"_a) //
        .def("remove_nuc_duplicates", &subset::remove_nuc_duplicates, "remove"_a = true, "keep_all_hi_matched"_a = false,
             py::doc("duplicates are detected when seqdb is built, using the complete nuc sequence. Tree can be re-populated with duplicates removed by this call.")) //
        .def(
            "remove_nuc_duplicates_by_aligned_truncated",
            [](subset& ss, const std::function<void(subset&, ssize_t, ssize_t)>& func, size_t truncate_at) {
                auto& seqdb = acmacs::seqdb::get();
                ss.sort_by_nuc_aligned_truncated(seqdb, truncate_at);
                auto first = ss.begin();
                for (auto cur = std::next(ss.begin()); cur != ss.end(); ++cur) {
                    if (cur->nuc_aligned(seqdb, truncate_at) != first->nuc_aligned(seqdb, truncate_at)) {
                        if (std::distance(first, cur) > 1)
                            func(ss, std::distance(ss.begin(), first), std::distance(ss.begin(), cur));
                        first = cur;
                    }
                }
                if (std::distance(first, ss.end()) > 1)
                    func(ss, std::distance(ss.begin(), first), std::distance(ss.begin(), ss.end()));
                const auto before = ss.size();
                ss.remove_marked();
                AD_DEBUG("remove_nuc_duplicates_by_aligned_truncated: {}", ss.size() - before);
                return ss;
            },
            "callback"_a, "truncate_at"_a,
            py::doc("callback is called with self and two indexes: first and after_last, referring duplicating sequences. redundant sequences must be marked for removal, they will be removed from "
                    "self before method returns. "
                    "self will be sorted by truncated nuc sequences after the call. Tree can NOT be re-populated with duplicates removed by this call.")) //
        .def(
            "filter_subtype", [](subset& ss, std::string_view subtype) { return ss.subtype(acmacs::uppercase{subtype}); }, "subtype"_a,
            py::doc("B, A(H1N1), H1, A(H3N2), H3")) //
        .def(
            "filter_lineage", [](subset& ss, std::string_view lineage) { return ss.lineage(acmacs::uppercase{lineage}); }, "lineage"_a) //
        .def(
            "filter_lab", [](subset& ss, std::string_view lab) { return ss.lab(acmacs::whocc::lab_name_normalize(lab)); }, "lab"_a) //
        .def(
            "filter_whocc_lab", [](subset& ss) { return ss.whocc_lab(true); }, py::doc("keep sequences made by WHO CC labs only")) //
        .def(
            "filter_host", [](subset& ss, std::string_view host) { return ss.host(acmacs::uppercase{host}); }, "host"_a) //
        .def("filter_human", [](subset& ss) { return ss.host("HUMAN"); })                                                //
        .def(
            "filter_dates", [](subset& ss, std::string_view start, std::string_view end) { return ss.dates(fix_date(start), fix_date(end)); }, "start"_a = "", "end"_a = "") //
        .def(
            "filter_continent", [](subset& ss, std::string_view continent) { return ss.continent(acmacs::uppercase{continent}); }, "continent"_a) //
        .def(
            "filter_country", [](subset& ss, std::string_view country) { return ss.country(acmacs::uppercase{country}); }, "country"_a) //
        .def(
            "filter_clade", [](subset& ss, std::string_view clade) { return ss.clade(acmacs::seqdb::get(), acmacs::uppercase{clade}); }, "clade"_a) //
        .def(
            "filter_aa_at_pos", [](subset& ss, const std::vector<std::string>& aas_pos) { return ss.aa_at_pos(acmacs::seqdb::get(), extract_aa_at_pos1_eq_list(aas_pos)); }, "aas_pos"_a,
            py::doc("[\"193S\", \"!68X\"]")) //
        .def(
            "filter_nuc_at_pos", [](subset& ss, const std::vector<std::string>& nucs_pos) { return ss.nuc_at_pos(acmacs::seqdb::get(), extract_nuc_at_pos1_eq_list(nucs_pos)); }, "nucs_pos"_a) //
        .def(
            "filter_out_with_deletions", [](subset& ss, size_t threshold) { return ss.remove_with_deletions(acmacs::seqdb::get(), true, threshold); }, "threshold"_a,
            py::doc("remove if number of deletions >= threshold > 0")) //
        .def(
            "filter_out_with_front_back_deletions", [](subset& ss, size_t length) { return ss.remove_with_front_back_deletions(acmacs::seqdb::get(), true, length); }, "length"_a = 0) //
        .def(
            "filter_nuc_hamming_distance_mean", [](subset& ss, size_t threshold, size_t size_threshold) { return ss.nuc_hamming_distance_mean(threshold, size_threshold); }, "threshold"_a = 0,
            "size_threshold"_a = 1000) //
        .def(
            "sort", [](subset& ss, std::string_view sort_by) { return ss.sort(sorting_order(sort_by)); }, "by"_a = "none", py::doc("none, name, -name, date, -date")) //
        .def(
            "fasta",
            [](const subset& ss, bool nucs, size_t wrap_at, bool aligned, bool most_common_length, size_t length, std::string_view name_format) {
                return ss.export_sequences(acmacs::seqdb::get(), export_options{}
                                                                     .fasta(nucs)
                                                                     .wrap(wrap_at)
                                                                     .aligned(aligned ? export_options::aligned::yes : export_options::aligned::no)
                                                                     .most_common_length(most_common_length ? export_options::most_common_length::yes : export_options::most_common_length::no)
                                                                     .length(length)
                                                                     .name_format(name_format)
                                                                     .deletion_report_threshold(5));
            },
            "nucs"_a = false, "wrap_at"_a = 0, "aligned"_a = true, "most_common_length"_a = false, "length"_a = 0, "name_format"_a = "{seq_id}",
            py::doc("name_format: {seq_id} {full_name} {hi_name_or_full_name} {hi_names} {hi_name} {lineage} {name} {date} {dates} {lab_id} {passage} {clades} {lab} {country} {continent} {group_no} "
                    "{hamming_distance} {nuc_length} {aa_length} {gisaid_accession_numbers} {ncbi_accession_numbers} {aa} {aa:193} {aa:193:6} {nuc} {nuc:193} {nuc:193:6}")) //

        // .with_issues(opt.with_issues)
        // .min_aa_length(seqdb, opt.minimum_aa_length)
        // .min_nuc_length(seqdb, opt.minimum_nuc_length)
        // .multiple_dates(opt.multiple_dates)
        // .with_hi_name(opt.with_hi_name)
        // .names_matching_regex(opt.name_regex)
        // .exclude(opt.exclude)
        // .recent(opt.recent, opt.remove_nuc_duplicates ? acmacs::seqdb::subset::master_only::yes : acmacs::seqdb::subset::master_only::no)
        // .recent_matched(acmacs::string::split_into_size_t(*opt.recent_matched, ","), opt.remove_nuc_duplicates ? acmacs::seqdb::subset::master_only::yes
        // : acmacs::seqdb::subset::master_only::no) .random(opt.random)
        // // .subset_every_month(opt.subset_every_month)
        // .group_by_hamming_distance(seqdb, opt.group_by_hamming_distance, opt.output_size)
        // .subset_by_hamming_distance_random(seqdb, opt.subset_by_hamming_distance_random, opt.output_size)
        // .remove_empty(seqdb, opt.nucs)
        // .prepend(opt.prepend, seqdb)
        // .prepend(opt.base_seq_id, seqdb)
        // .report_stat(seqdb, !opt.no_stat && !opt.stat_month_region) // static_cast<bool>(opt.fasta))
        // .report_stat_month_region(opt.stat_month_region)
        // .report_aa_at(seqdb, aa_at_pos_report)
        // .export_json_sequences(opt.json, seqdb,
        //                   acmacs::seqdb::export_options{}
        //                       .fasta(opt.nucs)
        //                       .aligned(opt.not_aligned ? acmacs::seqdb::export_options::aligned::no : acmacs::seqdb::export_options::aligned::yes)
        //                       .most_common_length(opt.most_common_length ? acmacs::seqdb::export_options::most_common_length::yes :
        //                       acmacs::seqdb::export_options::most_common_length::no) .length(opt.length) .name_format(opt.name_format)
        //                       )
        // .print(seqdb, opt.name_format, print_header, opt.print /* || opt.fasta */)                       // acmacs::seqdb::v3::subset::make_name
        // .report_hamming_distance(opt.report_hamming_distance && !opt.base_seq_id->empty());

        ;

    py::class_<ref>(mdl, "Seqdb_ref")                              //
        .def("seq_id", [](const ref& rf) { return *rf.seq_id(); }) //
        .def(
            "aa_aligned", [](const ref& rf, const Seqdb& seqdb) { return *rf.aa_aligned(seqdb); }, "seqdb"_a) //
        .def(
            "nuc_aligned", [](const ref& rf, const Seqdb& seqdb) { return *rf.nuc_aligned(seqdb); }, "seqdb"_a) //
        .def("aa_aligned_length", &ref::aa_aligned_length, "seqdb"_a)                                           //
        .def("nuc_aligned_length", &ref::nuc_aligned_length, "seqdb"_a)                                         //
        .def("date", [](const ref& rf) { return rf.entry ? std::string{rf.entry->date()} : std::string{}; })    //
        .def("passage", [](const ref& rf) { return rf.seq().passage(); })                                       //
        .def("reassortant",
             [](const ref& rf) {
                 const auto& reass = rf.seq().reassortants;
                 return reass.empty() ? std::string_view{} : reass.front();
             }) //
        .def(
            "has_reassortant", [](const ref& rf, std::string_view reass) { return rf.seq().has_reassortant(reass); }, "reassortant"_a) //
        .def(
            "mark_for_removal", [](ref& rf, bool mark) { return rf.marked_for_removal = mark; }, "mark"_a = true) //
        ;

    py::class_<sequence_aligned_t>(mdl, "AlignedSequence") //
        .def(
            "__getitem__", [](const sequence_aligned_t& seq, size_t pos) { return seq.at(pos1_t{pos}); }, "pos"_a, py::doc("pos is 1-based")) //
        .def("__len__", [](const sequence_aligned_t& seq) { return *seq.size(); })                                                            //
        .def("__str__", [](const sequence_aligned_t& seq) { return *seq; })                                                                   //
        .def("__bool__", [](const sequence_aligned_t& seq) { return !seq.empty(); })                                                          //
        .def(
            "has",
            [](const sequence_aligned_t& seq, size_t pos, std::string_view aas) {
                if (aas.size() > 1 && aas[0] == '!')
                    return aas.find(seq.at(pos1_t{pos}), 1) == std::string_view::npos;
                else
                    return aas.find(seq.at(pos1_t{pos})) != std::string_view::npos;
            },
            "pos"_a, "letters"_a,
            py::doc("return if seq has any of the letters at pos. if letters starts with ! then return if none of the letters are at pos")) //
        .def(
            "matches_all",
            [](const sequence_aligned_t& seq, const std::vector<std::string>& data) {
                const auto elts = extract_aa_at_pos1_eq_list(data);
                const auto matches = [&seq](const auto& en) {
                    const auto eq = seq.at(std::get<acmacs::seqdb::pos1_t>(en)) == std::get<char>(en);
                    return std::get<bool>(en) == eq;
                };
                return std::all_of(std::begin(elts), std::end(elts), matches);
            },
            "data"_a, py::doc(R"(Returns if sequence matches all data entries, e.g. ["197N", "!199T"])")) //
        ;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
