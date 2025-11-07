/**
 * @file mesh.cpp
 * @brief Python bindings for mesh types
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha4
 * @date 2025-11-07
 *
 * Phase 56: Core Python Interface
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// Mesh headers
#include "mesh/core/Node.h"
#include "mesh/core/Element.h"
#include "mesh/core/MeshData.h"
#include "core/interfaces/IMesh.h"

namespace py = pybind11;

void bind_mesh(py::module& m) {
    m.doc() = "Mesh management and operations";

    // ============================================
    // ElementType enum
    // ============================================
    py::enum_<koo::core::ElementType>(m, "ElementType")
        .value("VERTEX", koo::core::ElementType::VERTEX)
        .value("LINE", koo::core::ElementType::LINE)
        .value("TRIANGLE", koo::core::ElementType::TRIANGLE)
        .value("QUADRILATERAL", koo::core::ElementType::QUADRILATERAL)
        .value("TETRAHEDRON", koo::core::ElementType::TETRAHEDRON)
        .value("HEXAHEDRON", koo::core::ElementType::HEXAHEDRON)
        .value("PRISM", koo::core::ElementType::PRISM)
        .value("PYRAMID", koo::core::ElementType::PYRAMID)
        .export_values();

    // ============================================
    // Node bindings
    // ============================================
    py::class_<koo::mesh::core::Node>(m, "Node")
        .def(py::init<>(), "Default constructor")
        .def(py::init<size_t, double, double, double>(),
             "Constructor with ID and coordinates",
             py::arg("id"), py::arg("x"), py::arg("y"), py::arg("z") = 0.0)

        .def_property("id",
                     &koo::mesh::core::Node::getId,
                     &koo::mesh::core::Node::setId,
                     "Node ID")

        .def_property_readonly("x",
                              &koo::mesh::core::Node::getX,
                              "X-coordinate")

        .def_property_readonly("y",
                              &koo::mesh::core::Node::getY,
                              "Y-coordinate")

        .def_property_readonly("z",
                              &koo::mesh::core::Node::getZ,
                              "Z-coordinate")

        .def("__repr__", [](const koo::mesh::core::Node& node) {
            std::ostringstream oss;
            oss << "<Node id=" << node.getId()
                << " pos=(" << node.getX() << ", " << node.getY() << ", " << node.getZ() << ")>";
            return oss.str();
        });

    // ============================================
    // Element bindings
    // ============================================
    py::class_<koo::mesh::core::Element>(m, "Element")
        .def(py::init<>(), "Default constructor")
        .def(py::init<size_t, koo::core::ElementType, const std::vector<size_t>&>(),
             "Constructor with ID, type, and node IDs",
             py::arg("id"), py::arg("type"), py::arg("node_ids"))

        .def_property("id",
                     &koo::mesh::core::Element::getId,
                     &koo::mesh::core::Element::setId,
                     "Element ID")

        .def_property_readonly("type",
                              &koo::mesh::core::Element::getType,
                              "Element type")

        .def("get_node_ids", &koo::mesh::core::Element::getNodeIds,
             "Get list of node IDs")

        .def("get_num_nodes", &koo::mesh::core::Element::getNumNodes,
             "Get number of nodes")

        .def("__repr__", [](const koo::mesh::core::Element& elem) {
            std::ostringstream oss;
            oss << "<Element id=" << elem.getId()
                << " type=" << static_cast<int>(elem.getType())
                << " nnodes=" << elem.getNumNodes() << ">";
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

        .def("get_node", py::overload_cast<size_t>(
                &koo::mesh::core::MeshData::getNode),
             "Get node by ID", py::arg("id"),
             py::return_value_policy::reference_internal)

        .def("get_element", py::overload_cast<size_t>(
                &koo::mesh::core::MeshData::getElement),
             "Get element by ID", py::arg("id"),
             py::return_value_policy::reference_internal)

        .def("get_num_nodes", &koo::mesh::core::MeshData::getNumNodes,
             "Get number of nodes")

        .def("get_num_elements", &koo::mesh::core::MeshData::getNumElements,
             "Get number of elements")

        .def("clear", &koo::mesh::core::MeshData::clear,
             "Clear all mesh data")

        .def("__repr__", [](const koo::mesh::core::MeshData& mesh) {
            std::ostringstream oss;
            oss << "<MeshData nodes=" << mesh.getNumNodes()
                << " elements=" << mesh.getNumElements() << ">";
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
            size_t node_id = 0;
            for (int j = 0; j <= ny; ++j) {
                for (int i = 0; i <= nx; ++i) {
                    double x = x0 + i * dx;
                    double y = y0 + j * dy;
                    mesh->addNode(koo::mesh::core::Node(node_id++, x, y, 0.0));
                }
            }

            // Create elements (quadrilaterals)
            size_t elem_id = 0;
            for (int j = 0; j < ny; ++j) {
                for (int i = 0; i < nx; ++i) {
                    int n0 = j * (nx + 1) + i;
                    std::vector<size_t> nodeIds = {
                        static_cast<size_t>(n0),
                        static_cast<size_t>(n0 + 1),
                        static_cast<size_t>(n0 + (nx + 1) + 1),
                        static_cast<size_t>(n0 + (nx + 1))
                    };
                    koo::mesh::core::Element elem(elem_id++,
                                                   koo::core::ElementType::QUADRILATERAL,
                                                   nodeIds);
                    mesh->addElement(elem);
                }
            }

            return mesh;
        },
        "Create a rectangular mesh",
        py::arg("x0"), py::arg("y0"), py::arg("x1"), py::arg("y1"),
        py::arg("nx"), py::arg("ny"));
}
