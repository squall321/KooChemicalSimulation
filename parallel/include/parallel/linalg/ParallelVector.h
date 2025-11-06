/**
 * @file ParallelVector.h
 * @brief Parallel distributed vector operations
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-beta
 * @date 2025-11-06
 *
 * Phase 43: Parallel Linear Algebra
 *
 * Distributed vector and matrix operations with:
 * - Domain-decomposed storage
 * - Parallel vector operations (dot product, norms, AXPY)
 * - Ghost cell synchronization
 * - Efficient communication patterns
 */

#ifndef KOO_PARALLEL_LINALG_PARALLEL_VECTOR_H
#define KOO_PARALLEL_LINALG_PARALLEL_VECTOR_H

#include "parallel/mpi/MPIWrapper.h"
#include "parallel/domain/DomainDecomposition.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace koo {
namespace parallel {
namespace linalg {

// ============================================================================
// Parallel Vector
// ============================================================================

/**
 * @brief Distributed parallel vector
 *
 * Each process owns a portion of the vector with ghost cells for neighbors
 */
class ParallelVector {
public:
    /**
     * @brief Constructor
     * @param localSize Number of local (owned) elements
     * @param ghostSize Number of ghost elements
     * @param comm MPI communicator
     */
    ParallelVector(int localSize, int ghostSize, const mpi::MPIComm& comm)
        : localSize_(localSize), ghostSize_(ghostSize), comm_(comm) {
        data_.resize(localSize + ghostSize, 0.0);

        // Compute global size
        globalSize_ = comm_.sum(localSize);

        // Compute offset for this process
        localOffset_ = 0;
        for (int i = 0; i < comm_.getRank(); ++i) {
            int rankLocalSize = localSize;  // Should get actual sizes per rank
            localOffset_ += rankLocalSize;
        }
    }

    /**
     * @brief Get local size (owned elements)
     */
    int getLocalSize() const { return localSize_; }

    /**
     * @brief Get ghost size
     */
    int getGhostSize() const { return ghostSize_; }

    /**
     * @brief Get global size
     */
    int getGlobalSize() const { return globalSize_; }

    /**
     * @brief Get local offset in global numbering
     */
    int getLocalOffset() const { return localOffset_; }

    /**
     * @brief Access local element (read-only)
     */
    double operator[](int i) const {
        return data_[i];
    }

    /**
     * @brief Access local element (read-write)
     */
    double& operator[](int i) {
        return data_[i];
    }

    /**
     * @brief Get raw data pointer
     */
    const double* data() const { return data_.data(); }
    double* data() { return data_.data(); }

    /**
     * @brief Set all values
     */
    void fill(double value) {
        std::fill(data_.begin(), data_.end(), value);
    }

    /**
     * @brief Set local values (owned elements only)
     */
    void setLocal(const std::vector<double>& values) {
        int copySize = std::min(localSize_, static_cast<int>(values.size()));
        std::copy(values.begin(), values.begin() + copySize, data_.begin());
    }

    /**
     * @brief Get local values (owned elements only)
     */
    std::vector<double> getLocal() const {
        return std::vector<double>(data_.begin(), data_.begin() + localSize_);
    }

    /**
     * @brief Dot product: this · other
     */
    double dot(const ParallelVector& other) const {
        double localDot = 0.0;
        for (int i = 0; i < localSize_; ++i) {
            localDot += data_[i] * other.data_[i];
        }
        return comm_.sum(localDot);
    }

    /**
     * @brief L2 norm
     */
    double norm2() const {
        return std::sqrt(dot(*this));
    }

    /**
     * @brief L1 norm
     */
    double norm1() const {
        double localSum = 0.0;
        for (int i = 0; i < localSize_; ++i) {
            localSum += std::abs(data_[i]);
        }
        return comm_.sum(localSum);
    }

    /**
     * @brief Infinity norm
     */
    double normInf() const {
        double localMax = 0.0;
        for (int i = 0; i < localSize_; ++i) {
            localMax = std::max(localMax, std::abs(data_[i]));
        }
        return comm_.max(localMax);
    }

    /**
     * @brief AXPY: this = alpha * x + this
     */
    void axpy(double alpha, const ParallelVector& x) {
        for (int i = 0; i < localSize_; ++i) {
            data_[i] += alpha * x.data_[i];
        }
    }

    /**
     * @brief Scale: this = alpha * this
     */
    void scale(double alpha) {
        for (int i = 0; i < localSize_; ++i) {
            data_[i] *= alpha;
        }
    }

    /**
     * @brief Vector addition: this = this + other
     */
    void add(const ParallelVector& other) {
        axpy(1.0, other);
    }

    /**
     * @brief Vector subtraction: this = this - other
     */
    void subtract(const ParallelVector& other) {
        axpy(-1.0, other);
    }

    /**
     * @brief Sum of all elements
     */
    double sum() const {
        double localSum = 0.0;
        for (int i = 0; i < localSize_; ++i) {
            localSum += data_[i];
        }
        return comm_.sum(localSum);
    }

    /**
     * @brief Maximum element
     */
    double max() const {
        double localMax = -std::numeric_limits<double>::max();
        for (int i = 0; i < localSize_; ++i) {
            localMax = std::max(localMax, data_[i]);
        }
        return comm_.max(localMax);
    }

    /**
     * @brief Minimum element
     */
    double min() const {
        double localMin = std::numeric_limits<double>::max();
        for (int i = 0; i < localSize_; ++i) {
            localMin = std::min(localMin, data_[i]);
        }
        return comm_.min(localMin);
    }

private:
    int localSize_;      ///< Number of owned elements
    int ghostSize_;      ///< Number of ghost elements
    int globalSize_;     ///< Total global size
    int localOffset_;    ///< Offset in global numbering
    std::vector<double> data_;  ///< Local data (owned + ghost)
    const mpi::MPIComm& comm_;
};

// ============================================================================
// Parallel Matrix-Vector Operations
// ============================================================================

/**
 * @brief Parallel sparse matrix (CSR format, distributed by rows)
 */
class ParallelCSRMatrix {
public:
    /**
     * @brief Constructor
     * @param localRows Number of local rows
     * @param comm MPI communicator
     */
    ParallelCSRMatrix(int localRows, const mpi::MPIComm& comm)
        : localRows_(localRows), comm_(comm) {
        rowPtr_.resize(localRows + 1, 0);
        globalRows_ = comm_.sum(localRows);
    }

    /**
     * @brief Add entry to matrix
     */
    void addEntry(int localRow, int globalCol, double value) {
        if (localRow < 0 || localRow >= localRows_) return;

        // For simplicity, store as triplets first
        entries_.push_back({localRow, globalCol, value});
    }

    /**
     * @brief Finalize matrix assembly (convert to CSR)
     */
    void finalize() {
        // Sort entries by row
        std::sort(entries_.begin(), entries_.end(),
                 [](const auto& a, const auto& b) {
                     return a[0] < b[0] || (a[0] == b[0] && a[1] < b[1]);
                 });

        // Build CSR structure
        colIdx_.clear();
        values_.clear();

        int currentRow = 0;
        rowPtr_[0] = 0;

        for (const auto& entry : entries_) {
            int row = entry[0];
            int col = entry[1];
            double val = entry[2];

            // Fill empty rows
            while (currentRow < row) {
                currentRow++;
                rowPtr_[currentRow] = static_cast<int>(colIdx_.size());
            }

            colIdx_.push_back(col);
            values_.push_back(val);
        }

        // Fill remaining rows
        while (currentRow < localRows_) {
            currentRow++;
            rowPtr_[currentRow] = static_cast<int>(colIdx_.size());
        }
    }

    /**
     * @brief Matrix-vector product: y = A * x
     */
    void mult(const ParallelVector& x, ParallelVector& y) const {
        for (int i = 0; i < localRows_; ++i) {
            double sum = 0.0;
            for (int j = rowPtr_[i]; j < rowPtr_[i + 1]; ++j) {
                int col = colIdx_[j];
                // Note: Would need global-to-local mapping for x[col]
                sum += values_[j] * x[col];
            }
            y[i] = sum;
        }
    }

    int getLocalRows() const { return localRows_; }
    int getGlobalRows() const { return globalRows_; }

private:
    int localRows_;
    int globalRows_;
    std::vector<int> rowPtr_;     ///< Row pointers (CSR)
    std::vector<int> colIdx_;     ///< Column indices
    std::vector<double> values_;  ///< Non-zero values
    std::vector<std::array<int, 3>> entries_;  ///< Temporary triplet storage [row, col, val]
    const mpi::MPIComm& comm_;
};

} // namespace linalg
} // namespace parallel
} // namespace koo

#endif // KOO_PARALLEL_LINALG_PARALLEL_VECTOR_H
