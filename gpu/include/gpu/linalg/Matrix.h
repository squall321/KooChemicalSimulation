/**
 * @file Matrix.h
 * @brief GPU sparse matrix operations (CSR format)
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha1
 * @date 2025-11-06
 *
 * Phase 52: GPU Linear Algebra
 *
 * Provides GPU-accelerated sparse matrix operations using cuSPARSE/rocSPARSE.
 * Supports CSR (Compressed Sparse Row) format.
 */

#ifndef KOO_GPU_LINALG_MATRIX_H
#define KOO_GPU_LINALG_MATRIX_H

#include "Vector.h"
#include <map>

// cuSPARSE/rocSPARSE includes
#if defined(KOO_CUDA_ENABLED)
    #include <cusparse_v2.h>
    #define KOO_SPARSE_HANDLE cusparseHandle_t
    #define KOO_SPARSE_STATUS cusparseStatus_t
    #define KOO_SPARSE_SUCCESS CUSPARSE_STATUS_SUCCESS
#elif defined(KOO_HIP_ENABLED)
    #include <rocsparse/rocsparse.h>
    #define KOO_SPARSE_HANDLE rocsparse_handle
    #define KOO_SPARSE_STATUS rocsparse_status
    #define KOO_SPARSE_SUCCESS rocsparse_status_success
    #define cusparseCreate rocsparse_create_handle
    #define cusparseDestroy rocsparse_destroy_handle
#endif

namespace koo {
namespace gpu {
namespace linalg {

/**
 * @class SparseError
 * @brief Exception for sparse matrix errors
 */
class SparseError : public std::runtime_error {
public:
    explicit SparseError(const std::string& message)
        : std::runtime_error("Sparse Error: " + message) {}
};

/**
 * @brief Check SPARSE call and throw on error
 */
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
    #define CHECK_SPARSE(call) \
        do { \
            KOO_SPARSE_STATUS status = call; \
            if (status != KOO_SPARSE_SUCCESS) { \
                throw koo::gpu::linalg::SparseError( \
                    std::string(#call) + " failed with status " + \
                    std::to_string(static_cast<int>(status))); \
            } \
        } while(0)
#else
    #define CHECK_SPARSE(call) call
#endif

/**
 * @class SparseHandle
 * @brief RAII wrapper for cuSPARSE/rocSPARSE handle
 *
 * Phase 52: Sparse Matrix Support
 */
class SparseHandle {
public:
    /**
     * @brief Constructor - create sparse handle
     */
    SparseHandle() {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_SPARSE(cusparseCreate(&handle_));
#else
        handle_ = nullptr;
#endif
    }

    /**
     * @brief Destructor - destroy sparse handle
     */
    ~SparseHandle() {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        if (handle_ != nullptr) {
            cusparseDestroy(handle_);
        }
#endif
    }

    /**
     * @brief Get native handle
     */
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
    KOO_SPARSE_HANDLE get() const { return handle_; }
#else
    void* get() const { return nullptr; }
#endif

    /**
     * @brief Get global sparse handle (singleton)
     */
    static SparseHandle& getGlobal() {
        static SparseHandle globalHandle;
        return globalHandle;
    }

    // Non-copyable
    SparseHandle(const SparseHandle&) = delete;
    SparseHandle& operator=(const SparseHandle&) = delete;

private:
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
    KOO_SPARSE_HANDLE handle_;
#else
    void* handle_;
#endif
};

/**
 * @class GPUSparseMatrixCSR
 * @brief GPU sparse matrix in CSR format
 *
 * Phase 52: GPU Sparse Linear Algebra
 *
 * CSR (Compressed Sparse Row) format:
 * - rowPtr[i] = index of first non-zero in row i
 * - colInd[j] = column index of element j
 * - values[j] = value of element j
 *
 * @tparam T Element type (float or double)
 */
template<typename T>
class GPUSparseMatrixCSR {
public:
    /**
     * @brief Constructor
     * @param numRows Number of rows
     * @param numCols Number of columns
     * @param nnz Number of non-zeros
     */
    GPUSparseMatrixCSR(int numRows, int numCols, int nnz)
        : numRows_(numRows),
          numCols_(numCols),
          nnz_(nnz),
          rowPtr_(numRows + 1),
          colInd_(nnz),
          values_(nnz),
          finalized_(false) {
        rowPtr_.zero();
        colInd_.zero();
        values_.zero();
    }

    /**
     * @brief Constructor from host COO format
     * @param numRows Number of rows
     * @param numCols Number of columns
     * @param rows Row indices
     * @param cols Column indices
     * @param vals Values
     */
    GPUSparseMatrixCSR(int numRows, int numCols,
                      const std::vector<int>& rows,
                      const std::vector<int>& cols,
                      const std::vector<T>& vals)
        : numRows_(numRows),
          numCols_(numCols),
          nnz_(static_cast<int>(vals.size())),
          rowPtr_(numRows + 1),
          colInd_(nnz_),
          values_(nnz_),
          finalized_(false) {

        if (rows.size() != cols.size() || rows.size() != vals.size()) {
            throw SparseError("Size mismatch in COO data");
        }

        // Convert COO to CSR on CPU, then copy to GPU
        convertCOOtoCSR(rows, cols, vals);
        finalized_ = true;
    }

    /**
     * @brief Get dimensions
     */
    int numRows() const { return numRows_; }
    int numCols() const { return numCols_; }
    int nnz() const { return nnz_; }

    /**
     * @brief Check if finalized
     */
    bool isFinalized() const { return finalized_; }

    /**
     * @brief Get device arrays
     */
    DeviceMemory<int>& rowPtr() { return rowPtr_; }
    DeviceMemory<int>& colInd() { return colInd_; }
    DeviceMemory<T>& values() { return values_; }

    const DeviceMemory<int>& rowPtr() const { return rowPtr_; }
    const DeviceMemory<int>& colInd() const { return colInd_; }
    const DeviceMemory<T>& values() const { return values_; }

    /**
     * @brief Sparse matrix-vector product: y = A * x
     * @param x Input vector
     * @param y Output vector
     * @param alpha Scalar multiplier (default: 1.0)
     * @param beta Scalar for y (default: 0.0)
     */
    void spmv(const GPUVector<T>& x, GPUVector<T>& y,
              T alpha = 1.0, T beta = 0.0) const {

        if (!finalized_) {
            throw SparseError("Matrix must be finalized before spmv");
        }

        if (x.size() != static_cast<size_t>(numCols_)) {
            throw SparseError("Input vector size mismatch");
        }

        if (y.size() != static_cast<size_t>(numRows_)) {
            y.resize(numRows_);
        }

#if defined(KOO_CUDA_ENABLED)
        auto& handle = SparseHandle::getGlobal();

        // Create matrix descriptor
        cusparseMatDescr_t descr;
        cusparseCreateMatDescr(&descr);
        cusparseSetMatType(descr, CUSPARSE_MATRIX_TYPE_GENERAL);
        cusparseSetMatIndexBase(descr, CUSPARSE_INDEX_BASE_ZERO);

        if constexpr (std::is_same_v<T, float>) {
            CHECK_SPARSE(cusparseScsrmv(
                handle.get(),
                CUSPARSE_OPERATION_NON_TRANSPOSE,
                numRows_, numCols_, nnz_,
                &alpha,
                descr,
                values_.data(), rowPtr_.data(), colInd_.data(),
                x.ptr(),
                &beta,
                y.ptr()
            ));
        } else if constexpr (std::is_same_v<T, double>) {
            CHECK_SPARSE(cusparseDcsrmv(
                handle.get(),
                CUSPARSE_OPERATION_NON_TRANSPOSE,
                numRows_, numCols_, nnz_,
                &alpha,
                descr,
                values_.data(), rowPtr_.data(), colInd_.data(),
                x.ptr(),
                &beta,
                y.ptr()
            ));
        }

        cusparseDestroyMatDescr(descr);

#elif defined(KOO_HIP_ENABLED)
        auto& handle = SparseHandle::getGlobal();

        // Create matrix descriptor
        rocsparse_mat_descr descr;
        rocsparse_create_mat_descr(&descr);
        rocsparse_set_mat_type(descr, rocsparse_matrix_type_general);
        rocsparse_set_mat_index_base(descr, rocsparse_index_base_zero);

        if constexpr (std::is_same_v<T, float>) {
            CHECK_SPARSE(rocsparse_scsrmv(
                handle.get(),
                rocsparse_operation_none,
                numRows_, numCols_, nnz_,
                &alpha,
                descr,
                values_.data(), rowPtr_.data(), colInd_.data(),
                nullptr,  // info
                x.ptr(),
                &beta,
                y.ptr()
            ));
        } else if constexpr (std::is_same_v<T, double>) {
            CHECK_SPARSE(rocsparse_dcsrmv(
                handle.get(),
                rocsparse_operation_none,
                numRows_, numCols_, nnz_,
                &alpha,
                descr,
                values_.data(), rowPtr_.data(), colInd_.data(),
                nullptr,  // info
                x.ptr(),
                &beta,
                y.ptr()
            ));
        }

        rocsparse_destroy_mat_descr(descr);

#else
        // CPU fallback - simple CSR SpMV
        auto hostRowPtr = rowPtr_.toHost();
        auto hostColInd = colInd_.toHost();
        auto hostValues = values_.toHost();
        auto hostX = x.toHost();
        std::vector<T> hostY(numRows_, 0);

        for (int i = 0; i < numRows_; ++i) {
            T sum = 0;
            for (int j = hostRowPtr[i]; j < hostRowPtr[i + 1]; ++j) {
                sum += hostValues[j] * hostX[hostColInd[j]];
            }
            hostY[i] = alpha * sum + beta * (y.size() > 0 ? y.toHost()[i] : 0);
        }

        y.fromHost(hostY);
#endif
    }

    /**
     * @brief Transpose: A^T
     * @return Transposed matrix
     */
    GPUSparseMatrixCSR<T> transpose() const {
        if (!finalized_) {
            throw SparseError("Matrix must be finalized before transpose");
        }

        // Convert to COO on CPU, transpose, convert back to CSR
        auto hostRowPtr = rowPtr_.toHost();
        auto hostColInd = colInd_.toHost();
        auto hostValues = values_.toHost();

        std::vector<int> rows, cols;
        std::vector<T> vals;

        // CSR to COO
        for (int i = 0; i < numRows_; ++i) {
            for (int j = hostRowPtr[i]; j < hostRowPtr[i + 1]; ++j) {
                rows.push_back(i);
                cols.push_back(hostColInd[j]);
                vals.push_back(hostValues[j]);
            }
        }

        // Transpose: swap rows and cols
        std::swap(rows, cols);

        // Create transposed matrix
        return GPUSparseMatrixCSR<T>(numCols_, numRows_, rows, cols, vals);
    }

private:
    int numRows_;
    int numCols_;
    int nnz_;
    DeviceMemory<int> rowPtr_;  // Size: numRows + 1
    DeviceMemory<int> colInd_;  // Size: nnz
    DeviceMemory<T> values_;    // Size: nnz
    bool finalized_;

    /**
     * @brief Convert COO to CSR on CPU
     */
    void convertCOOtoCSR(const std::vector<int>& rows,
                        const std::vector<int>& cols,
                        const std::vector<T>& vals) {

        // Create COO triplets with row-major ordering
        std::vector<std::tuple<int, int, T>> triplets;
        for (size_t i = 0; i < rows.size(); ++i) {
            triplets.emplace_back(rows[i], cols[i], vals[i]);
        }

        // Sort by row, then column
        std::sort(triplets.begin(), triplets.end(),
                 [](const auto& a, const auto& b) {
                     if (std::get<0>(a) != std::get<0>(b))
                         return std::get<0>(a) < std::get<0>(b);
                     return std::get<1>(a) < std::get<1>(b);
                 });

        // Build CSR arrays
        std::vector<int> hostRowPtr(numRows_ + 1, 0);
        std::vector<int> hostColInd(nnz_);
        std::vector<T> hostValues(nnz_);

        int currentRow = -1;
        for (size_t i = 0; i < triplets.size(); ++i) {
            int row = std::get<0>(triplets[i]);
            int col = std::get<1>(triplets[i]);
            T val = std::get<2>(triplets[i]);

            // Update row pointer
            while (currentRow < row) {
                currentRow++;
                hostRowPtr[currentRow] = static_cast<int>(i);
            }

            hostColInd[i] = col;
            hostValues[i] = val;
        }

        // Fill remaining row pointers
        while (currentRow < numRows_) {
            currentRow++;
            hostRowPtr[currentRow] = nnz_;
        }

        // Copy to GPU
        rowPtr_.copyFromHost(hostRowPtr.data(), hostRowPtr.size());
        colInd_.copyFromHost(hostColInd.data(), hostColInd.size());
        values_.copyFromHost(hostValues.data(), hostValues.size());
    }
};

// Type aliases
using GPUSparseMatrixF = GPUSparseMatrixCSR<float>;
using GPUSparseMatrixD = GPUSparseMatrixCSR<double>;

} // namespace linalg
} // namespace gpu
} // namespace koo

#endif // KOO_GPU_LINALG_MATRIX_H
