/**
 * @file chemistry.cpp
 * @brief Python bindings for chemistry types
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha4
 * @date 2025-11-07
 *
 * Phase 56: Core Python Interface
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// Chemistry headers
#include "chemistry/species/Species.h"
#include "chemistry/reaction/Reaction.h"

namespace py = pybind11;

void bind_chemistry(py::module& m) {
    m.doc() = "Chemical species and reactions";

    // ============================================
    // PhaseType enum
    // ============================================
    py::enum_<koo::chemistry::PhaseType>(m, "PhaseType")
        .value("GAS", koo::chemistry::PhaseType::GAS)
        .value("LIQUID", koo::chemistry::PhaseType::LIQUID)
        .value("SOLID", koo::chemistry::PhaseType::SOLID)
        .export_values();

    // ============================================
    // Species bindings
    // ============================================
    py::class_<koo::chemistry::Species>(m, "Species")
        .def(py::init<>(), "Default constructor")
        .def(py::init<const std::string&, const std::map<std::string, int>&, koo::chemistry::PhaseType>(),
             "Constructor with name, composition, and phase",
             py::arg("name"),
             py::arg("composition"),
             py::arg("phase") = koo::chemistry::PhaseType::GAS)

        .def_property("name",
                     &koo::chemistry::Species::getName,
                     &koo::chemistry::Species::setName,
                     "Species name")

        .def_property_readonly("molecular_weight",
                              &koo::chemistry::Species::getMolecularWeight,
                              "Molecular weight (g/mol)")

        .def("set_composition", &koo::chemistry::Species::setComposition,
             "Set elemental composition", py::arg("composition"))

        .def("get_composition", &koo::chemistry::Species::getComposition,
             "Get elemental composition")

        .def("get_phase", &koo::chemistry::Species::getPhase,
             "Get phase type")

        .def("__repr__", [](const koo::chemistry::Species& sp) {
            return "<Species '" + sp.getName() + "'>";
        });

    // ============================================
    // Reaction bindings
    // ============================================
    py::class_<koo::chemistry::Reaction>(m, "Reaction")
        .def(py::init<>(), "Default constructor")

        .def("add_reactant", &koo::chemistry::Reaction::addReactant,
             "Add reactant", py::arg("species"), py::arg("coefficient") = 1.0)

        .def("add_product", &koo::chemistry::Reaction::addProduct,
             "Add product", py::arg("species"), py::arg("coefficient") = 1.0)

        .def("get_reactants", &koo::chemistry::Reaction::getReactants,
             "Get reactant map")

        .def("get_products", &koo::chemistry::Reaction::getProducts,
             "Get product map")

        .def("is_reversible", &koo::chemistry::Reaction::isReversible,
             "Check if reaction is reversible")

        .def("set_reversible", &koo::chemistry::Reaction::setReversible,
             "Set reversibility", py::arg("reversible"))

        .def("__repr__", [](const koo::chemistry::Reaction& rxn) {
            std::ostringstream oss;
            oss << "<Reaction: ";

            // Reactants
            const auto& reactants = rxn.getReactants();
            bool first = true;
            for (const auto& [name, coeff] : reactants) {
                if (!first) oss << " + ";
                if (coeff != 1.0) oss << coeff << " ";
                oss << name;
                first = false;
            }

            oss << (rxn.isReversible() ? " <=> " : " => ");

            // Products
            const auto& products = rxn.getProducts();
            first = true;
            for (const auto& [name, coeff] : products) {
                if (!first) oss << " + ";
                if (coeff != 1.0) oss << coeff << " ";
                oss << name;
                first = false;
            }

            oss << ">";
            return oss.str();
        });

    // ============================================
    // Utility functions
    // ============================================
    m.def("create_species", [](const std::string& name) {
        std::map<std::string, int> empty_comp;
        return koo::chemistry::Species(name, empty_comp, koo::chemistry::PhaseType::GAS);
    }, "Create a species with name", py::arg("name"));
}
