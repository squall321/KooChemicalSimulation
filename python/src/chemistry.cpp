/**
 * @file chemistry.cpp
 * @brief Python bindings for chemistry types
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha2
 * @date 2025-11-06
 *
 * Phase 56: Core Python Interface
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

// Chemistry headers
#include "chemistry/species/Species.h"
#include "chemistry/reaction/Reaction.h"
#include "chemistry/kinetics/ArrheniusRate.h"

namespace py = pybind11;

void bind_chemistry(py::module& m) {
    m.doc() = "Chemical species and reactions";

    // ============================================
    // Species bindings
    // ============================================
    py::class_<koo::chemistry::Species>(m, "Species")
        .def(py::init<>(), "Default constructor")
        .def(py::init<const std::string&>(),
             "Constructor with name", py::arg("name"))

        .def_property("name",
                     &koo::chemistry::Species::getName,
                     &koo::chemistry::Species::setName,
                     "Species name")

        .def_property("molar_mass",
                     &koo::chemistry::Species::getMolarMass,
                     &koo::chemistry::Species::setMolarMass,
                     "Molar mass (kg/mol)")

        .def("set_composition", [](koo::chemistry::Species& sp,
                                   const std::map<std::string, int>& comp) {
            sp.setComposition(comp);
        }, "Set elemental composition", py::arg("composition"))

        .def("get_composition", &koo::chemistry::Species::getComposition,
             "Get elemental composition")

        .def("__repr__", [](const koo::chemistry::Species& sp) {
            return "<Species '" + sp.getName() + "'>";
        });

    // ============================================
    // Reaction bindings
    // ============================================
    py::class_<koo::chemistry::Reaction>(m, "Reaction")
        .def(py::init<>(), "Default constructor")

        .def("add_reactant", [](koo::chemistry::Reaction& rxn,
                               const std::string& name, double coeff) {
            rxn.addReactant(name, coeff);
        }, "Add reactant", py::arg("species"), py::arg("coefficient") = 1.0)

        .def("add_product", [](koo::chemistry::Reaction& rxn,
                              const std::string& name, double coeff) {
            rxn.addProduct(name, coeff);
        }, "Add product", py::arg("species"), py::arg("coefficient") = 1.0)

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
            oss << "<Reaction";

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
    // ArrheniusRate bindings
    // ============================================
    py::class_<koo::chemistry::ArrheniusRate>(m, "ArrheniusRate")
        .def(py::init<>(), "Default constructor")
        .def(py::init<double, double, double>(),
             "Constructor with parameters",
             py::arg("A"), py::arg("beta") = 0.0, py::arg("Ea") = 0.0)

        .def_property("A",
                     &koo::chemistry::ArrheniusRate::getA,
                     &koo::chemistry::ArrheniusRate::setA,
                     "Pre-exponential factor")

        .def_property("beta",
                     &koo::chemistry::ArrheniusRate::getBeta,
                     &koo::chemistry::ArrheniusRate::setBeta,
                     "Temperature exponent")

        .def_property("Ea",
                     &koo::chemistry::ArrheniusRate::getEa,
                     &koo::chemistry::ArrheniusRate::setEa,
                     "Activation energy (J/mol)")

        .def("evaluate", &koo::chemistry::ArrheniusRate::evaluate,
             "Evaluate rate constant at temperature T",
             py::arg("T"))

        .def("__call__", &koo::chemistry::ArrheniusRate::evaluate,
             "Evaluate rate constant at temperature T",
             py::arg("T"))

        .def("__repr__", [](const koo::chemistry::ArrheniusRate& rate) {
            std::ostringstream oss;
            oss << "<ArrheniusRate A=" << rate.getA()
                << " beta=" << rate.getBeta()
                << " Ea=" << rate.getEa() << ">";
            return oss.str();
        });

    // ============================================
    // Utility functions
    // ============================================
    m.def("create_species_list", [](const std::vector<std::string>& names) {
        std::vector<koo::chemistry::Species> species_list;
        for (const auto& name : names) {
            species_list.emplace_back(name);
        }
        return species_list;
    }, "Create list of species from names", py::arg("names"));

    m.def("parse_reaction", [](const std::string& rxn_str) {
        // Simple reaction parser (placeholder)
        // Format: "A + B => C + D" or "A + B <=> C + D"
        koo::chemistry::Reaction rxn;

        // Find arrow
        size_t arrow_pos = rxn_str.find("=>");
        bool reversible = false;

        if (arrow_pos == std::string::npos) {
            arrow_pos = rxn_str.find("<=>");
            reversible = true;
        }

        if (arrow_pos == std::string::npos) {
            throw std::runtime_error("Invalid reaction string");
        }

        rxn.setReversible(reversible);

        // Parse reactants
        std::string reactants = rxn_str.substr(0, arrow_pos);
        size_t arrow_len = reversible ? 3 : 2;
        std::string products = rxn_str.substr(arrow_pos + arrow_len);

        // Simple split by '+'
        auto split = [](const std::string& str) {
            std::vector<std::string> tokens;
            size_t start = 0, end = 0;
            while ((end = str.find('+', start)) != std::string::npos) {
                tokens.push_back(str.substr(start, end - start));
                start = end + 1;
            }
            tokens.push_back(str.substr(start));
            return tokens;
        };

        for (const auto& token : split(reactants)) {
            std::string name = token;
            // Trim whitespace
            name.erase(0, name.find_first_not_of(" \t"));
            name.erase(name.find_last_not_of(" \t") + 1);
            if (!name.empty()) {
                rxn.addReactant(name, 1.0);
            }
        }

        for (const auto& token : split(products)) {
            std::string name = token;
            name.erase(0, name.find_first_not_of(" \t"));
            name.erase(name.find_last_not_of(" \t") + 1);
            if (!name.empty()) {
                rxn.addProduct(name, 1.0);
            }
        }

        return rxn;
    }, "Parse reaction from string", py::arg("reaction_string"));
}
