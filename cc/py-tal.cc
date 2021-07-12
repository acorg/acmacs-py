#include "acmacs-base/quicklook.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/settings.hh"
#include "acmacs-py/py.hh"

// ----------------------------------------------------------------------

namespace acmacs_py
{
}


// ----------------------------------------------------------------------

void acmacs_py::tal(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::tal;
    using namespace std::string_view_literals;

    py::class_<Tal>(mdl, "Tal") //
        .def(py::init([](py::object tree_file) {
                 Tal* tal = new Tal{};
                 Settings* settings = new Settings{*tal}; // settings must be alive during tal lifetime
                 settings->load_from_conf({"tal.json"sv, "clades.json"sv, "vaccines.json"sv});
                 tal->import_tree(std::string{py::str(tree_file)});
                 settings->apply("tal-default"sv);
                 return tal;
             }),
             "tree"_a) //
        .def(
            "draw",
            [](Tal& tal, py::object output, bool open) {
                tal.prepare();
                const ExportOptions export_options{.add_aa_substitution_labels = false}; // true - export subst labels into newick
                const std::string output_s{py::str(output)};
                tal.export_tree(output_s, export_options);
                if (open)
                    acmacs::open_or_quicklook(true, false, output_s, 2);
            },
            "output"_a, "open"_a = true) //
        ;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
