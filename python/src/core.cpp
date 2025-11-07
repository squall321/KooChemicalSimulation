/**
 * @file core.cpp
 * @brief Python bindings for core types
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha2
 * @date 2025-11-06
 *
 * Phase 56: Core Python Interface
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

// Core headers
#include "core/types/CommonTypes.h"
#include "core/types/PhysicalQuantity.h"
#include "utils/logger/Logger.h"
#include "utils/error/Exception.h"

namespace py = pybind11;

void bind_core(py::module& m) {
    m.doc() = "Core types and utilities";

    // ============================================
    // Vector bindings
    // ============================================
    py::class_<koo::types::Vector<double>>(m, "Vector", py::buffer_protocol())
        .def(py::init<>(), "Default constructor")
        .def(py::init<size_t>(), "Constructor with size", py::arg("size"))
        .def(py::init<size_t, double>(), "Constructor with size and value",
             py::arg("size"), py::arg("value"))

        .def("size", &koo::types::Vector<double>::size,
             "Get vector size")

        .def("resize", &koo::types::Vector<double>::resize,
             "Resize vector", py::arg("new_size"))

        .def("__len__", &koo::types::Vector<double>::size)

        .def("__getitem__", [](const koo::types::Vector<double>& v, size_t i) {
            if (i >= v.size()) {
                throw py::index_error("Index out of range");
            }
            return v[i];
        })

        .def("__setitem__", [](koo::types::Vector<double>& v, size_t i, double val) {
            if (i >= v.size()) {
                throw py::index_error("Index out of range");
            }
            v[i] = val;
        })

        .def("__repr__", [](const koo::types::Vector<double>& v) {
            std::ostringstream oss;
            oss << "<Vector size=" << v.size() << ">";
            return oss.str();
        })

        // NumPy buffer protocol
        .def_buffer([](koo::types::Vector<double>& v) -> py::buffer_info {
            return py::buffer_info(
                v.data(),                              // Pointer to data
                sizeof(double),                        // Size of one element
                py::format_descriptor<double>::format(), // Format
                1,                                     // Number of dimensions
                {v.size()},                            // Shape
                {sizeof(double)}                       // Strides
            );
        })

        // From NumPy array
        .def_static("from_array", [](py::array_t<double> arr) {
            py::buffer_info info = arr.request();
            auto v = koo::types::Vector<double>(info.shape[0]);
            std::memcpy(v.data(), info.ptr, sizeof(double) * info.shape[0]);
            return v;
        }, "Create Vector from NumPy array", py::arg("array"))

        // To NumPy array
        .def("to_array", [](koo::types::Vector<double>& v) {
            return py::array_t<double>(
                {v.size()},
                {sizeof(double)},
                v.data(),
                py::cast(v)
            );
        }, "Convert to NumPy array");

    // ============================================
    // Logger bindings
    // ============================================
    py::enum_<koo::utils::LogLevel>(m, "LogLevel")
        .value("DEBUG", koo::utils::LogLevel::DEBUG)
        .value("INFO", koo::utils::LogLevel::INFO)
        .value("WARNING", koo::utils::LogLevel::WARNING)
        .value("ERROR", koo::utils::LogLevel::ERROR)
        .export_values();

    py::class_<koo::utils::Logger>(m, "Logger")
        .def_static("get_instance", &koo::utils::Logger::getInstance,
                    py::return_value_policy::reference,
                    "Get logger singleton instance")

        .def("set_level", &koo::utils::Logger::setLevel,
             "Set logging level", py::arg("level"))

        .def("log", [](koo::utils::Logger& logger,
                      koo::utils::LogLevel level,
                      const std::string& message) {
            logger.log(level, message);
        }, "Log a message", py::arg("level"), py::arg("message"))

        .def("debug", [](koo::utils::Logger& logger, const std::string& msg) {
            logger.log(koo::utils::LogLevel::DEBUG, msg);
        }, "Log debug message", py::arg("message"))

        .def("info", [](koo::utils::Logger& logger, const std::string& msg) {
            logger.log(koo::utils::LogLevel::INFO, msg);
        }, "Log info message", py::arg("message"))

        .def("warning", [](koo::utils::Logger& logger, const std::string& msg) {
            logger.log(koo::utils::LogLevel::WARNING, msg);
        }, "Log warning message", py::arg("message"))

        .def("error", [](koo::utils::Logger& logger, const std::string& msg) {
            logger.log(koo::utils::LogLevel::ERROR, msg);
        }, "Log error message", py::arg("message"));

    // ============================================
    // Exception bindings
    // ============================================
    py::register_exception<koo::utils::KooException>(m, "KooException");

    // ============================================
    // Utility functions
    // ============================================
    m.def("version", []() { return std::string("6.0.0-alpha2"); },
          "Get library version");
}
