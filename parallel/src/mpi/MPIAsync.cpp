/**
 * @file MPIAsync.cpp
 * @brief Implementation of non-blocking MPI communication
 */

#include "parallel/mpi/MPIAsync.h"
#include <algorithm>

namespace koo {
namespace parallel {
namespace mpi {

// ============================================================================
// MPIRequest Implementation
// ============================================================================

MPIRequest::MPIRequest() : active_(false) {
#ifdef USE_MPI
    request_ = MPI_REQUEST_NULL;
#endif
}

MPIRequest::~MPIRequest() {
    if (active_) {
        cancel();
    }
}

MPIRequest::MPIRequest(MPIRequest&& other) noexcept
    : active_(other.active_) {
#ifdef USE_MPI
    request_ = other.request_;
    other.request_ = MPI_REQUEST_NULL;
#endif
    other.active_ = false;
}

MPIRequest& MPIRequest::operator=(MPIRequest&& other) noexcept {
    if (this != &other) {
        if (active_) {
            cancel();
        }

#ifdef USE_MPI
        request_ = other.request_;
        other.request_ = MPI_REQUEST_NULL;
#endif
        active_ = other.active_;
        other.active_ = false;
    }
    return *this;
}

void MPIRequest::wait() {
#ifdef USE_MPI
    if (active_ && request_ != MPI_REQUEST_NULL) {
        MPI_Status status;
        MPI_Wait(&request_, &status);
        active_ = false;
        request_ = MPI_REQUEST_NULL;
    }
#endif
}

bool MPIRequest::test() {
#ifdef USE_MPI
    if (active_ && request_ != MPI_REQUEST_NULL) {
        int flag;
        MPI_Status status;
        MPI_Test(&request_, &flag, &status);

        if (flag) {
            active_ = false;
            request_ = MPI_REQUEST_NULL;
            return true;
        }
        return false;
    }
#endif
    return true;  // Already completed
}

void MPIRequest::cancel() {
#ifdef USE_MPI
    if (active_ && request_ != MPI_REQUEST_NULL) {
        MPI_Cancel(&request_);
        MPI_Status status;
        MPI_Wait(&request_, &status);  // Must wait after cancel
        active_ = false;
        request_ = MPI_REQUEST_NULL;
    }
#endif
}

// ============================================================================
// Multiple Request Operations
// ============================================================================

void waitAll(std::vector<std::unique_ptr<MPIRequest>>& requests) {
#ifdef USE_MPI
    std::vector<MPI_Request> mpi_requests;
    for (const auto& req : requests) {
        if (req && req->isActive()) {
            mpi_requests.push_back(*req->getRawRequest());
        }
    }

    if (!mpi_requests.empty()) {
        std::vector<MPI_Status> statuses(mpi_requests.size());
        MPI_Waitall(static_cast<int>(mpi_requests.size()),
                    mpi_requests.data(), statuses.data());

        // Mark all as inactive
        for (auto& req : requests) {
            if (req) {
                req->wait();  // This will mark as inactive
            }
        }
    }
#else
    (void)requests;
#endif
}

int waitAny(std::vector<std::unique_ptr<MPIRequest>>& requests) {
#ifdef USE_MPI
    std::vector<MPI_Request> mpi_requests;
    std::vector<int> indices;

    for (size_t i = 0; i < requests.size(); ++i) {
        if (requests[i] && requests[i]->isActive()) {
            mpi_requests.push_back(*requests[i]->getRawRequest());
            indices.push_back(static_cast<int>(i));
        }
    }

    if (mpi_requests.empty()) {
        return -1;
    }

    int index;
    MPI_Status status;
    MPI_Waitany(static_cast<int>(mpi_requests.size()),
                mpi_requests.data(), &index, &status);

    if (index != MPI_UNDEFINED && index >= 0) {
        int userIndex = indices[index];
        requests[userIndex]->wait();  // Mark as inactive
        return userIndex;
    }
#else
    (void)requests;
#endif
    return -1;
}

bool testAll(std::vector<std::unique_ptr<MPIRequest>>& requests) {
#ifdef USE_MPI
    std::vector<MPI_Request> mpi_requests;
    for (const auto& req : requests) {
        if (req && req->isActive()) {
            mpi_requests.push_back(*req->getRawRequest());
        }
    }

    if (mpi_requests.empty()) {
        return true;  // All already completed
    }

    int flag;
    std::vector<MPI_Status> statuses(mpi_requests.size());
    MPI_Testall(static_cast<int>(mpi_requests.size()),
                mpi_requests.data(), &flag, statuses.data());

    if (flag) {
        // Mark all as inactive
        for (auto& req : requests) {
            if (req) {
                req->wait();
            }
        }
        return true;
    }
    return false;
#else
    (void)requests;
    return true;
#endif
}

int testAny(std::vector<std::unique_ptr<MPIRequest>>& requests) {
#ifdef USE_MPI
    std::vector<MPI_Request> mpi_requests;
    std::vector<int> indices;

    for (size_t i = 0; i < requests.size(); ++i) {
        if (requests[i] && requests[i]->isActive()) {
            mpi_requests.push_back(*requests[i]->getRawRequest());
            indices.push_back(static_cast<int>(i));
        }
    }

    if (mpi_requests.empty()) {
        return -1;
    }

    int index, flag;
    MPI_Status status;
    MPI_Testany(static_cast<int>(mpi_requests.size()),
                mpi_requests.data(), &index, &flag, &status);

    if (flag && index != MPI_UNDEFINED && index >= 0) {
        int userIndex = indices[index];
        requests[userIndex]->wait();  // Mark as inactive
        return userIndex;
    }
#else
    (void)requests;
#endif
    return -1;
}

} // namespace mpi
} // namespace parallel
} // namespace koo
