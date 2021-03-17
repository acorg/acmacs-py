#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/common.hh"

#include "acmacs-py/py.hh"

// ----------------------------------------------------------------------

void acmacs_py::common(py::module_& mdl)
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
        .def(py::init([](const ChartModify& primary, const ChartModify& secondary, common::antigen_selector_t antigen_entry_extractor, common::serum_selector_t serum_entry_extractor,
                         const std::string& match_level) {
                 return new CommonAntigensSera(primary, secondary, antigen_entry_extractor, serum_entry_extractor, CommonAntigensSera::match_level(match_level));
             }),
             "primary"_a, "secondary"_a, "antigen_entry_extractor"_a, "serum_entry_extractor"_a, "match_level"_a = "auto",
             py::doc(R"(match_level: "strict", "relaxed", "ignored", "auto")")) //_
        .def(py::init([](const Chart& chart) { return new CommonAntigensSera(chart); }), "chart"_a,
             py::doc(R"(for procrustes between projections of the same chart)"))  //_
        .def("report", &CommonAntigensSera::report, "indent"_a = 0)               //
        .def("report_unique", &CommonAntigensSera::report_unique, "indent"_a = 0) //
        ;

    py::class_<common::AntigenEntry>(mdl, "common_AntigenEntry") //
        .def(py::init<size_t, const Antigen&>())                 //
        .def_property(
            "name", [](const common::AntigenEntry& en) { return *en.name; },
            [](common::AntigenEntry& en, const std::string& new_name) {
                en.make_orig();
                en.name = new_name;
            })                                              //
        .def("full_name", &common::AntigenEntry::full_name) //
        .def_property(
            "reassortant", [](const common::AntigenEntry& en) { return *en.reassortant; },
            [](common::AntigenEntry& en, const std::string& new_reassortant) {
                en.make_orig();
                en.reassortant = acmacs::virus::Reassortant{new_reassortant};
            }) //
        .def_property(
            "annotations", [](const common::AntigenEntry& en) { return *en.annotations; },
            [](common::AntigenEntry& en, const std::vector<std::string>& new_annotations) {
                en.make_orig();
                en.annotations = Annotations{new_annotations};
            }) //
        .def_property(
            "passage", [](const common::AntigenEntry& en) { return *en.passage; },
            [](common::AntigenEntry& en, const std::string& new_passage) {
                en.make_orig();
                en.passage = acmacs::virus::Passage{new_passage};
            }) //
        ;

    py::class_<common::SerumEntry>(mdl, "common_SerumEntry") //
        .def(py::init<size_t, const Serum&>())               //
        .def_property(
            "name", [](const common::SerumEntry& en) { return *en.name; },
            [](common::SerumEntry& en, const std::string& new_name) {
                en.make_orig();
                en.name = new_name;
            })                                            //
        .def("full_name", &common::SerumEntry::full_name) //
        .def_property(
            "reassortant", [](const common::SerumEntry& en) { return *en.reassortant; },
            [](common::SerumEntry& en, const std::string& new_reassortant) {
                en.make_orig();
                en.reassortant = acmacs::virus::Reassortant{new_reassortant};
            }) //
        .def_property(
            "annotations", [](const common::SerumEntry& en) { return *en.annotations; },
            [](common::SerumEntry& en, const std::vector<std::string>& new_annotations) {
                en.make_orig();
                en.annotations = Annotations{new_annotations};
            }) //
        .def_property(
            "serum_id", [](const common::SerumEntry& en) { return *en.serum_id; },
            [](common::SerumEntry& en, const std::string& new_serum_id) {
                en.make_orig();
                en.serum_id = SerumId{new_serum_id};
            }) //
        .def_property(
            "passage", [](const common::SerumEntry& en) { return *en.passage; },
            [](common::SerumEntry& en, const std::string& new_passage) {
                en.make_orig();
                en.passage = acmacs::virus::Passage{new_passage};
            }) //
        ;

} // acmacs_py::common

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
