/**
 * @file DomainDecomposition.h
 * @brief Domain decomposition for parallel mesh processing
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-beta
 * @date 2025-11-06
 *
 * Phase 42: Domain Decomposition
 *
 * Partitions computational domains across MPI processes with:
 * - 1D/2D/3D domain decomposition
 * - Ghost cell management
 * - Neighbor communication patterns
 * - Load balancing support
 */

#ifndef KOO_PARALLEL_DOMAIN_DOMAIN_DECOMPOSITION_H
#define KOO_PARALLEL_DOMAIN_DOMAIN_DECOMPOSITION_H

#include "parallel/mpi/MPIWrapper.h"
#include <vector>
#include <map>
#include <set>
#include <array>
#include <algorithm>
#include <cmath>

namespace koo {
namespace parallel {
namespace domain {

// ============================================================================
// Domain Partition
// ============================================================================

/**
 * @brief Domain partition information
 *
 * Describes a subdomain assigned to a process
 */
struct DomainPartition {
    int processRank;                    ///< Process owning this partition
    std::vector<int> localNodeIDs;      ///< Local node IDs in this partition
    std::vector<int> localElementIDs;   ///< Local element IDs
    std::vector<int> ghostNodeIDs;      ///< Ghost nodes from neighbors
    std::map<int, std::vector<int>> neighborNodes; ///< Nodes shared with neighbors

    /**
     * @brief Get total number of local nodes (including ghosts)
     */
    size_t getTotalLocalNodes() const {
        return localNodeIDs.size() + ghostNodeIDs.size();
    }

    /**
     * @brief Get number of neighbors
     */
    size_t getNumNeighbors() const {
        return neighborNodes.size();
    }

    /**
     * @brief Check if a process is a neighbor
     */
    bool hasNeighbor(int rank) const {
        return neighborNodes.find(rank) != neighborNodes.end();
    }
};

// ============================================================================
// Cartesian Domain Decomposition
// ============================================================================

/**
 * @brief Cartesian domain decomposition
 *
 * Decomposes a regular grid into subdomains using Cartesian topology
 */
class CartesianDecomposition {
public:
    /**
     * @brief Constructor
     * @param nx Global grid size in x
     * @param ny Global grid size in y
     * @param nz Global grid size in z
     * @param comm MPI communicator
     */
    CartesianDecomposition(int nx, int ny, int nz, const mpi::MPIComm& comm)
        : nx_(nx), ny_(ny), nz_(nz), comm_(comm) {

        // Determine process grid dimensions
        computeProcessGrid();

        // Compute local domain bounds
        computeLocalBounds();
    }

    /**
     * @brief Get local domain bounds
     * @return Array of [xmin, xmax, ymin, ymax, zmin, zmax]
     */
    std::array<int, 6> getLocalBounds() const {
        return localBounds_;
    }

    /**
     * @brief Get process grid dimensions
     * @return Array of [npx, npy, npz]
     */
    std::array<int, 3> getProcessGrid() const {
        return {npx_, npy_, npz_};
    }

    /**
     * @brief Get process coordinates in process grid
     * @return Array of [px, py, pz]
     */
    std::array<int, 3> getProcessCoords() const {
        return processCoords_;
    }

    /**
     * @brief Get neighbor ranks
     * @return Vector of neighbor process ranks
     */
    std::vector<int> getNeighborRanks() const {
        std::vector<int> neighbors;

        // 6 face neighbors (if not at boundary)
        if (processCoords_[0] > 0) {
            neighbors.push_back(getRank(processCoords_[0] - 1, processCoords_[1], processCoords_[2]));
        }
        if (processCoords_[0] < npx_ - 1) {
            neighbors.push_back(getRank(processCoords_[0] + 1, processCoords_[1], processCoords_[2]));
        }
        if (processCoords_[1] > 0) {
            neighbors.push_back(getRank(processCoords_[0], processCoords_[1] - 1, processCoords_[2]));
        }
        if (processCoords_[1] < npy_ - 1) {
            neighbors.push_back(getRank(processCoords_[0], processCoords_[1] + 1, processCoords_[2]));
        }
        if (processCoords_[2] > 0) {
            neighbors.push_back(getRank(processCoords_[0], processCoords_[1], processCoords_[2] - 1));
        }
        if (processCoords_[2] < npz_ - 1) {
            neighbors.push_back(getRank(processCoords_[0], processCoords_[1], processCoords_[2] + 1));
        }

        return neighbors;
    }

    /**
     * @brief Check if local domain is at boundary
     * @param direction 0=x, 1=y, 2=z
     * @param side 0=min, 1=max
     */
    bool isAtBoundary(int direction, int side) const {
        if (side == 0) {
            return processCoords_[direction] == 0;
        } else {
            int np = (direction == 0) ? npx_ : ((direction == 1) ? npy_ : npz_);
            return processCoords_[direction] == np - 1;
        }
    }

    /**
     * @brief Get local grid size
     */
    std::array<int, 3> getLocalSize() const {
        return {
            localBounds_[1] - localBounds_[0] + 1,
            localBounds_[3] - localBounds_[2] + 1,
            localBounds_[5] - localBounds_[4] + 1
        };
    }

private:
    /**
     * @brief Compute process grid dimensions
     */
    void computeProcessGrid() {
        int nprocs = comm_.getSize();

        // Simple factorization for now
        // TODO: Use MPI_Dims_create for better decomposition
        npz_ = 1;
        npy_ = 1;
        npx_ = nprocs;

        // Try to balance dimensions
        if (nprocs >= 4) {
            npz_ = static_cast<int>(std::cbrt(nprocs));
            int remaining = nprocs / npz_;
            npy_ = static_cast<int>(std::sqrt(remaining));
            npx_ = nprocs / (npy_ * npz_);
        }

        // Compute process coordinates
        int rank = comm_.getRank();
        processCoords_[2] = rank / (npx_ * npy_);
        int remainder = rank % (npx_ * npy_);
        processCoords_[1] = remainder / npx_;
        processCoords_[0] = remainder % npx_;
    }

    /**
     * @brief Compute local domain bounds
     */
    void computeLocalBounds() {
        // Compute local x bounds
        int xlocal = nx_ / npx_;
        int xremainder = nx_ % npx_;
        localBounds_[0] = processCoords_[0] * xlocal +
                         std::min(processCoords_[0], xremainder);
        localBounds_[1] = localBounds_[0] + xlocal - 1 +
                         (processCoords_[0] < xremainder ? 1 : 0);

        // Compute local y bounds
        int ylocal = ny_ / npy_;
        int yremainder = ny_ % npy_;
        localBounds_[2] = processCoords_[1] * ylocal +
                         std::min(processCoords_[1], yremainder);
        localBounds_[3] = localBounds_[2] + ylocal - 1 +
                         (processCoords_[1] < yremainder ? 1 : 0);

        // Compute local z bounds
        int zlocal = nz_ / npz_;
        int zremainder = nz_ % npz_;
        localBounds_[4] = processCoords_[2] * zlocal +
                         std::min(processCoords_[2], zremainder);
        localBounds_[5] = localBounds_[4] + zlocal - 1 +
                         (processCoords_[2] < zremainder ? 1 : 0);
    }

    /**
     * @brief Get rank from process coordinates
     */
    int getRank(int px, int py, int pz) const {
        return pz * npx_ * npy_ + py * npx_ + px;
    }

    int nx_, ny_, nz_;           ///< Global grid dimensions
    int npx_, npy_, npz_;        ///< Process grid dimensions
    std::array<int, 3> processCoords_;  ///< Process coordinates
    std::array<int, 6> localBounds_;    ///< Local domain bounds
    const mpi::MPIComm& comm_;
};

// ============================================================================
// Graph-based Domain Decomposition
// ============================================================================

/**
 * @brief Graph-based domain decomposition using simple partitioning
 *
 * For unstructured meshes, partition nodes based on connectivity
 */
class GraphDecomposition {
public:
    /**
     * @brief Constructor
     * @param numNodes Total number of nodes
     * @param comm MPI communicator
     */
    GraphDecomposition(int numNodes, const mpi::MPIComm& comm)
        : numNodes_(numNodes), comm_(comm) {

        // Simple contiguous partitioning
        computeContiguousPartition();
    }

    /**
     * @brief Get partition for this process
     */
    DomainPartition getPartition() const {
        return partition_;
    }

    /**
     * @brief Get global node ID from local index
     */
    int getGlobalNodeID(int localIndex) const {
        if (localIndex < 0 || localIndex >= static_cast<int>(partition_.localNodeIDs.size())) {
            return -1;
        }
        return partition_.localNodeIDs[localIndex];
    }

    /**
     * @brief Get local node index from global ID
     */
    int getLocalNodeIndex(int globalID) const {
        auto it = std::find(partition_.localNodeIDs.begin(),
                          partition_.localNodeIDs.end(), globalID);
        if (it == partition_.localNodeIDs.end()) {
            return -1;
        }
        return static_cast<int>(std::distance(partition_.localNodeIDs.begin(), it));
    }

    /**
     * @brief Check if node is local to this process
     */
    bool isLocalNode(int globalID) const {
        return getLocalNodeIndex(globalID) >= 0;
    }

private:
    /**
     * @brief Compute contiguous partition (simple round-robin)
     */
    void computeContiguousPartition() {
        int rank = comm_.getRank();
        int nprocs = comm_.getSize();

        partition_.processRank = rank;

        // Distribute nodes contiguously
        int nodesPerProc = numNodes_ / nprocs;
        int remainder = numNodes_ % nprocs;

        int startNode = rank * nodesPerProc + std::min(rank, remainder);
        int endNode = startNode + nodesPerProc + (rank < remainder ? 1 : 0);

        for (int i = startNode; i < endNode; ++i) {
            partition_.localNodeIDs.push_back(i);
        }
    }

    int numNodes_;
    const mpi::MPIComm& comm_;
    DomainPartition partition_;
};

// ============================================================================
// Ghost Cell Manager
// ============================================================================

/**
 * @brief Manages ghost cells for domain decomposition
 *
 * Handles communication of boundary data between neighboring domains
 */
class GhostCellManager {
public:
    /**
     * @brief Constructor
     * @param partition Domain partition
     * @param comm MPI communicator
     */
    GhostCellManager(const DomainPartition& partition, const mpi::MPIComm& comm)
        : partition_(partition), comm_(comm) {}

    /**
     * @brief Exchange ghost cell data
     *
     * Communicates boundary values with neighbors
     */
    template<typename T>
    void exchangeGhostData(std::vector<T>& data) {
        // Send to neighbors
        for (const auto& pair : partition_.neighborNodes) {
            int neighborRank = pair.first;
            const auto& sharedNodes = pair.second;

            std::vector<T> sendBuf(sharedNodes.size());
            for (size_t i = 0; i < sharedNodes.size(); ++i) {
                sendBuf[i] = data[sharedNodes[i]];
            }

            comm_.send(sendBuf.data(), static_cast<int>(sendBuf.size()),
                      neighborRank, 0);
        }

        // Receive from neighbors
        for (const auto& pair : partition_.neighborNodes) {
            int neighborRank = pair.first;
            const auto& sharedNodes = pair.second;

            std::vector<T> recvBuf(sharedNodes.size());
            comm_.recv(recvBuf.data(), static_cast<int>(recvBuf.size()),
                      neighborRank, 0);

            // Update ghost values
            for (size_t i = 0; i < sharedNodes.size(); ++i) {
                // Assuming ghost nodes are stored after local nodes
                int ghostIndex = partition_.localNodeIDs.size() + i;
                if (ghostIndex < static_cast<int>(data.size())) {
                    data[ghostIndex] = recvBuf[i];
                }
            }
        }
    }

    /**
     * @brief Add ghost nodes for a neighbor
     */
    void addGhostNodes(int neighborRank, const std::vector<int>& nodeIDs) {
        partition_.neighborNodes[neighborRank] = nodeIDs;
        partition_.ghostNodeIDs.insert(partition_.ghostNodeIDs.end(),
                                      nodeIDs.begin(), nodeIDs.end());
    }

private:
    DomainPartition partition_;
    const mpi::MPIComm& comm_;
};

} // namespace domain
} // namespace parallel
} // namespace koo

#endif // KOO_PARALLEL_DOMAIN_DOMAIN_DECOMPOSITION_H
