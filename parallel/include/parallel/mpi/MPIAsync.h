/**
 * @file MPIAsync.h
 * @brief Non-blocking (asynchronous) MPI communication
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha5
 * @date 2025-11-08
 *
 * Priority D1.2: MPI Communication Layer Enhancement
 *
 * Non-blocking communication enables overlapping computation with communication
 * for better performance on modern HPC systems.
 *
 * Features:
 * - Non-blocking send/recv (Isend, Irecv)
 * - Request management (Wait, Test, Cancel)
 * - Multiple requests (Waitall, Waitany, Testall)
 * - Persistent communication for repeated patterns
 */

#ifndef KOO_PARALLEL_MPI_ASYNC_H
#define KOO_PARALLEL_MPI_ASYNC_H

#include "parallel/mpi/MPIWrapper.h"
#include <vector>
#include <memory>

#ifdef USE_MPI
#include <mpi.h>
#endif

namespace koo {
namespace parallel {
namespace mpi {

// ============================================================================
// MPI Request Handle
// ============================================================================

/**
 * @brief RAII wrapper for MPI_Request
 *
 * Manages lifetime of non-blocking communication requests
 */
class MPIRequest {
public:
    /**
     * @brief Default constructor
     */
    MPIRequest();

    /**
     * @brief Destructor - cancels request if not completed
     */
    ~MPIRequest();

    // Delete copy, allow move
    MPIRequest(const MPIRequest&) = delete;
    MPIRequest& operator=(const MPIRequest&) = delete;
    MPIRequest(MPIRequest&& other) noexcept;
    MPIRequest& operator=(MPIRequest&& other) noexcept;

    /**
     * @brief Wait for request to complete
     */
    void wait();

    /**
     * @brief Test if request has completed (non-blocking)
     * @return true if completed
     */
    bool test();

    /**
     * @brief Cancel the request
     */
    void cancel();

    /**
     * @brief Check if request is active
     */
    bool isActive() const { return active_; }

#ifdef USE_MPI
    /**
     * @brief Get raw MPI_Request (for internal use)
     */
    MPI_Request* getRawRequest() { return &request_; }
#endif

private:
#ifdef USE_MPI
    MPI_Request request_;
#endif
    bool active_;
};

// ============================================================================
// Multiple Request Operations
// ============================================================================

/**
 * @brief Wait for all requests to complete
 * @param requests Vector of requests
 */
void waitAll(std::vector<std::unique_ptr<MPIRequest>>& requests);

/**
 * @brief Wait for any one request to complete
 * @param requests Vector of requests
 * @return Index of completed request
 */
int waitAny(std::vector<std::unique_ptr<MPIRequest>>& requests);

/**
 * @brief Test if all requests have completed
 * @param requests Vector of requests
 * @return true if all completed
 */
bool testAll(std::vector<std::unique_ptr<MPIRequest>>& requests);

/**
 * @brief Test if any request has completed
 * @param requests Vector of requests
 * @return Index of completed request, or -1 if none
 */
int testAny(std::vector<std::unique_ptr<MPIRequest>>& requests);

// ============================================================================
// Non-Blocking Communication
// ============================================================================

/**
 * @brief Non-blocking point-to-point communication
 *
 * Extends MPIComm with asynchronous operations
 */
class MPIAsyncComm {
public:
    /**
     * @brief Constructor
     * @param comm Base MPI communicator
     */
    explicit MPIAsyncComm(const MPIComm& comm) : comm_(comm) {}

    /**
     * @brief Non-blocking send
     * @return Request handle
     */
    template<typename T>
    std::unique_ptr<MPIRequest> isend(const T* data, int count,
                                      int dest, int tag = 0) {
        auto request = std::make_unique<MPIRequest>();

#ifdef USE_MPI
        MPI_Isend(data, count, getMPIType<T>(), dest, tag,
                  MPI_COMM_WORLD, request->getRawRequest());
#else
        (void)data; (void)count; (void)dest; (void)tag;
#endif

        return request;
    }

    /**
     * @brief Non-blocking receive
     * @return Request handle
     */
    template<typename T>
    std::unique_ptr<MPIRequest> irecv(T* data, int count,
                                      int source, int tag = 0) {
        auto request = std::make_unique<MPIRequest>();

#ifdef USE_MPI
        MPI_Irecv(data, count, getMPIType<T>(), source, tag,
                  MPI_COMM_WORLD, request->getRawRequest());
#else
        (void)data; (void)count; (void)source; (void)tag;
#endif

        return request;
    }

    /**
     * @brief Non-blocking broadcast
     */
    template<typename T>
    std::unique_ptr<MPIRequest> ibcast(T* data, int count, int root = 0) {
        auto request = std::make_unique<MPIRequest>();

#ifdef USE_MPI
        MPI_Ibcast(data, count, getMPIType<T>(), root,
                   MPI_COMM_WORLD, request->getRawRequest());
#else
        (void)data; (void)count; (void)root;
#endif

        return request;
    }

    /**
     * @brief Non-blocking barrier
     */
    std::unique_ptr<MPIRequest> ibarrier() {
        auto request = std::make_unique<MPIRequest>();

#ifdef USE_MPI
        MPI_Ibarrier(MPI_COMM_WORLD, request->getRawRequest());
#endif

        return request;
    }

    /**
     * @brief Non-blocking reduce
     */
    template<typename T>
    std::unique_ptr<MPIRequest> ireduce(
        const T* sendbuf, T* recvbuf, int count,
        MPIOperation op, int root = 0) {

        auto request = std::make_unique<MPIRequest>();

#ifdef USE_MPI
        MPI_Ireduce(sendbuf, recvbuf, count, getMPIType<T>(),
                    getMPIOp(op), root, MPI_COMM_WORLD,
                    request->getRawRequest());
#else
        if (sendbuf != recvbuf) {
            std::copy(sendbuf, sendbuf + count, recvbuf);
        }
        (void)op; (void)root;
#endif

        return request;
    }

    /**
     * @brief Non-blocking allreduce
     */
    template<typename T>
    std::unique_ptr<MPIRequest> iallreduce(
        const T* sendbuf, T* recvbuf, int count, MPIOperation op) {

        auto request = std::make_unique<MPIRequest>();

#ifdef USE_MPI
        MPI_Iallreduce(sendbuf, recvbuf, count, getMPIType<T>(),
                       getMPIOp(op), MPI_COMM_WORLD,
                       request->getRawRequest());
#else
        if (sendbuf != recvbuf) {
            std::copy(sendbuf, sendbuf + count, recvbuf);
        }
        (void)op;
#endif

        return request;
    }

    /**
     * @brief Get underlying communicator
     */
    const MPIComm& getComm() const { return comm_; }

private:
#ifdef USE_MPI
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
#endif

    const MPIComm& comm_;
};

// ============================================================================
// Persistent Communication
// ============================================================================

/**
 * @brief Persistent communication for repeated patterns
 *
 * Use when same communication pattern is repeated many times
 * (e.g., in a time-stepping loop)
 */
class MPIPersistentComm {
public:
    /**
     * @brief Create persistent send
     */
    template<typename T>
    static std::unique_ptr<MPIRequest> sendInit(
        const T* data, int count, int dest, int tag) {

        auto request = std::make_unique<MPIRequest>();

#ifdef USE_MPI
        MPI_Send_init(data, count, getMPIType<T>(), dest, tag,
                      MPI_COMM_WORLD, request->getRawRequest());
#else
        (void)data; (void)count; (void)dest; (void)tag;
#endif

        return request;
    }

    /**
     * @brief Create persistent receive
     */
    template<typename T>
    static std::unique_ptr<MPIRequest> recvInit(
        T* data, int count, int source, int tag) {

        auto request = std::make_unique<MPIRequest>();

#ifdef USE_MPI
        MPI_Recv_init(data, count, getMPIType<T>(), source, tag,
                      MPI_COMM_WORLD, request->getRawRequest());
#else
        (void)data; (void)count; (void)source; (void)tag;
#endif

        return request;
    }

    /**
     * @brief Start persistent communication
     */
    static void start(MPIRequest& request) {
#ifdef USE_MPI
        MPI_Start(request.getRawRequest());
#else
        (void)request;
#endif
    }

private:
#ifdef USE_MPI
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
#endif
};

} // namespace mpi
} // namespace parallel
} // namespace koo

#endif // KOO_PARALLEL_MPI_ASYNC_H
