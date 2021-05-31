#include "seqdb-3/seqdb.hh"
#include "acmacs-py/py.hh"

// ----------------------------------------------------------------------

void acmacs_py::seqdb(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::seqdb;

    py::class_<Seqdb>(mdl, "Seqdb")                                                                                                          //
        .def("all", &Seqdb::all)                                                                                                             //
        .def("select_by_seq_id", py::overload_cast<std::string_view>(&Seqdb::select_by_seq_id, py::const_), "seq_id"_a)                      //
        .def("select_by_seq_id", py::overload_cast<const std::vector<std::string_view>&>(&Seqdb::select_by_seq_id, py::const_), "seq_ids"_a) //
        .def("select_by_name", py::overload_cast<std::string_view>(&Seqdb::select_by_name, py::const_), "name"_a)                            //
        .def("select_by_name", py::overload_cast<const std::vector<std::string_view>&>(&Seqdb::select_by_name, py::const_), "names"_a)       //
        .def("select_by_regex", py::overload_cast<std::string_view>(&Seqdb::select_by_regex, py::const_), "regex"_a)                         //
        ;

    mdl.def("seqdb", &Seqdb::get, py::return_value_policy::reference);

    py::class_<subset>(mdl, "Seqdb_subset")      //
        .def("size", &subset::size)              //
        .def("__getitem__", &subset::operator[]) //
        ;

    py::class_<ref>(mdl, "Seqdb_ref")                              //
        .def("seq_id", [](const ref& rf) { return *rf.seq_id(); }) //
        .def(
            "aa_aligned", [](const ref& rf, const Seqdb& seqdb) { return *rf.aa_aligned(seqdb); }, "seqdb"_a) //
        .def(
            "nuc_aligned", [](const ref& rf, const Seqdb& seqdb) { return *rf.nuc_aligned(seqdb); }, "seqdb"_a) //
        .def("passage", [](const ref& rf) { return rf.seq().passage(); })                                       //
        .def("reassortant",
             [](const ref& rf) {
                 const auto& reass = rf.seq().reassortants;
                 return reass.empty() ? std::string_view{} : reass.front();
             }) //
        .def(
            "has_reassortant", [](const ref& rf, std::string_view reass) { return rf.seq().has_reassortant(reass); }, "reassortant"_a) //
        ;

    py::class_<sequence_aligned_t>(mdl, "AlignedSequence") //
        .def(
            "__getitem__", [](const sequence_aligned_t& seq, size_t pos) { return seq.at(pos1_t{pos}); }, "pos"_a) //
        .def("__len__", [](const sequence_aligned_t& seq) { return *seq.size(); })                                 //
        .def("__str__", [](const sequence_aligned_t& seq) { return *seq; })                                        //
        .def(
            "has", [](const sequence_aligned_t& seq, size_t pos, std::string_view aas) { return aas.find(seq.at(pos1_t{pos})) != std::string_view::npos; }, "pos"_a, "letters"_a,
            py::doc("returns if seq has any of the letters at pos")) //
        ;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
