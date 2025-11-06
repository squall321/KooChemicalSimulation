/**
 * @file DiffusionKernels.h
 * @brief GPU kernels for diffusion equations
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha1
 * @date 2025-11-06
 *
 * Phase 53: GPU Diffusion Solvers
 *
 * Provides GPU-accelerated kernels for diffusion equation discretization.
 * Supports 1D, 2D, and 3D domains with various boundary conditions.
 */

#ifndef KOO_GPU_DIFFUSION_KERNELS_H
#define KOO_GPU_DIFFUSION_KERNELS_H

#include "../Device.h"
#include "../Memory.h"
#include "../Kernel.h"
#include <stdexcept>

namespace koo {
namespace gpu {
namespace diffusion {

/**
 * @brief Boundary condition types
 */
enum class BoundaryType {
    DIRICHLET,      // Fixed value
    NEUMANN,        // Fixed gradient (zero flux when value=0)
    PERIODIC        // Periodic boundary
};

/**
 * @class DiffusionError
 * @brief Exception for diffusion solver errors
 */
class DiffusionError : public std::runtime_error {
public:
    explicit DiffusionError(const std::string& message)
        : std::runtime_error("Diffusion Error: " + message) {}
};

// ============================================
// 1D Diffusion Kernels
// ============================================

#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)

/**
 * @brief 1D Laplacian kernel
 *
 * Computes: laplacian[i] = (u[i+1] - 2*u[i] + u[i-1]) / dx²
 */
template<typename T>
__global__ void laplacian1D_kernel(
    const T* u,
    T* laplacian,
    int nx,
    T dx_inv_sq
) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;

    if (i > 0 && i < nx - 1) {
        laplacian[i] = (u[i+1] - 2.0 * u[i] + u[i-1]) * dx_inv_sq;
    }
}

/**
 * @brief 1D explicit Euler step kernel
 *
 * Computes: u_new[i] = u[i] + dt * D * laplacian[i]
 */
template<typename T>
__global__ void explicitEuler1D_kernel(
    const T* u,
    const T* laplacian,
    T* u_new,
    int nx,
    T dt,
    T D
) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;

    if (i > 0 && i < nx - 1) {
        u_new[i] = u[i] + dt * D * laplacian[i];
    }
}

/**
 * @brief 1D combined Laplacian + Euler step kernel (fused)
 *
 * More efficient - combines two passes into one
 */
template<typename T>
__global__ void diffusionStep1D_kernel(
    const T* u,
    T* u_new,
    int nx,
    T dt,
    T D,
    T dx_inv_sq
) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;

    if (i > 0 && i < nx - 1) {
        T laplacian = (u[i+1] - 2.0 * u[i] + u[i-1]) * dx_inv_sq;
        u_new[i] = u[i] + dt * D * laplacian;
    }
}

/**
 * @brief Apply 1D boundary conditions kernel
 */
template<typename T>
__global__ void applyBC1D_kernel(
    T* u,
    int nx,
    BoundaryType bcLeft,
    BoundaryType bcRight,
    T valueLeft,
    T valueRight
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (idx == 0) {
        // Left boundary
        if (bcLeft == BoundaryType::DIRICHLET) {
            u[0] = valueLeft;
        } else if (bcLeft == BoundaryType::NEUMANN) {
            u[0] = u[1];  // Zero gradient approximation
        } else if (bcLeft == BoundaryType::PERIODIC) {
            u[0] = u[nx-2];
        }

        // Right boundary
        if (bcRight == BoundaryType::DIRICHLET) {
            u[nx-1] = valueRight;
        } else if (bcRight == BoundaryType::NEUMANN) {
            u[nx-1] = u[nx-2];
        } else if (bcRight == BoundaryType::PERIODIC) {
            u[nx-1] = u[1];
        }
    }
}

// ============================================
// 2D Diffusion Kernels
// ============================================

/**
 * @brief 2D Laplacian kernel
 *
 * Computes: laplacian[i,j] = (u[i+1,j] + u[i-1,j] + u[i,j+1] + u[i,j-1] - 4*u[i,j]) / h²
 */
template<typename T>
__global__ void laplacian2D_kernel(
    const T* u,
    T* laplacian,
    int nx,
    int ny,
    T dx_inv_sq,
    T dy_inv_sq
) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int j = blockIdx.y * blockDim.y + threadIdx.y;

    if (i > 0 && i < nx - 1 && j > 0 && j < ny - 1) {
        int idx = j * nx + i;

        T d2u_dx2 = (u[idx+1] - 2.0 * u[idx] + u[idx-1]) * dx_inv_sq;
        T d2u_dy2 = (u[idx+nx] - 2.0 * u[idx] + u[idx-nx]) * dy_inv_sq;

        laplacian[idx] = d2u_dx2 + d2u_dy2;
    }
}

/**
 * @brief 2D diffusion step kernel (fused)
 */
template<typename T>
__global__ void diffusionStep2D_kernel(
    const T* u,
    T* u_new,
    int nx,
    int ny,
    T dt,
    T D,
    T dx_inv_sq,
    T dy_inv_sq
) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int j = blockIdx.y * blockDim.y + threadIdx.y;

    if (i > 0 && i < nx - 1 && j > 0 && j < ny - 1) {
        int idx = j * nx + i;

        T d2u_dx2 = (u[idx+1] - 2.0 * u[idx] + u[idx-1]) * dx_inv_sq;
        T d2u_dy2 = (u[idx+nx] - 2.0 * u[idx] + u[idx-nx]) * dy_inv_sq;
        T laplacian = d2u_dx2 + d2u_dy2;

        u_new[idx] = u[idx] + dt * D * laplacian;
    }
}

// ============================================
// 3D Diffusion Kernels
// ============================================

/**
 * @brief 3D diffusion step kernel (fused)
 */
template<typename T>
__global__ void diffusionStep3D_kernel(
    const T* u,
    T* u_new,
    int nx,
    int ny,
    int nz,
    T dt,
    T D,
    T dx_inv_sq,
    T dy_inv_sq,
    T dz_inv_sq
) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int j = blockIdx.y * blockDim.y + threadIdx.y;
    int k = blockIdx.z * blockDim.z + threadIdx.z;

    if (i > 0 && i < nx - 1 && j > 0 && j < ny - 1 && k > 0 && k < nz - 1) {
        int idx = (k * ny + j) * nx + i;
        int nxy = nx * ny;

        T d2u_dx2 = (u[idx+1] - 2.0 * u[idx] + u[idx-1]) * dx_inv_sq;
        T d2u_dy2 = (u[idx+nx] - 2.0 * u[idx] + u[idx-nx]) * dy_inv_sq;
        T d2u_dz2 = (u[idx+nxy] - 2.0 * u[idx] + u[idx-nxy]) * dz_inv_sq;
        T laplacian = d2u_dx2 + d2u_dy2 + d2u_dz2;

        u_new[idx] = u[idx] + dt * D * laplacian;
    }
}

#endif // KOO_CUDA_ENABLED || KOO_HIP_ENABLED

// ============================================
// Host Interface Classes
// ============================================

/**
 * @class DiffusionKernels1D
 * @brief Host interface for 1D diffusion GPU kernels
 *
 * Phase 53: GPU Diffusion Kernels
 */
template<typename T>
class DiffusionKernels1D {
public:
    /**
     * @brief Constructor
     * @param nx Grid points
     * @param dx Grid spacing
     */
    DiffusionKernels1D(int nx, T dx)
        : nx_(nx), dx_(dx), dx_inv_sq_(1.0 / (dx * dx)) {

        if (nx < 3) {
            throw DiffusionError("Grid too small (nx >= 3 required)");
        }
    }

    /**
     * @brief Compute Laplacian
     */
    void laplacian(const DeviceMemory<T>& u, DeviceMemory<T>& lap) const {
        if (u.size() != static_cast<size_t>(nx_)) {
            throw DiffusionError("Input size mismatch");
        }
        if (lap.size() != static_cast<size_t>(nx_)) {
            lap.resize(nx_);
        }

#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        auto config = KernelLauncher::make1DConfig(nx_, 256);
        laplacian1D_kernel<<<config.gridSize, config.blockSize>>>(
            u.data(), lap.data(), nx_, dx_inv_sq_
        );
        CHECK_GPU(cudaGetLastError());
#else
        // CPU fallback
        auto hostU = u.toHost();
        std::vector<T> hostLap(nx_, 0);

        for (int i = 1; i < nx_ - 1; ++i) {
            hostLap[i] = (hostU[i+1] - 2.0 * hostU[i] + hostU[i-1]) * dx_inv_sq_;
        }

        lap.copyFromHost(hostLap.data(), hostLap.size());
#endif
    }

    /**
     * @brief Single diffusion step (fused kernel)
     * @param u Current field
     * @param u_new Output field
     * @param dt Time step
     * @param D Diffusion coefficient
     */
    void step(const DeviceMemory<T>& u, DeviceMemory<T>& u_new,
              T dt, T D) const {

        if (u.size() != static_cast<size_t>(nx_)) {
            throw DiffusionError("Input size mismatch");
        }
        if (u_new.size() != static_cast<size_t>(nx_)) {
            u_new.resize(nx_);
        }

#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        auto config = KernelLauncher::make1DConfig(nx_, 256);
        diffusionStep1D_kernel<<<config.gridSize, config.blockSize>>>(
            u.data(), u_new.data(), nx_, dt, D, dx_inv_sq_
        );
        CHECK_GPU(cudaGetLastError());
#else
        // CPU fallback
        auto hostU = u.toHost();
        std::vector<T> hostUNew(nx_);

        for (int i = 1; i < nx_ - 1; ++i) {
            T laplacian = (hostU[i+1] - 2.0 * hostU[i] + hostU[i-1]) * dx_inv_sq_;
            hostUNew[i] = hostU[i] + dt * D * laplacian;
        }

        // Copy boundary values
        hostUNew[0] = hostU[0];
        hostUNew[nx_-1] = hostU[nx_-1];

        u_new.copyFromHost(hostUNew.data(), hostUNew.size());
#endif
    }

    /**
     * @brief Apply boundary conditions
     */
    void applyBC(DeviceMemory<T>& u,
                 BoundaryType bcLeft, BoundaryType bcRight,
                 T valueLeft = 0, T valueRight = 0) const {

#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        applyBC1D_kernel<<<1, 1>>>(
            u.data(), nx_, bcLeft, bcRight, valueLeft, valueRight
        );
        CHECK_GPU(cudaGetLastError());
#else
        // CPU fallback
        auto hostU = u.toHost();

        // Left boundary
        if (bcLeft == BoundaryType::DIRICHLET) {
            hostU[0] = valueLeft;
        } else if (bcLeft == BoundaryType::NEUMANN) {
            hostU[0] = hostU[1];
        } else if (bcLeft == BoundaryType::PERIODIC) {
            hostU[0] = hostU[nx_-2];
        }

        // Right boundary
        if (bcRight == BoundaryType::DIRICHLET) {
            hostU[nx_-1] = valueRight;
        } else if (bcRight == BoundaryType::NEUMANN) {
            hostU[nx_-1] = hostU[nx_-2];
        } else if (bcRight == BoundaryType::PERIODIC) {
            hostU[nx_-1] = hostU[1];
        }

        u.copyFromHost(hostU.data(), hostU.size());
#endif
    }

    /**
     * @brief Get CFL number
     */
    T getCFL(T dt, T D) const {
        return D * dt * dx_inv_sq_;
    }

    /**
     * @brief Get maximum stable time step
     */
    T getMaxDt(T D, T cfl_max = 0.5) const {
        return cfl_max / (D * dx_inv_sq_);
    }

private:
    int nx_;
    T dx_;
    T dx_inv_sq_;
};

/**
 * @class DiffusionKernels2D
 * @brief Host interface for 2D diffusion GPU kernels
 */
template<typename T>
class DiffusionKernels2D {
public:
    /**
     * @brief Constructor
     */
    DiffusionKernels2D(int nx, int ny, T dx, T dy)
        : nx_(nx), ny_(ny), dx_(dx), dy_(dy),
          dx_inv_sq_(1.0 / (dx * dx)),
          dy_inv_sq_(1.0 / (dy * dy)) {

        if (nx < 3 || ny < 3) {
            throw DiffusionError("Grid too small (nx,ny >= 3 required)");
        }
    }

    /**
     * @brief Single diffusion step (fused kernel)
     */
    void step(const DeviceMemory<T>& u, DeviceMemory<T>& u_new,
              T dt, T D) const {

        size_t total = static_cast<size_t>(nx_ * ny_);
        if (u.size() != total) {
            throw DiffusionError("Input size mismatch");
        }
        if (u_new.size() != total) {
            u_new.resize(total);
        }

#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        auto config = KernelLauncher::make2DConfig(nx_, ny_, 16, 16);
        diffusionStep2D_kernel<<<config.gridSize, config.blockSize>>>(
            u.data(), u_new.data(), nx_, ny_, dt, D, dx_inv_sq_, dy_inv_sq_
        );
        CHECK_GPU(cudaGetLastError());
#else
        // CPU fallback
        auto hostU = u.toHost();
        std::vector<T> hostUNew(total, 0);

        for (int j = 1; j < ny_ - 1; ++j) {
            for (int i = 1; i < nx_ - 1; ++i) {
                int idx = j * nx_ + i;
                T d2u_dx2 = (hostU[idx+1] - 2.0 * hostU[idx] + hostU[idx-1]) * dx_inv_sq_;
                T d2u_dy2 = (hostU[idx+nx_] - 2.0 * hostU[idx] + hostU[idx-nx_]) * dy_inv_sq_;
                T laplacian = d2u_dx2 + d2u_dy2;
                hostUNew[idx] = hostU[idx] + dt * D * laplacian;
            }
        }

        // Copy boundary values
        for (int i = 0; i < nx_; ++i) {
            hostUNew[i] = hostU[i];  // Bottom
            hostUNew[(ny_-1)*nx_ + i] = hostU[(ny_-1)*nx_ + i];  // Top
        }
        for (int j = 0; j < ny_; ++j) {
            hostUNew[j*nx_] = hostU[j*nx_];  // Left
            hostUNew[j*nx_ + nx_-1] = hostU[j*nx_ + nx_-1];  // Right
        }

        u_new.copyFromHost(hostUNew.data(), hostUNew.size());
#endif
    }

    /**
     * @brief Get CFL number
     */
    T getCFL(T dt, T D) const {
        return D * dt * (dx_inv_sq_ + dy_inv_sq_);
    }

    /**
     * @brief Get maximum stable time step
     */
    T getMaxDt(T D, T cfl_max = 0.5) const {
        return cfl_max / (D * (dx_inv_sq_ + dy_inv_sq_));
    }

private:
    int nx_, ny_;
    T dx_, dy_;
    T dx_inv_sq_, dy_inv_sq_;
};

/**
 * @class DiffusionKernels3D
 * @brief Host interface for 3D diffusion GPU kernels
 */
template<typename T>
class DiffusionKernels3D {
public:
    /**
     * @brief Constructor
     */
    DiffusionKernels3D(int nx, int ny, int nz, T dx, T dy, T dz)
        : nx_(nx), ny_(ny), nz_(nz), dx_(dx), dy_(dy), dz_(dz),
          dx_inv_sq_(1.0 / (dx * dx)),
          dy_inv_sq_(1.0 / (dy * dy)),
          dz_inv_sq_(1.0 / (dz * dz)) {

        if (nx < 3 || ny < 3 || nz < 3) {
            throw DiffusionError("Grid too small (nx,ny,nz >= 3 required)");
        }
    }

    /**
     * @brief Single diffusion step (fused kernel)
     */
    void step(const DeviceMemory<T>& u, DeviceMemory<T>& u_new,
              T dt, T D) const {

        size_t total = static_cast<size_t>(nx_ * ny_ * nz_);
        if (u.size() != total) {
            throw DiffusionError("Input size mismatch");
        }
        if (u_new.size() != total) {
            u_new.resize(total);
        }

#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        auto config = KernelLauncher::make3DConfig(nx_, ny_, nz_, 8, 8, 8);
        diffusionStep3D_kernel<<<config.gridSize, config.blockSize>>>(
            u.data(), u_new.data(), nx_, ny_, nz_, dt, D,
            dx_inv_sq_, dy_inv_sq_, dz_inv_sq_
        );
        CHECK_GPU(cudaGetLastError());
#else
        // CPU fallback - simplified for brevity
        throw DiffusionError("3D CPU fallback not yet implemented");
#endif
    }

    /**
     * @brief Get CFL number
     */
    T getCFL(T dt, T D) const {
        return D * dt * (dx_inv_sq_ + dy_inv_sq_ + dz_inv_sq_);
    }

    /**
     * @brief Get maximum stable time step
     */
    T getMaxDt(T D, T cfl_max = 0.5) const {
        return cfl_max / (D * (dx_inv_sq_ + dy_inv_sq_ + dz_inv_sq_));
    }

private:
    int nx_, ny_, nz_;
    T dx_, dy_, dz_;
    T dx_inv_sq_, dy_inv_sq_, dz_inv_sq_;
};

// Type aliases
using DiffusionKernels1DF = DiffusionKernels1D<float>;
using DiffusionKernels1DD = DiffusionKernels1D<double>;
using DiffusionKernels2DF = DiffusionKernels2D<float>;
using DiffusionKernels2DD = DiffusionKernels2D<double>;
using DiffusionKernels3DF = DiffusionKernels3D<float>;
using DiffusionKernels3DD = DiffusionKernels3D<double>;

} // namespace diffusion
} // namespace gpu
} // namespace koo

#endif // KOO_GPU_DIFFUSION_KERNELS_H
