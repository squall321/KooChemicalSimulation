/**
 * @file mesh.cpp
 * @brief Python bindings for mesh types
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha2
 * @date 2025-11-06
 *
 * Phase 56: Core Python Interface
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

// Mesh headers
#include "mesh/core/Node.h"
#include "mesh/core/Element.h"
#include "mesh/core/MeshData.h"

namespace py = pybind11;

void bind_mesh(py::module& m) {
    m.doc() = "Mesh management and operations";

    // ============================================
    // Node bindings
    // ============================================
    py::class_<koo::mesh::core::Node>(m, "Node")
        .def(py::init<>(), "Default constructor")
        .def(py::init<int, double, double, double>(),
             "Constructor with ID and coordinates",
             py::arg("id"), py::arg("x"), py::arg("y"), py::arg("z") = 0.0)

        .def_property("id",
                     &koo::mesh::core::Node::getId,
                     &koo::mesh::core::Node::setId,
                     "Node ID")

        .def_property_readonly("x",
                              &koo::mesh::core::Node::x,
                              "X-coordinate")

        .def_property_readonly("y",
                              &koo::mesh::core::Node::y,
                              "Y-coordinate")

        .def_property_readonly("z",
                              &koo::mesh::core::Node::z,
                              "Z-coordinate")

        .def("coords", [](const koo::mesh::core::Node& node) {
            return py::make_tuple(node.x(), node.y(), node.z());
        }, "Get coordinates as tuple")

        .def("__repr__", [](const koo::mesh::core::Node& node) {
            std::ostringstream oss;
            oss << "<Node id=" << node.getId()
                << " pos=(" << node.x() << ", " << node.y() << ", " << node.z() << ")>";
            return oss.str();
        });

    // ============================================
    // ElementType enum
    // ============================================
    py::enum_<koo::mesh::core::ElementType>(m, "ElementType")
        .value("NODE", koo::mesh::core::ElementType::NODE)
        .value("EDGE", koo::mesh::core::ElementType::EDGE)
        .value("TRIANGLE", koo::mesh::core::ElementType::TRIANGLE)
        .value("QUADRILATERAL", koo::mesh::core::ElementType::QUADRILATERAL)
        .value("TETRAHEDRON", koo::mesh::core::ElementType::TETRAHEDRON)
        .value("HEXAHEDRON", koo::mesh::core::ElementType::HEXAHEDRON)
        .export_values();

    // ============================================
    // Element bindings
    // ============================================
    py::class_<koo::mesh::core::Element>(m, "Element")
        .def(py::init<>(), "Default constructor")
        .def(py::init<int, koo::mesh::core::ElementType>(),
             "Constructor with ID and type",
             py::arg("id"), py::arg("type"))

        .def_property("id",
                     &koo::mesh::core::Element::getId,
                     &koo::mesh::core::Element::setId,
                     "Element ID")

        .def_property_readonly("type",
                              &koo::mesh::core::Element::getType,
                              "Element type")

        .def("add_node", &koo::mesh::core::Element::addNode,
             "Add node ID to element", py::arg("node_id"))

        .def("get_nodes", &koo::mesh::core::Element::getNodes,
             "Get list of node IDs")

        .def("num_nodes", &koo::mesh::core::Element::numNodes,
             "Get number of nodes")

        .def("__repr__", [](const koo::mesh::core::Element& elem) {
            std::ostringstream oss;
            oss << "<Element id=" << elem.getId()
                << " type=" << static_cast<int>(elem.getType())
                << " nnodes=" << elem.numNodes() << ">";
            return oss.str();
        });

    // ============================================
    // MeshData bindings
    // ============================================
    py::class_<koo::mesh::core::MeshData>(m, "MeshData")
        .def(py::init<>(), "Default constructor")

        .def("add_node", py::overload_cast<const koo::mesh::core::Node&>(
                &koo::mesh::core::MeshData::addNode),
             "Add node to mesh", py::arg("node"))

        .def("add_element", py::overload_cast<const koo::mesh::core::Element&>(
                &koo::mesh::core::MeshData::addElement),
             "Add element to mesh", py::arg("element"))

        .def("get_node", py::overload_cast<int>(
                &koo::mesh::core::MeshData::getNode),
             "Get node by ID", py::arg("id"),
             py::return_value_policy::reference_internal)

        .def("get_element", py::overload_cast<int>(
                &koo::mesh::core::MeshData::getElement),
             "Get element by ID", py::arg("id"),
             py::return_value_policy::reference_internal)

        .def("num_nodes", &koo::mesh::core::MeshData::numNodes,
             "Get number of nodes")

        .def("num_elements", &koo::mesh::core::MeshData::numElements,
             "Get number of elements")

        .def("clear", &koo::mesh::core::MeshData::clear,
             "Clear all mesh data")

        .def("__repr__", [](const koo::mesh::core::MeshData& mesh) {
            std::ostringstream oss;
            oss << "<MeshData nodes=" << mesh.numNodes()
                << " elements=" << mesh.numElements() << ">";
            return oss.str();
        });

    // ============================================
    // Utility functions
    // ============================================
    m.def("create_rectangular_mesh", [](
        double x0, double y0, double x1, double y1, int nx, int ny) {
            auto mesh = std::make_shared<koo::mesh::core::MeshData>();

            double dx = (x1 - x0) / nx;
            double dy = (y1 - y0) / ny;

            // Create nodes
            int node_id = 0;
            for (int j = 0; j <= ny; ++j) {
                for (int i = 0; i <= nx; ++i) {
                    double x = x0 + i * dx;
                    double y = y0 + j * dy;
                    mesh->addNode(koo::mesh::core::Node(node_id++, x, y, 0.0));
                }
            }

            // Create elements (quadrilaterals)
            int elem_id = 0;
            for (int j = 0; j < ny; ++j) {
                for (int i = 0; i < nx; ++i) {
                    koo::mesh::core::Element elem(elem_id++, koo::mesh::core::ElementType::QUADRILATERAL);
                    int n0 = j * (nx + 1) + i;
                    elem.addNode(n0);
                    elem.addNode(n0 + 1);
                    elem.addNode(n0 + (nx + 1) + 1);
                    elem.addNode(n0 + (nx + 1));
                    mesh->addElement(elem);
                }
            }

            return mesh;
        },
        "Create a rectangular mesh",
        py::arg("x0"), py::arg("y0"), py::arg("x1"), py::arg("y1"),
        py::arg("nx"), py::arg("ny"));
}
