#include "acmacs-chart-2/selected-antigens-sera.hh"
#include "acmacs-map-draw/figure.hh"
#include "acmacs-py/py.hh"
#include "acmacs-py/py-seq.hh"
#include "acmacs-py/py-antigen-indexes.hh"

namespace acmacs_py
{
    template <typename AgSr> static inline bool inside(const acmacs::chart::SelectionData<AgSr> data, const acmacs::mapi::Figure& figure)
    {
        return figure.inside(data.coord);
    }

    template <typename AgSr> static inline bool clade_any_of(const acmacs::chart::SelectionData<AgSr> data, const std::vector<std::string>& clades)
    {
        return data.ag_sr->clades().exists_any_of(clades);
    }
}

// ----------------------------------------------------------------------

void acmacs_py::antigen(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::chart;

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
        .def("__str__", [](const detail::AntigenSerum& ag_sr) { return ag_sr.format("{fields}"); })                                                 //
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

    py::class_<Antigen, std::shared_ptr<Antigen>, detail::AntigenSerum>(mdl, "AntigenRO") //
        .def("date", [](const Antigen& ag) { return *ag.date(); })                        //
        .def("reference", &Antigen::reference)                                            //
        .def("lab_ids", [](const Antigen& ag) { return *ag.lab_ids(); })                  //
        ;

    py::class_<AntigenModify, std::shared_ptr<AntigenModify>, Antigen>(mdl, "Antigen")                                                                  //
        .def("name", [](const AntigenModify& ag) { return *ag.name(); })                                                                                //
        .def("name", [](AntigenModify& ag, const std::string& new_name) { ag.name(new_name); })                                                         //
        .def("passage", [](const AntigenModify& ag) { return ag.passage(); })                                                                           //
        .def("passage", [](AntigenModify& ag, const std::string& new_passage) { ag.passage(acmacs::virus::Passage{new_passage}); })                     //
        .def("reassortant", [](const AntigenModify& ag) { return *ag.reassortant(); })                                                                  //
        .def("reassortant", [](AntigenModify& ag, const std::string& new_reassortant) { ag.reassortant(acmacs::virus::Reassortant{new_reassortant}); }) //
        .def("add_annotation", &AntigenModify::add_annotation)                                                                                          //
        .def("remove_annotation", &AntigenModify::remove_annotation)                                                                                    //
        .def("date", [](const AntigenModify& ag) { return *ag.date(); })                                                                                //
        .def("date", [](AntigenModify& ag, const std::string& new_date) { ag.date(new_date); })                                                         //
        .def("reference", [](const AntigenModify& ag) { return ag.reference(); })                                                                       //
        .def("reference", [](AntigenModify& ag, bool new_reference) { ag.reference(new_reference); })                                                   //
        .def("sequenced", [](AntigenModify& ag) { return ag.sequenced(); })                                      //
        .def("sequence_aa", [](AntigenModify& ag) { return acmacs::seqdb::sequence_aligned_t{ag.sequence_aa()}; })                                      //
        .def("sequence_aa", [](AntigenModify& ag, std::string_view sequence) { ag.sequence_aa(sequence); })                                             //
        .def("sequence_nuc", [](AntigenModify& ag) { return acmacs::seqdb::sequence_aligned_t{ag.sequence_nuc()}; })                                    //
        .def("sequence_nuc", [](AntigenModify& ag, std::string_view sequence) { ag.sequence_nuc(sequence); })                                           //
        .def("add_clade", &AntigenModify::add_clade)                                                                                                    //
        .def("clades", [](const AntigenModify& ag) { return *ag.clades(); })                                                                            //
        ;

    py::class_<Serum, std::shared_ptr<Serum>, detail::AntigenSerum>(mdl, "SerumRO") //
        .def("serum_id", [](const Serum& sr) { return *sr.serum_id(); })            //
        .def("serum_species", [](const Serum& sr) { return *sr.serum_species(); })  //
        .def("homologous_antigens", &Serum::homologous_antigens)                    //
        ;

    py::class_<SerumModify, std::shared_ptr<SerumModify>, Serum>(mdl, "Serum")                                                                        //
        .def("name", [](const SerumModify& sr) { return *sr.name(); })                                                                                //
        .def("name", [](SerumModify& sr, const std::string& new_name) { sr.name(new_name); })                                                         //
        .def("passage", [](const SerumModify& sr) { return sr.passage(); })                                                                           //
        .def("passage", [](SerumModify& sr, const std::string& new_passage) { sr.passage(acmacs::virus::Passage{new_passage}); })                     //
        .def("reassortant", [](const SerumModify& sr) { return *sr.reassortant(); })                                                                  //
        .def("reassortant", [](SerumModify& sr, const std::string& new_reassortant) { sr.reassortant(acmacs::virus::Reassortant{new_reassortant}); }) //
        .def("add_annotation", &SerumModify::add_annotation)                                                                                          //
        .def("remove_annotation", &SerumModify::remove_annotation)                                                                                    //
        .def("serum_id", [](const SerumModify& sr) { return *sr.serum_id(); })                                                                        //
        .def("serum_id", [](SerumModify& sr, const std::string& new_serum_id) { return sr.serum_id(SerumId{new_serum_id}); })                         //
        .def("serum_species", [](const SerumModify& sr) { return *sr.serum_species(); })                                                              //
        .def("serum_species", [](SerumModify& sr, const std::string& new_species) { return sr.serum_species(SerumSpecies{new_species}); })            //
        .def("sequenced", [](SerumModify& sr) { return sr.sequenced(); })                                      //
        .def("sequence_aa", [](SerumModify& sr) { return acmacs::seqdb::sequence_aligned_t{sr.sequence_aa()}; })                                      //
        .def("sequence_aa", [](SerumModify& sr, std::string_view sequence) { sr.sequence_aa(sequence); })                                             //
        .def("sequence_nuc", [](SerumModify& sr) { return acmacs::seqdb::sequence_aligned_t{sr.sequence_nuc()}; })                                    //
        .def("sequence_nuc", [](SerumModify& sr, std::string_view sequence) { sr.sequence_nuc(sequence); })                                           //
        .def("add_clade", &SerumModify::add_clade)                                                                                                    //
        .def("clades", [](const AntigenModify& ag) { return *ag.clades(); })                                                                            //
        ;

    // ----------------------------------------------------------------------

    py::class_<SelectionData<Antigen>>(mdl, "SelectionDataAntigen")                                                             //
        .def_readonly("no", &SelectionData<Antigen>::index)                                                                     //
        .def_readonly("point_no", &SelectionData<Antigen>::point_no)                                                            //
        .def_readonly("antigen", &SelectionData<Antigen>::ag_sr)                                                                //
        .def_readonly("coordinates", &SelectionData<Antigen>::coord)                                                            //
        .def_property_readonly("name", [](const SelectionData<Antigen>& data) { return *data.ag_sr->name(); })                  //
        .def_property_readonly("lineage", [](const SelectionData<Antigen>& data) { return data.ag_sr->lineage().to_string(); }) //
        .def_property_readonly("passage", [](const SelectionData<Antigen>& data) { return data.ag_sr->passage(); })             //
        .def_property_readonly("reassortant", [](const SelectionData<Antigen>& data) { return *data.ag_sr->reassortant(); })    //
        .def("inside", &inside<Antigen>, "figure"_a)                                                                            //
        .def("clade_any_of", &clade_any_of<Antigen>, "clades"_a)                                                                            //
        ;

    py::class_<SelectionData<Serum>>(mdl, "SelectionDataSerum")                                                                //
        .def_readonly("no", &SelectionData<Serum>::index)                                                                      //
        .def_readonly("point_no", &SelectionData<Serum>::point_no)                                                             //
        .def_readonly("serum", &SelectionData<Serum>::ag_sr)                                                                   //
        .def_readonly("coordinates", &SelectionData<Serum>::coord)                                                             //
        .def_property_readonly("name", [](const SelectionData<Serum>& data) { return *data.ag_sr->name(); })                   //
        .def_property_readonly("lineage", [](const SelectionData<Serum>& data) { return data.ag_sr->lineage().to_string(); })  //
        .def_property_readonly("passage", [](const SelectionData<Serum>& data) { return data.ag_sr->passage(); })              //
        .def_property_readonly("reassortant", [](const SelectionData<Serum>& data) { return *data.ag_sr->reassortant(); })     //
        .def_property_readonly("serum_id", [](const SelectionData<Serum>& data) { return *data.ag_sr->serum_id(); })           //
        .def_property_readonly("serum_species", [](const SelectionData<Serum>& data) { return *data.ag_sr->serum_species(); }) //
        .def("inside", &inside<Serum>, "figure"_a)                                                                             //
        .def("clade_any_of", &clade_any_of<Serum>, "clades"_a)                                                                            //
        ;

    // ----------------------------------------------------------------------

    py::class_<SelectedAntigensModify, std::shared_ptr<SelectedAntigensModify>>(mdl, "SelectedAntigens")
        .def(
            "deselect_by_aa",
            [](SelectedAntigensModify& selected, const std::vector<std::string>& criteria) {
                acmacs::seqdb::populate(*selected.chart);
                acmacs_py::deselect_by_aa(selected.indexes, *selected.chart->antigens(), criteria);
                return selected;
            },
            "criteria"_a, py::doc("Criteria is a list of strings, e.g. [\"156K\", \"!145K\"], all criteria is the list must match")) //
        .def(
            "exclude",
            [](SelectedAntigensModify& selected, const SelectedAntigensModify& exclude) {
                selected.exclude(exclude);
                return selected;
            },
            "exclude"_a, py::doc("Deselect antigens selected by exclusion list")) //
        .def(
            "filter_sequenced",
            [](SelectedAntigensModify& selected) {
                acmacs::seqdb::populate(*selected.chart);
                acmacs_py::deselect_not_sequenced(selected.indexes, *selected.chart->antigens());
                return selected;
            },
            py::doc("deselect not sequenced"))                                                                                         //
        .def("report", &SelectedAntigensModify::report, "format"_a = "{no0},")                                                         //
        .def("report_list", &SelectedAntigensModify::report_list, "format"_a = "{name}")                                               //
        .def("__repr__", [](const SelectedAntigensModify& selected) { return fmt::format("SelectedAntigens ({})", selected.size()); }) //
        .def("empty", &SelectedAntigensModify::empty)                                                                                  //
        .def("size", &SelectedAntigensModify::size)                                                                                    //
        .def("__len__", &SelectedAntigensModify::size)                                                                                 //
        .def("__getitem__", &SelectedAntigensModify::operator[])                                                                       //
        .def("__bool_", [](const SelectedAntigensModify& antigens) { return !antigens.empty(); })                                      //
        .def(
            "__iter__", [](SelectedAntigensModify& antigens) { return py::make_iterator(antigens.begin(), antigens.end()); }, py::keep_alive<0, 1>()) //
        .def("indexes", [](const SelectedAntigensModify& selected) { return *selected.indexes; })                                                     //
        .def("for_each", &SelectedAntigensModify::for_each, "modifier"_a,
             py::doc("modifier(ag_no, antigen) is called for each selected antigen, antigen fields, e.g. name, can be modified in the function.")) //
        ;

    py::class_<SelectedSeraModify, std::shared_ptr<SelectedSeraModify>>(mdl, "SelectedSera")
        .def(
            "deselect_by_aa",
            [](SelectedSeraModify& selected, const std::vector<std::string>& criteria) {
                acmacs::seqdb::populate(*selected.chart);
                acmacs_py::deselect_by_aa(selected.indexes, *selected.chart->sera(), criteria);
                return selected;
            },
            "criteria"_a, py::doc("Criteria is a list of strings, e.g. [\"156K\", \"!145K\"], all criteria is the list must match")) //
        .def(
            "exclude",
            [](SelectedSeraModify& selected, const SelectedSeraModify& exclude) {
                selected.exclude(exclude);
                return selected;
            },
            "exclude"_a, py::doc("Deselect sera selected by exclusion list")) //
        .def(
            "filter_sequenced",
            [](SelectedSeraModify& selected) {
                acmacs::seqdb::populate(*selected.chart);
                acmacs_py::deselect_not_sequenced(selected.indexes, *selected.chart->sera());
                return selected;
            },
            py::doc("deselect not sequenced"))                                        //
        .def("report", &SelectedSeraModify::report, "format"_a = "{no0},")            //
        .def("report_list", &SelectedSeraModify::report_list, "format"_a = "{name}")  //
        .def("__repr__", [](const SelectedSeraModify& selected) { return fmt::format("SelectedSera ({})", selected.size()); }) //
        .def("empty", &SelectedSeraModify::empty)                                     //
        .def("size", &SelectedSeraModify::size)                                       //
        .def("__len__", &SelectedSeraModify::size)                                    //
        .def("__bool_", [](const SelectedSeraModify& sera) { return !sera.empty(); }) //
        .def(
            "__iter__", [](SelectedSeraModify& sera) { return py::make_iterator(sera.begin(), sera.end()); }, py::keep_alive<0, 1>())                                                                 //
        .def("__getitem__", &SelectedSeraModify::operator[])                                                                                                                                          //
        .def("indexes", [](const SelectedSeraModify& selected) { return *selected.indexes; })                                                                                                         //
        .def("for_each", &SelectedSeraModify::for_each, "modifier"_a, py::doc("modifier(sr_no, serum) is called for each selected serum, serum fields, e.g. name, can be modified in the function.")) //
        ;

} // acmacs_py::antigen

// ======================================================================
// DEPRECATED
// ======================================================================

void acmacs_py::DEPRECATED::antigen_indexes(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::chart;

    py::class_<AntigenIndexes, std::shared_ptr<AntigenIndexes>>(mdl, "DEPRECATED_AntigenIndexes") //
        .def("__str__", [](const AntigenIndexes& indexes) { return fmt::format("DEPRECATED::AntigenIndexes({}){}", indexes.indexes.size(), indexes.indexes); })
        .def("empty", &AntigenIndexes::empty)

        .def(
            "filter_lineage",
            [](AntigenIndexes& indexes, const std::string& lineage) {
                indexes.ag_sr->filter_lineage(indexes.indexes, acmacs::chart::BLineage{lineage});
                return indexes;
            },           //
            "lineage"_a) //
        ;

    py::class_<SerumIndexes, std::shared_ptr<SerumIndexes>>(mdl, "DEPRECATED_SerumIndexes") //
        .def("__str__", [](const SerumIndexes& indexes) { return fmt::format("DEPRECATED::SerumIndexes({}){}", indexes.indexes.size(), indexes.indexes); })
        .def("empty", &SerumIndexes::empty)

        .def(
            "filter_lineage",
            [](SerumIndexes& indexes, const std::string& lineage) {
                indexes.ag_sr->filter_lineage(indexes.indexes, acmacs::chart::BLineage{lineage});
                return indexes;
            },           //
            "lineage"_a) //
        .def(
            "filter_serum_id",
            [](SerumIndexes& indexes, const std::string& serum_id) {
                indexes.ag_sr->filter_serum_id(indexes.indexes, serum_id);
                return indexes;
            },            //
            "serum_id"_a) //
        ;

} // acmacs_py::DEPRECATED::antigen_indexes(

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
