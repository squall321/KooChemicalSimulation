/**
 * @file CommonTypes.h
 * @brief Common mathematical types for the framework
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-alpha3
 * @date 2025-11-06
 *
 * This file defines common mathematical types used throughout the framework,
 * including vectors, matrices, and tensors. These types are built on Eigen3
 * for high-performance linear algebra operations.
 */

#ifndef KOO_CORE_TYPES_COMMON_TYPES_H
#define KOO_CORE_TYPES_COMMON_TYPES_H

#include <vector>
#include <array>
#include <cstddef>
#include <cmath>
#include <stdexcept>
#include <string>

// Note: Eigen3 is optional in Phase 1-2, but becomes required from Phase 3+
// If Eigen3 is not available, simple fallback implementations are provided
#ifdef USE_EIGEN
#include <Eigen/Dense>
#include <Eigen/Sparse>
#endif

namespace koo {
namespace core {
namespace types {

// ============================================================================
// Type Aliases for Eigen Types (when available)
// ============================================================================

#ifdef USE_EIGEN

/**
 * @brief 2D vector (double precision)
 */
using Vector2D = Eigen::Vector2d;

/**
 * @brief 3D vector (double precision)
 */
using Vector3D = Eigen::Vector3d;

/**
 * @brief 4D vector (double precision)
 */
using Vector4D = Eigen::Vector4d;

/**
 * @brief Dynamic-size vector (double precision)
 */
using VectorXd = Eigen::VectorXd;

/**
 * @brief 2x2 matrix (double precision)
 */
using Matrix2D = Eigen::Matrix2d;

/**
 * @brief 3x3 matrix (double precision)
 */
using Matrix3D = Eigen::Matrix3d;

/**
 * @brief 4x4 matrix (double precision)
 */
using Matrix4D = Eigen::Matrix4d;

/**
 * @brief Dynamic-size matrix (double precision)
 */
using MatrixXd = Eigen::MatrixXd;

/**
 * @brief Sparse matrix (double precision)
 */
using SparseMatrix = Eigen::SparseMatrix<double>;

/**
 * @brief Triplet for sparse matrix construction
 */
using Triplet = Eigen::Triplet<double>;

#else

// ============================================================================
// Fallback Implementations (when Eigen is not available)
// ============================================================================

/**
 * @brief Simple 3D vector implementation (fallback)
 *
 * This is a minimal implementation used when Eigen3 is not available.
 * For production use, Eigen3 is strongly recommended.
 */
class Vector3D {
public:
    /**
     * @brief Default constructor - initializes to zero
     */
    Vector3D() : data_{0.0, 0.0, 0.0} {}

    /**
     * @brief Constructor with individual components
     */
    Vector3D(double x, double y, double z) : data_{x, y, z} {}

    /**
     * @brief Access element by index
     */
    double& operator[](size_t i) { return data_[i]; }

    /**
     * @brief Access element by index (const)
     */
    const double& operator[](size_t i) const { return data_[i]; }

    /**
     * @brief Get x component
     */
    double x() const { return data_[0]; }

    /**
     * @brief Get y component
     */
    double y() const { return data_[1]; }

    /**
     * @brief Get z component
     */
    double z() const { return data_[2]; }

    /**
     * @brief Set x component
     */
    void setX(double x) { data_[0] = x; }

    /**
     * @brief Set y component
     */
    void setY(double y) { data_[1] = y; }

    /**
     * @brief Set z component
     */
    void setZ(double z) { data_[2] = z; }

    /**
     * @brief Vector addition
     */
    Vector3D operator+(const Vector3D& other) const {
        return Vector3D(data_[0] + other.data_[0],
                       data_[1] + other.data_[1],
                       data_[2] + other.data_[2]);
    }

    /**
     * @brief Vector subtraction
     */
    Vector3D operator-(const Vector3D& other) const {
        return Vector3D(data_[0] - other.data_[0],
                       data_[1] - other.data_[1],
                       data_[2] - other.data_[2]);
    }

    /**
     * @brief Scalar multiplication
     */
    Vector3D operator*(double scalar) const {
        return Vector3D(data_[0] * scalar,
                       data_[1] * scalar,
                       data_[2] * scalar);
    }

    /**
     * @brief Scalar division
     */
    Vector3D operator/(double scalar) const {
        if (std::abs(scalar) < 1e-15) {
            throw std::runtime_error("Vector3D: Division by zero or near-zero scalar");
        }
        return Vector3D(data_[0] / scalar,
                       data_[1] / scalar,
                       data_[2] / scalar);
    }

    /**
     * @brief Dot product
     */
    double dot(const Vector3D& other) const {
        return data_[0] * other.data_[0] +
               data_[1] * other.data_[1] +
               data_[2] * other.data_[2];
    }

    /**
     * @brief Cross product
     */
    Vector3D cross(const Vector3D& other) const {
        return Vector3D(
            data_[1] * other.data_[2] - data_[2] * other.data_[1],
            data_[2] * other.data_[0] - data_[0] * other.data_[2],
            data_[0] * other.data_[1] - data_[1] * other.data_[0]
        );
    }

    /**
     * @brief Vector norm (length)
     */
    double norm() const {
        return std::sqrt(dot(*this));
    }

    /**
     * @brief Normalize the vector
     * @throws std::runtime_error if vector length is too small
     */
    Vector3D normalized() const {
        double len = norm();
        if (len < 1e-15) {
            throw std::runtime_error("Cannot normalize zero or near-zero vector (length = " + std::to_string(len) + ")");
        }
        return *this / len;
    }

    /**
     * @brief Get raw data pointer
     */
    double* data() { return data_.data(); }

    /**
     * @brief Get raw data pointer (const)
     */
    const double* data() const { return data_.data(); }

private:
    std::array<double, 3> data_;
};

/**
 * @brief Simple dynamic vector implementation (fallback)
 */
class VectorXd {
public:
    /**
     * @brief Constructor with size
     */
    explicit VectorXd(size_t size = 0) : data_(size, 0.0) {}

    /**
     * @brief Constructor from std::vector
     */
    explicit VectorXd(const std::vector<double>& data) : data_(data) {}

    /**
     * @brief Access element
     */
    double& operator[](size_t i) { return data_[i]; }

    /**
     * @brief Access element (const)
     */
    const double& operator[](size_t i) const { return data_[i]; }

    /**
     * @brief Get size
     */
    size_t size() const { return data_.size(); }

    /**
     * @brief Resize vector
     */
    void resize(size_t newSize) { data_.resize(newSize, 0.0); }

    /**
     * @brief Get raw data pointer
     */
    double* data() { return data_.data(); }

    /**
     * @brief Get raw data pointer (const)
     */
    const double* data() const { return data_.data(); }

private:
    std::vector<double> data_;
};

/**
 * @brief Simple matrix implementation (fallback)
 */
class MatrixXd {
public:
    /**
     * @brief Constructor with dimensions
     */
    MatrixXd(size_t rows = 0, size_t cols = 0)
        : rows_(rows), cols_(cols), data_(rows * cols, 0.0) {}

    /**
     * @brief Access element
     */
    double& operator()(size_t i, size_t j) {
        if (i >= rows_ || j >= cols_) {
            throw std::out_of_range("Matrix index out of range: (" + std::to_string(i) +
                                   ", " + std::to_string(j) + ") for matrix of size (" +
                                   std::to_string(rows_) + ", " + std::to_string(cols_) + ")");
        }
        return data_[i * cols_ + j];
    }

    /**
     * @brief Access element (const)
     */
    const double& operator()(size_t i, size_t j) const {
        if (i >= rows_ || j >= cols_) {
            throw std::out_of_range("Matrix index out of range: (" + std::to_string(i) +
                                   ", " + std::to_string(j) + ") for matrix of size (" +
                                   std::to_string(rows_) + ", " + std::to_string(cols_) + ")");
        }
        return data_[i * cols_ + j];
    }

    /**
     * @brief Get number of rows
     */
    size_t rows() const { return rows_; }

    /**
     * @brief Get number of columns
     */
    size_t cols() const { return cols_; }

    /**
     * @brief Resize matrix
     */
    void resize(size_t rows, size_t cols) {
        rows_ = rows;
        cols_ = cols;
        data_.resize(rows * cols, 0.0);
    }

    /**
     * @brief Get raw data pointer
     */
    double* data() { return data_.data(); }

    /**
     * @brief Get raw data pointer (const)
     */
    const double* data() const { return data_.data(); }

private:
    size_t rows_;
    size_t cols_;
    std::vector<double> data_;
};

// Additional type aliases for consistency
using Vector2D = std::array<double, 2>;
using Vector4D = std::array<double, 4>;
using Matrix2D = std::array<std::array<double, 2>, 2>;
using Matrix3D = std::array<std::array<double, 3>, 3>;
using Matrix4D = std::array<std::array<double, 4>, 4>;

#endif // USE_EIGEN

// ============================================================================
// Common Type Aliases (independent of Eigen)
// ============================================================================

/**
 * @brief Integer type for indices
 */
using Index = std::size_t;

/**
 * @brief Integer type for signed indices
 */
using SignedIndex = std::ptrdiff_t;

/**
 * @brief Real number type (double precision)
 */
using Real = double;

/**
 * @brief Complex number type (if needed in future)
 */
// using Complex = std::complex<double>;

/**
 * @brief Array of indices
 */
using IndexArray = std::vector<Index>;

/**
 * @brief Array of real numbers
 */
using RealArray = std::vector<Real>;

// ============================================================================
// Tensor Types (for future use)
// ============================================================================

/**
 * @brief 3rd order tensor (3x3x3)
 *
 * Used for representing material properties with directional dependence.
 * Layout: Tensor3D[i][j][k]
 */
using Tensor3D = std::array<std::array<std::array<double, 3>, 3>, 3>;

/**
 * @brief 4th order tensor (3x3x3x3)
 *
 * Used for elasticity tensors and similar applications.
 * Layout: Tensor4D[i][j][k][l]
 */
using Tensor4D = std::array<std::array<std::array<std::array<double, 3>, 3>, 3>, 3>;

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Create zero vector of given size
 */
inline VectorXd zeros(size_t size) {
    VectorXd result(size);
    for (size_t i = 0; i < size; ++i) {
        result[i] = 0.0;
    }
    return result;
}

/**
 * @brief Create vector of ones
 */
inline VectorXd ones(size_t size) {
    VectorXd result(size);
    for (size_t i = 0; i < size; ++i) {
        result[i] = 1.0;
    }
    return result;
}

/**
 * @brief Create zero matrix
 */
#ifndef USE_EIGEN
inline MatrixXd zeros(size_t rows, size_t cols) {
    return MatrixXd(rows, cols);
}
#endif

} // namespace types
} // namespace core
} // namespace koo

#endif // KOO_CORE_TYPES_COMMON_TYPES_H
