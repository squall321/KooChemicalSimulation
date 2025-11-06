#ifndef KOO_OUTPUT_WRITER_H
#define KOO_OUTPUT_WRITER_H

#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace koo {
namespace io {

/**
 * @class CSVWriter
 * @brief Writes data in CSV format
 *
 * Phase 32: Output System
 */
class CSVWriter {
public:
    /**
     * @brief Constructor
     */
    explicit CSVWriter(const std::string& filename, char delimiter = ',')
        : filename_(filename), delimiter_(delimiter), headerWritten_(false) {}

    /**
     * @brief Open file
     */
    bool open() {
        file_.open(filename_);
        return file_.is_open();
    }

    /**
     * @brief Write header
     */
    void writeHeader(const std::vector<std::string>& columns) {
        if (!file_.is_open()) return;

        for (size_t i = 0; i < columns.size(); ++i) {
            file_ << columns[i];
            if (i < columns.size() - 1) file_ << delimiter_;
        }
        file_ << "\n";
        headerWritten_ = true;
    }

    /**
     * @brief Write row of doubles
     */
    void writeRow(const std::vector<double>& values) {
        if (!file_.is_open()) return;

        file_ << std::scientific << std::setprecision(10);
        for (size_t i = 0; i < values.size(); ++i) {
            file_ << values[i];
            if (i < values.size() - 1) file_ << delimiter_;
        }
        file_ << "\n";
    }

    /**
     * @brief Write row of mixed types
     */
    void writeRow(const std::vector<std::string>& values) {
        if (!file_.is_open()) return;

        for (size_t i = 0; i < values.size(); ++i) {
            file_ << values[i];
            if (i < values.size() - 1) file_ << delimiter_;
        }
        file_ << "\n";
    }

    /**
     * @brief Close file
     */
    void close() {
        if (file_.is_open()) {
            file_.close();
        }
    }

    ~CSVWriter() { close(); }

private:
    std::string filename_;
    char delimiter_;
    std::ofstream file_;
    bool headerWritten_;
};

/**
 * @class DataExporter
 * @brief Exports simulation data
 *
 * Phase 32: Output System
 */
class DataExporter {
public:
    /**
     * @brief Export field data to CSV
     */
    static bool exportFieldCSV(const std::string& filename,
                              const std::vector<double>& x,
                              const std::vector<double>& values,
                              const std::string& fieldName = "value") {
        if (x.size() != values.size()) return false;

        CSVWriter writer(filename);
        if (!writer.open()) return false;

        writer.writeHeader({"x", fieldName});

        for (size_t i = 0; i < x.size(); ++i) {
            writer.writeRow({x[i], values[i]});
        }

        writer.close();
        return true;
    }

    /**
     * @brief Export 2D field data
     */
    static bool export2DFieldCSV(const std::string& filename,
                                const std::vector<double>& x,
                                const std::vector<double>& y,
                                const std::vector<std::vector<double>>& values) {
        CSVWriter writer(filename);
        if (!writer.open()) return false;

        // Write header: x, y, value
        writer.writeHeader({"x", "y", "value"});

        // Write data
        for (size_t i = 0; i < x.size(); ++i) {
            for (size_t j = 0; j < y.size(); ++j) {
                writer.writeRow({x[i], y[j], values[i][j]});
            }
        }

        writer.close();
        return true;
    }

    /**
     * @brief Export time series
     */
    static bool exportTimeSeries(const std::string& filename,
                                const std::vector<double>& times,
                                const std::vector<std::vector<double>>& data,
                                const std::vector<std::string>& varNames) {
        if (times.size() != data.size()) return false;

        CSVWriter writer(filename);
        if (!writer.open()) return false;

        // Header
        std::vector<std::string> header = {"time"};
        header.insert(header.end(), varNames.begin(), varNames.end());
        writer.writeHeader(header);

        // Data
        for (size_t i = 0; i < times.size(); ++i) {
            std::vector<double> row = {times[i]};
            row.insert(row.end(), data[i].begin(), data[i].end());
            writer.writeRow(row);
        }

        writer.close();
        return true;
    }
};

} // namespace io
} // namespace koo

#endif // KOO_OUTPUT_WRITER_H
