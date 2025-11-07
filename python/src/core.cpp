/**
 * @file core.cpp
 * @brief Python bindings for core types
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha4
 * @date 2025-11-07
 *
 * Phase 56: Core Python Interface
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// Core headers
#include "utils/logger/Logger.h"
#include "utils/error/Exception.h"

namespace py = pybind11;

void bind_core(py::module& m) {
    m.doc() = "Core types and utilities";

    // ============================================
    // Logger bindings
    // ============================================
    py::enum_<koo::utils::logger::LogLevel>(m, "LogLevel")
        .value("TRACE", koo::utils::logger::LogLevel::TRACE)
        .value("DEBUG", koo::utils::logger::LogLevel::DEBUG)
        .value("INFO", koo::utils::logger::LogLevel::INFO)
        .value("WARNING", koo::utils::logger::LogLevel::WARNING)
        .value("ERROR", koo::utils::logger::LogLevel::ERROR)
        .value("CRITICAL", koo::utils::logger::LogLevel::CRITICAL)
        .value("OFF", koo::utils::logger::LogLevel::OFF)
        .export_values();

    py::class_<koo::utils::logger::Logger>(m, "Logger")
        .def_static("initialize", &koo::utils::logger::Logger::initialize,
                    "Initialize logger with name",
                    py::arg("name") = "KooChemicalSimulation")

        .def_static("set_level", &koo::utils::logger::Logger::setLevel,
                    "Set logging level", py::arg("level"))

        .def_static("debug", [](const std::string& msg) {
            koo::utils::logger::Logger::debug(msg);
        }, "Log debug message", py::arg("message"))

        .def_static("info", [](const std::string& msg) {
            koo::utils::logger::Logger::info(msg);
        }, "Log info message", py::arg("message"))

        .def_static("warning", [](const std::string& msg) {
            koo::utils::logger::Logger::warning(msg);
        }, "Log warning message", py::arg("message"))

        .def_static("error", [](const std::string& msg) {
            koo::utils::logger::Logger::error(msg);
        }, "Log error message", py::arg("message"));

    // ============================================
    // Exception bindings
    // ============================================
    py::register_exception<koo::utils::error::KooException>(m, "KooException");

    // ============================================
    // Utility functions
    // ============================================
    m.def("version", []() { return std::string("6.0.0-alpha4"); },
          "Get library version");
}
