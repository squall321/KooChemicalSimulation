/**
 * @file MPIWrapper.h
 * @brief MPI wrapper and communicator abstraction
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-beta
 * @date 2025-11-06
 *
 * Phase 41: MPI Wrapper
 *
 * Provides C++ wrapper around MPI with RAII, type-safe communication,
 * and fallback to serial execution when MPI is not available.
 */

#ifndef KOO_PARALLEL_MPI_MPI_WRAPPER_H
#define KOO_PARALLEL_MPI_MPI_WRAPPER_H

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <algorithm>

// Check if MPI is available
#ifdef USE_MPI
#include <mpi.h>
#endif

namespace koo {
namespace parallel {
namespace mpi {

// ============================================================================
// MPI Data Types
// ============================================================================

/**
 * @brief MPI operation types
 */
enum class MPIOperation {
    SUM,
    PROD,
    MIN,
    MAX,
    LAND,  // Logical AND
    LOR,   // Logical OR
    BAND,  // Bitwise AND
    BOR    // Bitwise OR
};

/**
 * @brief MPI exception
 */
class MPIException : public std::runtime_error {
public:
    explicit MPIException(const std::string& message)
        : std::runtime_error("MPI Error: " + message) {}
};

// ============================================================================
// MPI Communicator
// ============================================================================

/**
 * @brief MPI communicator wrapper
 *
 * Provides RAII wrapper around MPI_Comm with type-safe operations.
 * Falls back to serial execution when MPI is not available.
 */
class MPIComm {
public:
    /**
     * @brief Constructor
     * @param comm MPI communicator (default: MPI_COMM_WORLD)
     */
#ifdef USE_MPI
    explicit MPIComm(MPI_Comm comm = MPI_COMM_WORLD) : comm_(comm) {
        MPI_Comm_rank(comm_, &rank_);
        MPI_Comm_size(comm_, &size_);
    }
#else
    explicit MPIComm(int /*comm*/ = 0) : rank_(0), size_(1) {}
#endif

    /**
     * @brief Get rank of this process
     */
    int getRank() const { return rank_; }

    /**
     * @brief Get total number of processes
     */
    int getSize() const { return size_; }

    /**
     * @brief Check if this is the root process
     */
    bool isRoot() const { return rank_ == 0; }

    /**
     * @brief Barrier synchronization
     */
    void barrier() const {
#ifdef USE_MPI
        MPI_Barrier(comm_);
#endif
    }

    /**
     * @brief Broadcast data from root to all processes
     */
    template<typename T>
    void broadcast(T* data, int count, int root = 0) const {
#ifdef USE_MPI
        MPI_Bcast(data, count, getMPIType<T>(), root, comm_);
#else
        (void)data; (void)count; (void)root;  // Suppress warnings
#endif
    }

    /**
     * @brief Broadcast vector from root to all processes
     */
    template<typename T>
    void broadcast(std::vector<T>& data, int root = 0) const {
        int size = static_cast<int>(data.size());
        broadcast(&size, 1, root);

        if (rank_ != root) {
            data.resize(size);
        }

        if (size > 0) {
            broadcast(data.data(), size, root);
        }
    }

    /**
     * @brief Send data to another process
     */
    template<typename T>
    void send(const T* data, int count, int dest, int tag = 0) const {
#ifdef USE_MPI
        MPI_Send(data, count, getMPIType<T>(), dest, tag, comm_);
#else
        (void)data; (void)count; (void)dest; (void)tag;
#endif
    }

    /**
     * @brief Receive data from another process
     */
    template<typename T>
    void recv(T* data, int count, int source, int tag = 0) const {
#ifdef USE_MPI
        MPI_Status status;
        MPI_Recv(data, count, getMPIType<T>(), source, tag, comm_, &status);
#else
        (void)data; (void)count; (void)source; (void)tag;
#endif
    }

    /**
     * @brief Reduce data with specified operation
     */
    template<typename T>
    void reduce(const T* sendbuf, T* recvbuf, int count,
                MPIOperation op, int root = 0) const {
#ifdef USE_MPI
        MPI_Reduce(sendbuf, recvbuf, count, getMPIType<T>(),
                   getMPIOp(op), root, comm_);
#else
        if (sendbuf != recvbuf) {
            std::copy(sendbuf, sendbuf + count, recvbuf);
        }
        (void)op; (void)root;
#endif
    }

    /**
     * @brief All-reduce (reduce result available on all processes)
     */
    template<typename T>
    void allReduce(const T* sendbuf, T* recvbuf, int count,
                   MPIOperation op) const {
#ifdef USE_MPI
        MPI_Allreduce(sendbuf, recvbuf, count, getMPIType<T>(),
                      getMPIOp(op), comm_);
#else
        if (sendbuf != recvbuf) {
            std::copy(sendbuf, sendbuf + count, recvbuf);
        }
        (void)op;
#endif
    }

    /**
     * @brief Gather data from all processes to root
     */
    template<typename T>
    void gather(const T* sendbuf, int sendcount,
                T* recvbuf, int recvcount, int root = 0) const {
#ifdef USE_MPI
        MPI_Gather(sendbuf, sendcount, getMPIType<T>(),
                   recvbuf, recvcount, getMPIType<T>(), root, comm_);
#else
        if (sendbuf != recvbuf) {
            std::copy(sendbuf, sendbuf + sendcount, recvbuf);
        }
        (void)recvcount; (void)root;
#endif
    }

    /**
     * @brief Scatter data from root to all processes
     */
    template<typename T>
    void scatter(const T* sendbuf, int sendcount,
                 T* recvbuf, int recvcount, int root = 0) const {
#ifdef USE_MPI
        MPI_Scatter(sendbuf, sendcount, getMPIType<T>(),
                    recvbuf, recvcount, getMPIType<T>(), root, comm_);
#else
        if (sendbuf != recvbuf) {
            std::copy(sendbuf, sendbuf + recvcount, recvbuf);
        }
        (void)sendcount; (void)root;
#endif
    }

    /**
     * @brief All-gather (gather result available on all processes)
     */
    template<typename T>
    void allGather(const T* sendbuf, int sendcount,
                   T* recvbuf, int recvcount) const {
#ifdef USE_MPI
        MPI_Allgather(sendbuf, sendcount, getMPIType<T>(),
                      recvbuf, recvcount, getMPIType<T>(), comm_);
#else
        if (sendbuf != recvbuf) {
            std::copy(sendbuf, sendbuf + sendcount, recvbuf);
        }
        (void)recvcount;
#endif
    }

    /**
     * @brief All-to-all communication
     */
    template<typename T>
    void allToAll(const T* sendbuf, int sendcount,
                  T* recvbuf, int recvcount) const {
#ifdef USE_MPI
        MPI_Alltoall(sendbuf, sendcount, getMPIType<T>(),
                     recvbuf, recvcount, getMPIType<T>(), comm_);
#else
        if (sendbuf != recvbuf) {
            std::copy(sendbuf, sendbuf + sendcount, recvbuf);
        }
        (void)recvcount;
#endif
    }

    /**
     * @brief Sum reduction (convenience method)
     */
    template<typename T>
    T sum(T value) const {
        T result;
        allReduce(&value, &result, 1, MPIOperation::SUM);
        return result;
    }

    /**
     * @brief Max reduction (convenience method)
     */
    template<typename T>
    T max(T value) const {
        T result;
        allReduce(&value, &result, 1, MPIOperation::MAX);
        return result;
    }

    /**
     * @brief Min reduction (convenience method)
     */
    template<typename T>
    T min(T value) const {
        T result;
        allReduce(&value, &result, 1, MPIOperation::MIN);
        return result;
    }

private:
#ifdef USE_MPI
    /**
     * @brief Get MPI data type for C++ type
     */
    template<typename T>
    static MPI_Datatype getMPIType() {
        if (std::is_same<T, int>::value) return MPI_INT;
        if (std::is_same<T, long>::value) return MPI_LONG;
        if (std::is_same<T, float>::value) return MPI_FLOAT;
        if (std::is_same<T, double>::value) return MPI_DOUBLE;
        if (std::is_same<T, char>::value) return MPI_CHAR;
        if (std::is_same<T, unsigned int>::value) return MPI_UNSIGNED;
        if (std::is_same<T, unsigned long>::value) return MPI_UNSIGNED_LONG;
        throw MPIException("Unsupported MPI data type");
    }

    /**
     * @brief Get MPI operation
     */
    static MPI_Op getMPIOp(MPIOperation op) {
        switch (op) {
            case MPIOperation::SUM: return MPI_SUM;
            case MPIOperation::PROD: return MPI_PROD;
            case MPIOperation::MIN: return MPI_MIN;
            case MPIOperation::MAX: return MPI_MAX;
            case MPIOperation::LAND: return MPI_LAND;
            case MPIOperation::LOR: return MPI_LOR;
            case MPIOperation::BAND: return MPI_BAND;
            case MPIOperation::BOR: return MPI_BOR;
            default: return MPI_SUM;
        }
    }

    MPI_Comm comm_;
#endif
    int rank_;
    int size_;
};

// ============================================================================
// MPI Environment
// ============================================================================

/**
 * @brief MPI environment manager (RAII)
 *
 * Initializes and finalizes MPI automatically
 */
class MPIEnvironment {
public:
    /**
     * @brief Constructor - initializes MPI
     */
    MPIEnvironment(int* argc = nullptr, char*** argv = nullptr) {
#ifdef USE_MPI
        int initialized;
        MPI_Initialized(&initialized);
        if (!initialized) {
            MPI_Init(argc, argv);
            shouldFinalize_ = true;
        }
#else
        (void)argc; (void)argv;
#endif
    }

    /**
     * @brief Destructor - finalizes MPI
     */
    ~MPIEnvironment() {
#ifdef USE_MPI
        if (shouldFinalize_) {
            int finalized;
            MPI_Finalized(&finalized);
            if (!finalized) {
                MPI_Finalize();
            }
        }
#endif
    }

    // Delete copy and move
    MPIEnvironment(const MPIEnvironment&) = delete;
    MPIEnvironment& operator=(const MPIEnvironment&) = delete;
    MPIEnvironment(MPIEnvironment&&) = delete;
    MPIEnvironment& operator=(MPIEnvironment&&) = delete;

    /**
     * @brief Check if MPI is available
     */
    static bool isAvailable() {
#ifdef USE_MPI
        return true;
#else
        return false;
#endif
    }

    /**
     * @brief Get world communicator
     */
    static MPIComm getWorldComm() {
#ifdef USE_MPI
        return MPIComm(MPI_COMM_WORLD);
#else
        return MPIComm(0);
#endif
    }

    /**
     * @brief Get self communicator
     */
    static MPIComm getSelfComm() {
#ifdef USE_MPI
        return MPIComm(MPI_COMM_SELF);
#else
        return MPIComm(0);
#endif
    }

private:
    bool shouldFinalize_ = false;
};

} // namespace mpi
} // namespace parallel
} // namespace koo

#endif // KOO_PARALLEL_MPI_MPI_WRAPPER_H
