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
                 settings->update_env();
                 settings->apply("tal-default"sv);
                 return tal;
             }),
             "tree"_a) // , "format"_a = "", py::doc(R"(format: "" (autodetect), "newick")"))                                                                    //
        .def("tree", py::overload_cast<>(&Tal::tree), py::return_value_policy::reference) //
        .def("prepare", &Tal::prepare)                                                    //
        .def(
            "draw",
            [](Tal& tal, py::object output, bool open) {
                const ExportOptions export_options{.add_aa_substitution_labels = false}; // true - export subst labels into newick
                const std::string output_s{py::str(output)};
                tal.export_tree(output_s, export_options);
                if (open)
                    acmacs::open_or_quicklook(true, false, output_s, 2);
            },
            "output"_a, "open"_a = true) //
        ;

    py::class_<Tree>(mdl, "Tree")                                                          //
        .def("cumulative_calculate", &Tree::cumulative_calculate, "recalculate"_a = false) //
        .def("closest_leaf_subtree_size", &Tree::closest_leaf_subtree_size, "min_subtree_size"_a = 2,
             py::doc("Intermediate node's closest leaf and its subtree size, sorted by subtree size descending. The same closest leaf may be referenced by different intermediate nodes")) //
        ;

    py::class_<NodeSet>(mdl, "NodeSet") //
        .def("size", &NodeSet::size)    //
        .def("__len__", &NodeSet::size) //
        .def(
            "__getitem__", [](NodeSet& nodes, size_t index) { return nodes[index]; }, py::return_value_policy::reference) //
        // .def(
        //     "__getitem__", [](NodeSet& nodes, ssize_t index) { return index >= 0 ? nodes[static_cast<size_t>(index)] : nodes[nodes.size() - static_cast<size_t>(-index)]; },
        //     py::return_value_policy::reference)                               //
        .def("__getitem__",
             [](const NodeSet& nodes, py::slice slice) -> NodeSet* { // ~/AD/build/acmacs-build/build/pybind11-2.6.1/tests/test_sequences_and_iterators.cpp
                 size_t start, stop, step, slicelength;
                 if (!slice.compute(nodes.size(), &start, &stop, &step, &slicelength))
                     throw py::error_already_set();
                 auto* subset = new NodeSet(slicelength);
                 for (size_t i = 0; i < slicelength; ++i) {
                     (*subset)[i] = nodes[start];
                     start += step;
                 }
                 return subset;
             })
        .def("__bool__", [](const NodeSet& nodes) { return !nodes.empty(); }) //
        .def(
            "__iter__", [](NodeSet& nodes) { return py::make_iterator(nodes.begin(), nodes.end()); }, py::keep_alive<0, 1>()) //

        ;

    py::class_<Node>(mdl, "Node")                                                                           //
        .def_property_readonly("seq_id", [](const Node& node) { return *node.seq_id; })                     //
        .def_property_readonly("node_id", [](const Node& node) { return fmt::format("{}", node.node_id); }) //
        .def("number_leaves_in_subtree", &Node::number_leaves_in_subtree)                                   //
        .def(
            "closest_leaf", [](const Node& node) { return node.closest_leaves[0]; }, py::return_value_policy::reference)           //
        .def_property_readonly("edge_length", [](const Node& node) { return node.edge_length.as_number(); })                       //
        .def_property_readonly("cumulative_edge_length", [](const Node& node) { return node.cumulative_edge_length.as_number(); }) //
        ;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
