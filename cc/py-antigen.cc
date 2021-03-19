#include "acmacs-chart-2/selected-antigens-sera.hh"
#include "acmacs-py/py.hh"
#include "acmacs-py/py-antigen-indexes.hh"

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
        .def("date", [](const Antigen& ag) { return *ag.date(); })                      //
        .def("reference", &Antigen::reference)                                          //
        .def("lab_ids", [](const Antigen& ag) { return *ag.lab_ids(); })                //
        ;

    py::class_<AntigenModify, std::shared_ptr<AntigenModify>, Antigen>(mdl, "Antigen") //
        .def("name", [](AntigenModify& ag, const std::string& new_name) { ag.name(new_name); })                                  //
        ;

    py::class_<Serum, std::shared_ptr<Serum>, detail::AntigenSerum>(mdl, "Serum")  //
        .def("serum_id", [](const Serum& sr) { return *sr.serum_id(); })           //
        .def("serum_species", [](const Serum& sr) { return *sr.serum_species(); }) //
        .def("homologous_antigens", &Serum::homologous_antigens)                   //
        ;

    // ----------------------------------------------------------------------

    py::class_<SelectedAntigensModify, std::shared_ptr<SelectedAntigensModify>>(mdl, "SelectedAntigens")
        .def("report", &SelectedAntigensModify::report, "format"_a = "{no0},")                                                                                                                  //
        .def("empty", &SelectedAntigensModify::empty)                                                                                                                                           //
        .def("size", &SelectedAntigensModify::size)                                                                                                                                             //
        .def("indexes", [](const SelectedAntigensModify& selected) { return *selected.indexes; })                                                                                               //
        .def("for_each", &SelectedAntigensModify::for_each, "modifier"_a, py::doc("modifier is called for each selected antigen, antigen fields, e.g. name, can be modified in the function.")) //
        ;

    py::class_<SelectedSeraModify, std::shared_ptr<SelectedSeraModify>>(mdl, "SelectedSera")
        .def("report", &SelectedSeraModify::report, "format"_a = "{no0},")                                                                                                              //
        .def("empty", &SelectedSeraModify::empty)                                                                                                                                       //
        .def("size", &SelectedSeraModify::size)                                                                                                                                         //
        .def("indexes", [](const SelectedSeraModify& selected) { return *selected.indexes; })                                                                                           //
        .def("for_each", &SelectedSeraModify::for_each, "modifier"_a, py::doc("modifier is called for each selected serum, serum fields, e.g. name, can be modified in the function.")) //
        ;

} // acmacs_py::antigen

// ======================================================================
// DEPRECATED
// ======================================================================

void acmacs_py::DEPRECATED::antigen_indexes(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::chart;

    py::class_<AntigenIndexes, std::shared_ptr<AntigenIndexes>>(mdl, "AntigenIndexes") //
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
