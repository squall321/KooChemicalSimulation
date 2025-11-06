/**
 * @file ParallelIO.h
 * @brief Parallel I/O operations
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-beta
 * @date 2025-11-06
 *
 * Phase 44: Parallel I/O
 *
 * Efficient parallel file I/O with:
 * - Collective writes
 * - Domain-decomposed output
 * - Parallel VTK/HDF5 support (when available)
 * - Checkpoint/restart capabilities
 */

#ifndef KOO_PARALLEL_IO_PARALLEL_IO_H
#define KOO_PARALLEL_IO_PARALLEL_IO_H

#include "parallel/mpi/MPIWrapper.h"
#include "parallel/domain/DomainDecomposition.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace koo {
namespace parallel {
namespace io {

// ============================================================================
// Parallel File Writer
// ============================================================================

/**
 * @brief Parallel file writer
 *
 * Coordinates writing from multiple processes
 */
class ParallelWriter {
public:
    /**
     * @brief Constructor
     * @param comm MPI communicator
     */
    explicit ParallelWriter(const mpi::MPIComm& comm) : comm_(comm) {}

    /**
     * @brief Write local data to separate files (one per process)
     *
     * Simple approach: each process writes its own file
     */
    template<typename T>
    bool writeDistributed(const std::string& baseFilename,
                         const std::vector<T>& localData) const {
        std::string filename = baseFilename + "_rank" +
                              std::to_string(comm_.getRank()) + ".dat";

        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;

        file.write(reinterpret_cast<const char*>(localData.data()),
                  localData.size() * sizeof(T));

        return file.good();
    }

    /**
     * @brief Gather all data to root and write single file
     *
     * @param filename Output filename
     * @param localData Local data on each process
     * @param root Root process that writes
     */
    template<typename T>
    bool writeGathered(const std::string& filename,
                      const std::vector<T>& localData,
                      int root = 0) const {
        int localSize = static_cast<int>(localData.size());

        // Gather sizes from all processes
        std::vector<int> allSizes(comm_.getSize());
        comm_.allGather(&localSize, 1, allSizes.data(), 1);

        // Compute total size and offsets
        int totalSize = 0;
        std::vector<int> offsets(comm_.getSize());
        for (int i = 0; i < comm_.getSize(); ++i) {
            offsets[i] = totalSize;
            totalSize += allSizes[i];
        }

        // Gather data to root
        std::vector<T> globalData;
        if (comm_.getRank() == root) {
            globalData.resize(totalSize);
        }

        for (int i = 0; i < comm_.getSize(); ++i) {
            if (comm_.getRank() == i) {
                if (i == root) {
                    std::copy(localData.begin(), localData.end(),
                            globalData.begin() + offsets[i]);
                } else {
                    comm_.send(localData.data(), localSize, root, 0);
                }
            } else if (comm_.getRank() == root) {
                comm_.recv(globalData.data() + offsets[i], allSizes[i], i, 0);
            }
        }

        // Root writes file
        if (comm_.getRank() == root) {
            std::ofstream file(filename, std::ios::binary);
            if (!file.is_open()) return false;

            file.write(reinterpret_cast<const char*>(globalData.data()),
                      globalData.size() * sizeof(T));

            return file.good();
        }

        return true;
    }

    /**
     * @brief Write parallel VTK file (PVTU format)
     *
     * Creates a master .pvtu file and separate .vtu files per process
     */
    bool writeParallelVTK(const std::string& baseFilename,
                         const std::vector<double>& coordinates,
                         const std::vector<double>& fieldData,
                         const std::string& fieldName) const {
        int rank = comm_.getRank();
        int nprocs = comm_.getSize();

        // Each process writes its own VTU file
        std::ostringstream vtuFilename;
        vtuFilename << baseFilename << "_" << rank << ".vtu";

        std::ofstream vtu(vtuFilename.str());
        if (!vtu.is_open()) return false;

        int nPoints = static_cast<int>(coordinates.size() / 3);

        vtu << "<?xml version=\"1.0\"?>\n";
        vtu << "<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\">\n";
        vtu << "  <UnstructuredGrid>\n";
        vtu << "    <Piece NumberOfPoints=\"" << nPoints << "\" NumberOfCells=\"0\">\n";

        // Points
        vtu << "      <Points>\n";
        vtu << "        <DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\"ascii\">\n";
        for (int i = 0; i < nPoints; ++i) {
            vtu << "          " << coordinates[3*i] << " "
                << coordinates[3*i+1] << " " << coordinates[3*i+2] << "\n";
        }
        vtu << "        </DataArray>\n";
        vtu << "      </Points>\n";

        // Field data
        vtu << "      <PointData Scalars=\"" << fieldName << "\">\n";
        vtu << "        <DataArray type=\"Float64\" Name=\"" << fieldName
            << "\" format=\"ascii\">\n";
        for (size_t i = 0; i < fieldData.size(); ++i) {
            vtu << "          " << fieldData[i] << "\n";
        }
        vtu << "        </DataArray>\n";
        vtu << "      </PointData>\n";

        vtu << "      <Cells>\n";
        vtu << "        <DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\"></DataArray>\n";
        vtu << "        <DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\"></DataArray>\n";
        vtu << "        <DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\"></DataArray>\n";
        vtu << "      </Cells>\n";

        vtu << "    </Piece>\n";
        vtu << "  </UnstructuredGrid>\n";
        vtu << "</VTKFile>\n";

        vtu.close();

        // Root writes master PVTU file
        if (rank == 0) {
            std::ofstream pvtu(baseFilename + ".pvtu");
            if (!pvtu.is_open()) return false;

            pvtu << "<?xml version=\"1.0\"?>\n";
            pvtu << "<VTKFile type=\"PUnstructuredGrid\" version=\"0.1\">\n";
            pvtu << "  <PUnstructuredGrid GhostLevel=\"0\">\n";

            pvtu << "    <PPoints>\n";
            pvtu << "      <PDataArray type=\"Float64\" NumberOfComponents=\"3\"/>\n";
            pvtu << "    </PPoints>\n";

            pvtu << "    <PPointData Scalars=\"" << fieldName << "\">\n";
            pvtu << "      <PDataArray type=\"Float64\" Name=\"" << fieldName << "\"/>\n";
            pvtu << "    </PPointData>\n";

            for (int i = 0; i < nprocs; ++i) {
                pvtu << "    <Piece Source=\"" << baseFilename << "_" << i << ".vtu\"/>\n";
            }

            pvtu << "  </PUnstructuredGrid>\n";
            pvtu << "</VTKFile>\n";

            pvtu.close();
        }

        return true;
    }

private:
    const mpi::MPIComm& comm_;
};

// ============================================================================
// Checkpoint Manager
// ============================================================================

/**
 * @brief Manages checkpointing for restart
 */
class CheckpointManager {
public:
    /**
     * @brief Constructor
     * @param comm MPI communicator
     * @param checkpointDir Directory for checkpoint files
     */
    CheckpointManager(const mpi::MPIComm& comm, const std::string& checkpointDir)
        : comm_(comm), checkpointDir_(checkpointDir), writer_(comm) {}

    /**
     * @brief Write checkpoint
     *
     * @param step Time step number
     * @param time Current simulation time
     * @param data State vector
     */
    template<typename T>
    bool writeCheckpoint(int step, double time, const std::vector<T>& data) {
        std::ostringstream filename;
        filename << checkpointDir_ << "/checkpoint_step" << step;

        // Write metadata (root only)
        if (comm_.isRoot()) {
            std::ofstream meta(filename.str() + ".meta");
            meta << "step " << step << "\n";
            meta << "time " << time << "\n";
            meta << "nprocs " << comm_.getSize() << "\n";
        }

        // Each process writes its data
        return writer_.writeDistributed(filename.str(), data);
    }

    /**
     * @brief Read checkpoint
     */
    template<typename T>
    bool readCheckpoint(int step, double& time, std::vector<T>& data) {
        std::ostringstream filename;
        filename << checkpointDir_ << "/checkpoint_step" << step;

        // Read metadata
        if (comm_.isRoot()) {
            std::ifstream meta(filename.str() + ".meta");
            if (!meta.is_open()) return false;

            std::string key;
            meta >> key >> step;
            meta >> key >> time;
        }

        // Broadcast metadata
        comm_.broadcast(&time, 1, 0);

        // Each process reads its data
        std::string dataFile = filename.str() + "_rank" +
                              std::to_string(comm_.getRank()) + ".dat";

        std::ifstream file(dataFile, std::ios::binary);
        if (!file.is_open()) return false;

        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        data.resize(fileSize / sizeof(T));
        file.read(reinterpret_cast<char*>(data.data()), fileSize);

        return file.good();
    }

private:
    const mpi::MPIComm& comm_;
    std::string checkpointDir_;
    ParallelWriter writer_;
};

} // namespace io
} // namespace parallel
} // namespace koo

#endif // KOO_PARALLEL_IO_PARALLEL_IO_H
