#ifndef KOO_VTK_WRITER_H
#define KOO_VTK_WRITER_H

#include <string>
#include <vector>
#include <fstream>
#include <iomanip>

namespace koo {
namespace io {

/**
 * @class VTKWriter
 * @brief Writes data in VTK legacy format for visualization
 *
 * Phase 33: VTK Output
 *
 * VTK (Visualization Toolkit) format for ParaView/VisIt
 */
class VTKWriter {
public:
    /**
     * @brief Write 1D structured grid
     */
    static bool write1DStructuredGrid(const std::string& filename,
                                     const std::vector<double>& x,
                                     const std::vector<double>& values,
                                     const std::string& fieldName = "scalar") {
        std::ofstream file(filename);
        if (!file.is_open()) return false;

        // Header
        file << "# vtk DataFile Version 3.0\n";
        file << "1D Structured Grid\n";
        file << "ASCII\n";
        file << "DATASET STRUCTURED_GRID\n";
        file << "DIMENSIONS " << x.size() << " 1 1\n";

        // Points
        file << "POINTS " << x.size() << " double\n";
        for (size_t i = 0; i < x.size(); ++i) {
            file << std::scientific << std::setprecision(10);
            file << x[i] << " 0.0 0.0\n";
        }

        // Point data
        file << "POINT_DATA " << x.size() << "\n";
        file << "SCALARS " << fieldName << " double 1\n";
        file << "LOOKUP_TABLE default\n";
        for (double val : values) {
            file << std::scientific << std::setprecision(10);
            file << val << "\n";
        }

        file.close();
        return true;
    }

    /**
     * @brief Write 2D structured grid
     */
    static bool write2DStructuredGrid(const std::string& filename,
                                     const std::vector<double>& x,
                                     const std::vector<double>& y,
                                     const std::vector<std::vector<double>>& values,
                                     const std::string& fieldName = "scalar") {
        std::ofstream file(filename);
        if (!file.is_open()) return false;

        size_t nx = x.size();
        size_t ny = y.size();

        // Header
        file << "# vtk DataFile Version 3.0\n";
        file << "2D Structured Grid\n";
        file << "ASCII\n";
        file << "DATASET STRUCTURED_GRID\n";
        file << "DIMENSIONS " << nx << " " << ny << " 1\n";

        // Points
        file << "POINTS " << (nx * ny) << " double\n";
        file << std::scientific << std::setprecision(10);
        for (size_t j = 0; j < ny; ++j) {
            for (size_t i = 0; i < nx; ++i) {
                file << x[i] << " " << y[j] << " 0.0\n";
            }
        }

        // Point data
        file << "POINT_DATA " << (nx * ny) << "\n";
        file << "SCALARS " << fieldName << " double 1\n";
        file << "LOOKUP_TABLE default\n";
        for (size_t j = 0; j < ny; ++j) {
            for (size_t i = 0; i < nx; ++i) {
                file << values[i][j] << "\n";
            }
        }

        file.close();
        return true;
    }

    /**
     * @brief Write unstructured grid (3D points)
     */
    static bool writeUnstructuredGrid(const std::string& filename,
                                     const std::vector<double>& x,
                                     const std::vector<double>& y,
                                     const std::vector<double>& z,
                                     const std::vector<double>& values,
                                     const std::string& fieldName = "scalar") {
        if (x.size() != y.size() || x.size() != z.size() || x.size() != values.size()) {
            return false;
        }

        std::ofstream file(filename);
        if (!file.is_open()) return false;

        size_t nPoints = x.size();

        // Header
        file << "# vtk DataFile Version 3.0\n";
        file << "Unstructured Grid\n";
        file << "ASCII\n";
        file << "DATASET UNSTRUCTURED_GRID\n";

        // Points
        file << "POINTS " << nPoints << " double\n";
        file << std::scientific << std::setprecision(10);
        for (size_t i = 0; i < nPoints; ++i) {
            file << x[i] << " " << y[i] << " " << z[i] << "\n";
        }

        // Point data
        file << "POINT_DATA " << nPoints << "\n";
        file << "SCALARS " << fieldName << " double 1\n";
        file << "LOOKUP_TABLE default\n";
        for (double val : values) {
            file << val << "\n";
        }

        file.close();
        return true;
    }

    /**
     * @brief Write time series (PVD collection)
     */
    static bool writeTimeSeries(const std::string& pvdFilename,
                               const std::vector<double>& times,
                               const std::vector<std::string>& vtkFilenames) {
        if (times.size() != vtkFilenames.size()) return false;

        std::ofstream file(pvdFilename);
        if (!file.is_open()) return false;

        file << "<?xml version=\"1.0\"?>\n";
        file << "<VTKFile type=\"Collection\" version=\"0.1\">\n";
        file << "  <Collection>\n";

        for (size_t i = 0; i < times.size(); ++i) {
            file << "    <DataSet timestep=\"" << times[i]
                 << "\" file=\"" << vtkFilenames[i] << "\"/>\n";
        }

        file << "  </Collection>\n";
        file << "</VTKFile>\n";

        file.close();
        return true;
    }
};

} // namespace io
} // namespace koo

#endif // KOO_VTK_WRITER_H
