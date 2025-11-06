/**
 * @file HybridMPI.h
 * @brief MPI + GPU hybrid parallelism
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha2
 * @date 2025-11-06
 *
 * Phase 55: GPU Domain Decomposition
 *
 * Provides hybrid MPI + GPU parallelism for distributed multi-GPU computing:
 * - MPI ranks manage multiple GPUs per node
 * - Cross-node GPU communication via MPI
 * - Automatic GPU-rank affinity
 * - Unified communication API for intra-node and inter-node transfers
 */

#ifndef KOO_GPU_PARALLEL_HYBRIDMPI_H
#define KOO_GPU_PARALLEL_HYBRIDMPI_H

#include "../Device.h"
#include "MultiGPU.h"
#include "GPUComm.h"
#include <vector>
#include <memory>
#include <algorithm>

// MPI support (optional)
#ifdef KOO_USE_MPI
    #include <mpi.h>
#endif

namespace koo {
namespace gpu {
namespace parallel {

/**
 * @struct MPIRankInfo
 * @brief Information about MPI rank and GPU assignment
 */
struct MPIRankInfo {
    int rank{0};                  ///< MPI rank ID
    int numRanks{1};              ///< Total number of MPI ranks
    int numGPUsPerRank{0};        ///< GPUs assigned to this rank
    std::vector<int> gpuIds;      ///< GPU device IDs for this rank
    bool isMaster{true};          ///< Is this the master rank (rank 0)?

    /**
     * @brief Get global GPU index from local index
     */
    int globalGPUId(int localIndex) const {
        return rank * numGPUsPerRank + localIndex;
    }
};

/**
 * @class HybridMPIManager
 * @brief Manages hybrid MPI + GPU parallelism
 *
 * Features:
 * - Automatic GPU-to-rank assignment
 * - Intra-node GPU-to-GPU communication
 * - Inter-node MPI communication
 * - Unified communication interface
 *
 * Example:
 * @code
 *   HybridMPIManager hybrid(argc, argv);
 *   hybrid.initialize();
 *
 *   auto& localGPUs = hybrid.getLocalGPUManager();
 *   // Use local GPUs within this MPI rank
 *
 *   hybrid.finalize();
 * @endcode
 */
class HybridMPIManager {
public:
    /**
     * @brief Constructor
     * @param argc Command line argument count (for MPI_Init)
     * @param argv Command line arguments (for MPI_Init)
     */
    HybridMPIManager(int* argc = nullptr, char*** argv = nullptr)
        : argc_(argc), argv_(argv) {
    }

    /**
     * @brief Destructor
     */
    ~HybridMPIManager() {
        finalize();
    }

    // Non-copyable
    HybridMPIManager(const HybridMPIManager&) = delete;
    HybridMPIManager& operator=(const HybridMPIManager&) = delete;

    /**
     * @brief Initialize hybrid MPI + GPU system
     * @param gpusPerRank Number of GPUs per MPI rank (0 = auto-detect)
     */
    void initialize(int gpusPerRank = 0) {
        if (initialized_) {
            return;
        }

#ifdef KOO_USE_MPI
        // Initialize MPI
        int mpiInitialized = 0;
        MPI_Initialized(&mpiInitialized);

        if (!mpiInitialized) {
            if (argc_ && argv_) {
                MPI_Init(argc_, argv_);
            } else {
                MPI_Init(nullptr, nullptr);
            }
            mpiInitialized_ = true;
        }

        // Get MPI rank information
        MPI_Comm_rank(MPI_COMM_WORLD, &rankInfo_.rank);
        MPI_Comm_size(MPI_COMM_WORLD, &rankInfo_.numRanks);
        rankInfo_.isMaster = (rankInfo_.rank == 0);
#else
        // No MPI: single rank
        rankInfo_.rank = 0;
        rankInfo_.numRanks = 1;
        rankInfo_.isMaster = true;
#endif

        // Detect GPUs
        int totalGPUs = Device::getDeviceCount();

        if (gpusPerRank == 0) {
            // Auto-detect: divide GPUs among ranks
            rankInfo_.numGPUsPerRank = totalGPUs / rankInfo_.numRanks;
            if (rankInfo_.numGPUsPerRank == 0) {
                rankInfo_.numGPUsPerRank = 1;  // At least 1 GPU per rank if available
            }
        } else {
            rankInfo_.numGPUsPerRank = gpusPerRank;
        }

        // Assign GPUs to this rank
        assignGPUsToRank();

        // Initialize local GPU manager
        if (!rankInfo_.gpuIds.empty()) {
            gpuManager_ = std::make_unique<MultiGPUManager>();
            gpuManager_->initialize(rankInfo_.gpuIds);

            // Initialize GPU communicator
            gpuComm_ = std::make_unique<GPUCommunicator>(*gpuManager_);
        }

        initialized_ = true;

        // Print info from master rank
        if (isMaster()) {
            printInfo();
        }
    }

    /**
     * @brief Finalize and cleanup
     */
    void finalize() {
        if (!initialized_) {
            return;
        }

        // Cleanup GPU resources
        if (gpuComm_) {
            gpuComm_.reset();
        }

        if (gpuManager_) {
            gpuManager_->finalize();
            gpuManager_.reset();
        }

#ifdef KOO_USE_MPI
        // Finalize MPI if we initialized it
        if (mpiInitialized_) {
            MPI_Finalize();
            mpiInitialized_ = false;
        }
#endif

        initialized_ = false;
    }

    /**
     * @brief Get MPI rank information
     */
    const MPIRankInfo& getRankInfo() const {
        return rankInfo_;
    }

    /**
     * @brief Check if this is the master rank
     */
    bool isMaster() const {
        return rankInfo_.isMaster;
    }

    /**
     * @brief Get local GPU manager for this rank
     */
    MultiGPUManager& getLocalGPUManager() {
        if (!gpuManager_) {
            throw GPUError("GPU manager not initialized");
        }
        return *gpuManager_;
    }

    /**
     * @brief Get local GPU communicator
     */
    GPUCommunicator& getGPUCommunicator() {
        if (!gpuComm_) {
            throw GPUError("GPU communicator not initialized");
        }
        return *gpuComm_;
    }

    /**
     * @brief Barrier synchronization across all MPI ranks
     */
    void barrier() {
#ifdef KOO_USE_MPI
        MPI_Barrier(MPI_COMM_WORLD);
#endif
    }

    /**
     * @brief Broadcast data from master to all ranks
     * @param data Data buffer
     * @param count Number of elements
     */
    template<typename T>
    void broadcast(T* data, int count, int root = 0) {
#ifdef KOO_USE_MPI
        MPI_Datatype mpiType = getMPIDatatype<T>();
        MPI_Bcast(data, count, mpiType, root, MPI_COMM_WORLD);
#endif
    }

    /**
     * @brief Gather data from all ranks to root
     * @param sendbuf Send buffer
     * @param sendcount Number of elements to send
     * @param recvbuf Receive buffer (only valid on root)
     * @param root Root rank
     */
    template<typename T>
    void gather(const T* sendbuf, int sendcount, T* recvbuf, int root = 0) {
#ifdef KOO_USE_MPI
        MPI_Datatype mpiType = getMPIDatatype<T>();
        MPI_Gather(sendbuf, sendcount, mpiType,
                   recvbuf, sendcount, mpiType,
                   root, MPI_COMM_WORLD);
#else
        // No MPI: just copy
        if (recvbuf != sendbuf) {
            std::copy(sendbuf, sendbuf + sendcount, recvbuf);
        }
#endif
    }

    /**
     * @brief All-gather: gather from all ranks to all ranks
     */
    template<typename T>
    void allGather(const T* sendbuf, int sendcount, T* recvbuf) {
#ifdef KOO_USE_MPI
        MPI_Datatype mpiType = getMPIDatatype<T>();
        MPI_Allgather(sendbuf, sendcount, mpiType,
                      recvbuf, sendcount, mpiType,
                      MPI_COMM_WORLD);
#else
        if (recvbuf != sendbuf) {
            std::copy(sendbuf, sendbuf + sendcount, recvbuf);
        }
#endif
    }

    /**
     * @brief Reduce operation across all ranks
     * @param sendbuf Send buffer
     * @param recvbuf Receive buffer (only valid on root)
     * @param count Number of elements
     * @param op Operation (sum, max, min, etc.)
     * @param root Root rank
     */
    template<typename T>
    void reduce(const T* sendbuf, T* recvbuf, int count,
#ifdef KOO_USE_MPI
                MPI_Op op = MPI_SUM,
#else
                int op = 0,
#endif
                int root = 0) {
#ifdef KOO_USE_MPI
        MPI_Datatype mpiType = getMPIDatatype<T>();
        MPI_Reduce(sendbuf, recvbuf, count, mpiType, op, root, MPI_COMM_WORLD);
#else
        if (recvbuf != sendbuf) {
            std::copy(sendbuf, sendbuf + count, recvbuf);
        }
#endif
    }

    /**
     * @brief All-reduce: reduce across all ranks, result on all ranks
     */
    template<typename T>
    void allReduce(const T* sendbuf, T* recvbuf, int count,
#ifdef KOO_USE_MPI
                   MPI_Op op = MPI_SUM
#else
                   int op = 0
#endif
    ) {
#ifdef KOO_USE_MPI
        MPI_Datatype mpiType = getMPIDatatype<T>();
        MPI_Allreduce(sendbuf, recvbuf, count, mpiType, op, MPI_COMM_WORLD);
#else
        if (recvbuf != sendbuf) {
            std::copy(sendbuf, sendbuf + count, recvbuf);
        }
#endif
    }

    /**
     * @brief Point-to-point send
     */
    template<typename T>
    void send(const T* buf, int count, int dest, int tag = 0) {
#ifdef KOO_USE_MPI
        MPI_Datatype mpiType = getMPIDatatype<T>();
        MPI_Send(buf, count, mpiType, dest, tag, MPI_COMM_WORLD);
#endif
    }

    /**
     * @brief Point-to-point receive
     */
    template<typename T>
    void recv(T* buf, int count, int source, int tag = 0) {
#ifdef KOO_USE_MPI
        MPI_Datatype mpiType = getMPIDatatype<T>();
        MPI_Status status;
        MPI_Recv(buf, count, mpiType, source, tag, MPI_COMM_WORLD, &status);
#endif
    }

    /**
     * @brief Print hybrid configuration
     */
    void printInfo() const {
        std::cout << "=== Hybrid MPI + GPU Configuration ===" << std::endl;
        std::cout << "MPI Ranks: " << rankInfo_.numRanks << std::endl;
        std::cout << "GPUs per Rank: " << rankInfo_.numGPUsPerRank << std::endl;
        std::cout << "Total GPUs: " << (rankInfo_.numRanks * rankInfo_.numGPUsPerRank) << std::endl;

#ifdef KOO_USE_MPI
        std::cout << "MPI Support: Enabled" << std::endl;
#else
        std::cout << "MPI Support: Disabled" << std::endl;
#endif

        std::cout << "\nRank " << rankInfo_.rank << " GPU Assignment:" << std::endl;
        for (size_t i = 0; i < rankInfo_.gpuIds.size(); ++i) {
            std::cout << "  Local GPU " << i << " -> Device " << rankInfo_.gpuIds[i] << std::endl;
        }

        if (gpuManager_) {
            std::cout << std::endl;
            gpuManager_->printInfo();
        }
    }

private:
    /**
     * @brief Assign GPUs to this rank
     */
    void assignGPUsToRank() {
        int totalGPUs = Device::getDeviceCount();

        if (totalGPUs == 0) {
            // No GPUs available
            return;
        }

        // Calculate GPU range for this rank
        int startGPU = rankInfo_.rank * rankInfo_.numGPUsPerRank;
        int endGPU = std::min(startGPU + rankInfo_.numGPUsPerRank, totalGPUs);

        rankInfo_.gpuIds.clear();
        for (int i = startGPU; i < endGPU; ++i) {
            rankInfo_.gpuIds.push_back(i);
        }

        rankInfo_.numGPUsPerRank = static_cast<int>(rankInfo_.gpuIds.size());
    }

    /**
     * @brief Get MPI datatype for template type
     */
    template<typename T>
#ifdef KOO_USE_MPI
    MPI_Datatype getMPIDatatype() const {
#else
    int getMPIDatatype() const {
#endif
#ifdef KOO_USE_MPI
        if (std::is_same<T, int>::value) return MPI_INT;
        if (std::is_same<T, float>::value) return MPI_FLOAT;
        if (std::is_same<T, double>::value) return MPI_DOUBLE;
        if (std::is_same<T, long>::value) return MPI_LONG;
        if (std::is_same<T, unsigned int>::value) return MPI_UNSIGNED;
        if (std::is_same<T, char>::value) return MPI_CHAR;
        throw GPUError("Unsupported MPI datatype");
#else
        return 0;
#endif
    }

    bool initialized_{false};
    bool mpiInitialized_{false};
    int* argc_{nullptr};
    char*** argv_{nullptr};

    MPIRankInfo rankInfo_;
    std::unique_ptr<MultiGPUManager> gpuManager_;
    std::unique_ptr<GPUCommunicator> gpuComm_;
};

} // namespace parallel
} // namespace gpu
} // namespace koo

#endif // KOO_GPU_PARALLEL_HYBRIDMPI_H
