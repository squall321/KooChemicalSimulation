/**
 * @file LinearSystem.h
 * @brief Linear system representation for PDE solvers
 * @author KooChemicalSimulation Development Team
 * @version 0.3.0-alpha1
 * @date 2025-11-06
 *
 * Defines data structures for representing linear systems Ax = b.
 */

#ifndef KOO_SOLVER_PDE_LINEAR_SYSTEM_H
#define KOO_SOLVER_PDE_LINEAR_SYSTEM_H

#include "SolverTypes.h"
#include <vector>
#include <map>
#include <stdexcept>
#include <cmath>

namespace koo {
namespace solver {
namespace pde {

/**
 * @brief Sparse matrix storage format
 */
enum class MatrixFormat {
    DENSE,          ///< Dense matrix
    CSR,            ///< Compressed Sparse Row
    CSC,            ///< Compressed Sparse Column
    COO,            ///< Coordinate format
    CUSTOM          ///< Custom format
};

/**
 * @brief Sparse matrix in CSR (Compressed Sparse Row) format
 *
 * Stores matrix as:
 * - values: non-zero values
 * - colIndices: column indices
 * - rowPtr: row pointers
 */
class SparseMatrix {
public:
    SparseMatrix()
        : numRows_(0), numCols_(0), nnz_(0), format_(MatrixFormat::CSR) {}

    SparseMatrix(size_t rows, size_t cols, MatrixFormat format = MatrixFormat::CSR)
        : numRows_(rows), numCols_(cols), nnz_(0), format_(format) {
        if (format_ == MatrixFormat::CSR) {
            rowPtr_.resize(rows + 1, 0);
        }
    }

    /**
     * @brief Get number of rows
     */
    size_t rows() const { return numRows_; }

    /**
     * @brief Get number of columns
     */
    size_t cols() const { return numCols_; }

    /**
     * @brief Get number of non-zeros
     */
    size_t nnz() const { return nnz_; }

    /**
     * @brief Get matrix format
     */
    MatrixFormat getFormat() const { return format_; }

    /**
     * @brief Reserve space for non-zeros
     */
    void reserve(size_t capacity) {
        values_.reserve(capacity);
        colIndices_.reserve(capacity);
    }

    /**
     * @brief Add value to matrix (COO format build)
     */
    void addEntry(size_t row, size_t col, double value) {
        if (row >= numRows_ || col >= numCols_) {
            throw std::out_of_range("Matrix index out of range");
        }

        // Store in COO format temporarily
        cooRows_.push_back(row);
        cooCols_.push_back(col);
        cooVals_.push_back(value);
        nnz_++;
    }

    /**
     * @brief Finalize matrix (convert COO to CSR)
     */
    void finalize() {
        if (format_ != MatrixFormat::CSR || cooRows_.empty()) {
            return;
        }

        // Convert COO to CSR
        values_.clear();
        colIndices_.clear();
        rowPtr_.assign(numRows_ + 1, 0);

        // Count entries per row
        for (size_t row : cooRows_) {
            rowPtr_[row + 1]++;
        }

        // Cumulative sum
        for (size_t i = 0; i < numRows_; ++i) {
            rowPtr_[i + 1] += rowPtr_[i];
        }

        // Fill values and column indices
        values_.resize(nnz_);
        colIndices_.resize(nnz_);

        std::vector<size_t> rowCounter = rowPtr_;
        for (size_t i = 0; i < nnz_; ++i) {
            size_t row = cooRows_[i];
            size_t idx = rowCounter[row]++;
            values_[idx] = cooVals_[i];
            colIndices_[idx] = cooCols_[i];
        }

        // Clear COO data
        cooRows_.clear();
        cooCols_.clear();
        cooVals_.clear();
    }

    /**
     * @brief Matrix-vector product: y = A * x
     */
    void multiply(const std::vector<double>& x, std::vector<double>& y) const {
        if (x.size() != numCols_) {
            throw std::runtime_error("Vector size mismatch");
        }

        y.resize(numRows_);
        std::fill(y.begin(), y.end(), 0.0);

        if (format_ == MatrixFormat::CSR) {
            for (size_t i = 0; i < numRows_; ++i) {
                for (size_t j = rowPtr_[i]; j < rowPtr_[i + 1]; ++j) {
                    y[i] += values_[j] * x[colIndices_[j]];
                }
            }
        }
    }

    /**
     * @brief Get CSR data
     */
    const std::vector<double>& getValues() const { return values_; }
    const std::vector<size_t>& getColIndices() const { return colIndices_; }
    const std::vector<size_t>& getRowPtr() const { return rowPtr_; }

    /**
     * @brief Set value at specific location (slow, for testing)
     */
    void setValue(size_t row, size_t col, double value) {
        if (format_ != MatrixFormat::CSR) {
            throw std::runtime_error("setValue only supported for CSR format after finalize");
        }

        // Find entry
        for (size_t j = rowPtr_[row]; j < rowPtr_[row + 1]; ++j) {
            if (colIndices_[j] == col) {
                values_[j] = value;
                return;
            }
        }

        throw std::runtime_error("Entry not found in sparse matrix");
    }

    /**
     * @brief Get value at specific location (slow, for testing)
     */
    double getValue(size_t row, size_t col) const {
        if (format_ != MatrixFormat::CSR) {
            return 0.0;
        }

        for (size_t j = rowPtr_[row]; j < rowPtr_[row + 1]; ++j) {
            if (colIndices_[j] == col) {
                return values_[j];
            }
        }

        return 0.0;
    }

    /**
     * @brief Check if matrix is symmetric
     */
    bool isSymmetric(double tol = 1e-10) const {
        for (size_t i = 0; i < numRows_; ++i) {
            for (size_t j = rowPtr_[i]; j < rowPtr_[i + 1]; ++j) {
                size_t col = colIndices_[j];
                double valIJ = values_[j];
                double valJI = getValue(col, i);
                if (std::abs(valIJ - valJI) > tol) {
                    return false;
                }
            }
        }
        return true;
    }

private:
    size_t numRows_;
    size_t numCols_;
    size_t nnz_;
    MatrixFormat format_;

    // CSR storage
    std::vector<double> values_;
    std::vector<size_t> colIndices_;
    std::vector<size_t> rowPtr_;

    // COO storage (temporary during build)
    std::vector<size_t> cooRows_;
    std::vector<size_t> cooCols_;
    std::vector<double> cooVals_;
};

/**
 * @brief Linear system Ax = b
 */
class LinearSystem {
public:
    LinearSystem() = default;

    LinearSystem(size_t size)
        : A_(size, size), b_(size), x_(size) {}

    /**
     * @brief Get system matrix
     */
    SparseMatrix& getMatrix() { return A_; }
    const SparseMatrix& getMatrix() const { return A_; }

    /**
     * @brief Get right-hand side
     */
    SolutionVector& getRHS() { return b_; }
    const SolutionVector& getRHS() const { return b_; }

    /**
     * @brief Get solution vector
     */
    SolutionVector& getSolution() { return x_; }
    const SolutionVector& getSolution() const { return x_; }

    /**
     * @brief Set matrix
     */
    void setMatrix(const SparseMatrix& A) { A_ = A; }

    /**
     * @brief Set RHS
     */
    void setRHS(const SolutionVector& b) { b_ = b; }

    /**
     * @brief Set solution
     */
    void setSolution(const SolutionVector& x) { x_ = x; }

    /**
     * @brief Get system size
     */
    size_t size() const { return A_.rows(); }

    /**
     * @brief Resize system
     */
    void resize(size_t newSize) {
        A_ = SparseMatrix(newSize, newSize);
        b_.resize(newSize);
        x_.resize(newSize);
    }

    /**
     * @brief Apply Dirichlet boundary conditions
     * @param dofIndex Degree of freedom index
     * @param value Prescribed value
     */
    void applyDirichletBC(size_t dofIndex, double value) {
        if (dofIndex >= size()) {
            throw std::out_of_range("DOF index out of range");
        }

        // Set row to identity with prescribed value on RHS
        // A[i,i] = 1, A[i,j!=i] = 0, b[i] = value
        // (This is a simplified implementation)
        x_[dofIndex] = value;
    }

    /**
     * @brief Compute residual: r = b - A*x
     */
    double computeResidual() const {
        std::vector<double> Ax;
        A_.multiply(x_.getData(), Ax);

        double residual = 0.0;
        for (size_t i = 0; i < size(); ++i) {
            double r = b_[i] - Ax[i];
            residual += r * r;
        }

        return std::sqrt(residual);
    }

    /**
     * @brief Check if system is well-posed
     */
    bool isWellPosed() const {
        if (A_.rows() != A_.cols()) {
            return false;
        }
        if (b_.size() != A_.rows()) {
            return false;
        }
        if (A_.nnz() == 0) {
            return false;
        }
        return true;
    }

private:
    SparseMatrix A_;        ///< System matrix
    SolutionVector b_;      ///< Right-hand side
    SolutionVector x_;      ///< Solution vector
};

} // namespace pde
} // namespace solver
} // namespace koo

#endif // KOO_SOLVER_PDE_LINEAR_SYSTEM_H
