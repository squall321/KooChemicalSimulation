/**
 * @file ReactionKernels.h
 * @brief GPU kernels for chemical reaction rate calculations
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha1
 * @date 2025-11-06
 *
 * Phase 54: GPU Reaction Kinetics
 *
 * Provides GPU-accelerated kernels for:
 * - Reaction rate calculations (Arrhenius, modified Arrhenius)
 * - Production rate computations
 * - Stiff ODE integration for chemical kinetics
 */

#ifndef KOO_GPU_KINETICS_REACTION_KERNELS_H
#define KOO_GPU_KINETICS_REACTION_KERNELS_H

#include "../Device.h"
#include "../Memory.h"
#include "../Kernel.h"
#include <stdexcept>
#include <cmath>

namespace koo {
namespace gpu {
namespace kinetics {

/**
 * @class KineticsError
 * @brief Exception for kinetics solver errors
 */
class KineticsError : public std::runtime_error {
public:
    explicit KineticsError(const std::string& message)
        : std::runtime_error("Kinetics Error: " + message) {}
};

// ============================================
// GPU Kernels
// ============================================

#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)

/**
 * @brief Arrhenius rate constant kernel
 *
 * k = A * T^beta * exp(-Ea / (R*T))
 */
template<typename T>
__global__ void arrhenius_kernel(
    const T* A,        // Pre-exponential factor (n_reactions)
    const T* beta,     // Temperature exponent (n_reactions)
    const T* Ea,       // Activation energy (J/mol) (n_reactions)
    const T* T_values, // Temperature (K) (n_cells or 1)
    T* k_fwd,         // Forward rate constants (n_reactions * n_cells)
    int n_reactions,
    int n_cells,
    T R                // Gas constant
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int n_total = n_reactions * n_cells;

    if (idx < n_total) {
        int rxn_idx = idx / n_cells;
        int cell_idx = idx % n_cells;

        T T_val = (n_cells == 1) ? T_values[0] : T_values[cell_idx];

        // k = A * T^beta * exp(-Ea / (R*T))
        T k = A[rxn_idx] * pow(T_val, beta[rxn_idx]) * exp(-Ea[rxn_idx] / (R * T_val));

        k_fwd[idx] = k;
    }
}

/**
 * @brief Calculate production rates from reaction rates
 *
 * omega_i = sum_j (nu_ij * ROP_j)
 * where nu_ij = stoichiometric coefficient of species i in reaction j
 * ROP_j = rate of progress of reaction j
 */
template<typename T>
__global__ void production_rates_kernel(
    const T* ROP,           // Rate of progress (n_reactions * n_cells)
    const T* nu_matrix,     // Stoichiometric coefficients (n_species * n_reactions)
    T* omega,               // Production rates (n_species * n_cells)
    int n_species,
    int n_reactions,
    int n_cells
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (idx < n_species * n_cells) {
        int spec_idx = idx / n_cells;
        int cell_idx = idx % n_cells;

        T sum = 0.0;
        for (int j = 0; j < n_reactions; ++j) {
            int nu_idx = spec_idx * n_reactions + j;
            int rop_idx = j * n_cells + cell_idx;
            sum += nu_matrix[nu_idx] * ROP[rop_idx];
        }

        omega[idx] = sum;
    }
}

/**
 * @brief Calculate rate of progress (ROP) for each reaction
 *
 * ROP_j = k_fwd_j * product(C_i^nu_ij) - k_rev_j * product(C_i^nu'_ij)
 *
 * Simplified: ROP_j = k_fwd_j * product(C_i^nu_fwd_ij)
 */
template<typename T>
__global__ void rate_of_progress_kernel(
    const T* k_fwd,         // Forward rate constants (n_reactions * n_cells)
    const T* concentrations, // Species concentrations (n_species * n_cells)
    const int* reactant_indices, // Reactant indices (n_reactions * max_reactants)
    const T* reactant_orders,    // Reaction orders (n_reactions * max_reactants)
    T* ROP,                 // Rate of progress (n_reactions * n_cells)
    int n_reactions,
    int n_species,
    int n_cells,
    int max_reactants
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (idx < n_reactions * n_cells) {
        int rxn_idx = idx / n_cells;
        int cell_idx = idx % n_cells;

        T k = k_fwd[idx];
        T rate = k;

        // Multiply by concentration terms
        for (int r = 0; r < max_reactants; ++r) {
            int reactant_idx = reactant_indices[rxn_idx * max_reactants + r];
            if (reactant_idx < 0) break; // No more reactants

            T order = reactant_orders[rxn_idx * max_reactants + r];
            T conc = concentrations[reactant_idx * n_cells + cell_idx];

            rate *= pow(conc, order);
        }

        ROP[idx] = rate;
    }
}

/**
 * @brief Simple explicit Euler step for kinetics ODE
 *
 * C_new = C_old + dt * omega
 */
template<typename T>
__global__ void euler_step_kernel(
    const T* C_old,    // Old concentrations (n_species * n_cells)
    const T* omega,    // Production rates (n_species * n_cells)
    T* C_new,          // New concentrations (n_species * n_cells)
    T dt,
    int n_species,
    int n_cells
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (idx < n_species * n_cells) {
        T C = C_old[idx] + dt * omega[idx];
        // Ensure non-negative
        C_new[idx] = (C > 0.0) ? C : 0.0;
    }
}

/**
 * @brief SAXPY kernel: y = alpha * x + y
 */
template<typename T>
__global__ void saxpy_kernel(
    const T* x,
    T* y,
    T alpha,
    int n
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        y[idx] = alpha * x[idx] + y[idx];
    }
}

#endif // KOO_CUDA_ENABLED || KOO_HIP_ENABLED

// ============================================
// Host Interface
// ============================================

/**
 * @class ReactionKernels
 * @brief Host interface for GPU reaction rate kernels
 *
 * Phase 54: GPU Reaction Kinetics
 *
 * Provides GPU-accelerated reaction rate calculations for
 * multiple cells/grid points simultaneously.
 */
template<typename T>
class ReactionKernels {
public:
    /**
     * @brief Constructor
     * @param n_species Number of chemical species
     * @param n_reactions Number of reactions
     * @param n_cells Number of cells/grid points
     */
    ReactionKernels(int n_species, int n_reactions, int n_cells = 1)
        : n_species_(n_species),
          n_reactions_(n_reactions),
          n_cells_(n_cells),
          R_(8.314)  // Gas constant J/(mol*K)
    {
        if (n_species <= 0 || n_reactions <= 0 || n_cells <= 0) {
            throw KineticsError("Invalid dimensions");
        }
    }

    /**
     * @brief Set gas constant
     */
    void setGasConstant(T R) { R_ = R; }

    /**
     * @brief Calculate Arrhenius rate constants
     *
     * @param A Pre-exponential factors (n_reactions)
     * @param beta Temperature exponents (n_reactions)
     * @param Ea Activation energies J/mol (n_reactions)
     * @param T_values Temperatures K (n_cells or 1)
     * @param k_fwd Forward rate constants (output, n_reactions * n_cells)
     */
    void calculateArrheniusRates(
        const DeviceMemory<T>& A,
        const DeviceMemory<T>& beta,
        const DeviceMemory<T>& Ea,
        const DeviceMemory<T>& T_values,
        DeviceMemory<T>& k_fwd
    ) const {
        size_t expected_output = static_cast<size_t>(n_reactions_ * n_cells_);
        if (k_fwd.size() != expected_output) {
            k_fwd.resize(expected_output);
        }

#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        int n_total = n_reactions_ * n_cells_;
        auto config = KernelLauncher::make1DConfig(n_total, 256);

        arrhenius_kernel<<<config.gridSize, config.blockSize>>>(
            A.data(), beta.data(), Ea.data(), T_values.data(),
            k_fwd.data(), n_reactions_, n_cells_, R_
        );
        CHECK_GPU(cudaGetLastError());
#else
        // CPU fallback
        auto hostA = A.toHost();
        auto hostBeta = beta.toHost();
        auto hostEa = Ea.toHost();
        auto hostT = T_values.toHost();
        std::vector<T> hostK(n_reactions_ * n_cells_);

        for (int r = 0; r < n_reactions_; ++r) {
            for (int c = 0; c < n_cells_; ++c) {
                T T_val = (n_cells_ == 1) ? hostT[0] : hostT[c];
                T k = hostA[r] * std::pow(T_val, hostBeta[r]) *
                      std::exp(-hostEa[r] / (R_ * T_val));
                hostK[r * n_cells_ + c] = k;
            }
        }

        k_fwd.copyFromHost(hostK.data(), hostK.size());
#endif
    }

    /**
     * @brief Calculate production rates
     *
     * omega_i = sum_j (nu_ij * ROP_j)
     */
    void calculateProductionRates(
        const DeviceMemory<T>& ROP,
        const DeviceMemory<T>& nu_matrix,
        DeviceMemory<T>& omega
    ) const {
        size_t expected_output = static_cast<size_t>(n_species_ * n_cells_);
        if (omega.size() != expected_output) {
            omega.resize(expected_output);
        }

#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        auto config = KernelLauncher::make1DConfig(expected_output, 256);

        production_rates_kernel<<<config.gridSize, config.blockSize>>>(
            ROP.data(), nu_matrix.data(), omega.data(),
            n_species_, n_reactions_, n_cells_
        );
        CHECK_GPU(cudaGetLastError());
#else
        // CPU fallback
        auto hostROP = ROP.toHost();
        auto hostNu = nu_matrix.toHost();
        std::vector<T> hostOmega(expected_output, 0);

        for (int s = 0; s < n_species_; ++s) {
            for (int c = 0; c < n_cells_; ++c) {
                T sum = 0.0;
                for (int r = 0; r < n_reactions_; ++r) {
                    int nu_idx = s * n_reactions_ + r;
                    int rop_idx = r * n_cells_ + c;
                    sum += hostNu[nu_idx] * hostROP[rop_idx];
                }
                hostOmega[s * n_cells_ + c] = sum;
            }
        }

        omega.copyFromHost(hostOmega.data(), hostOmega.size());
#endif
    }

    /**
     * @brief Explicit Euler step
     *
     * C_new = C_old + dt * omega
     */
    void eulerStep(
        const DeviceMemory<T>& C_old,
        const DeviceMemory<T>& omega,
        DeviceMemory<T>& C_new,
        T dt
    ) const {
        size_t expected_size = static_cast<size_t>(n_species_ * n_cells_);
        if (C_new.size() != expected_size) {
            C_new.resize(expected_size);
        }

#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        auto config = KernelLauncher::make1DConfig(expected_size, 256);

        euler_step_kernel<<<config.gridSize, config.blockSize>>>(
            C_old.data(), omega.data(), C_new.data(),
            dt, n_species_, n_cells_
        );
        CHECK_GPU(cudaGetLastError());
#else
        // CPU fallback
        auto hostC = C_old.toHost();
        auto hostOmega = omega.toHost();
        std::vector<T> hostCNew(expected_size);

        for (size_t i = 0; i < expected_size; ++i) {
            T C = hostC[i] + dt * hostOmega[i];
            hostCNew[i] = (C > 0.0) ? C : 0.0;
        }

        C_new.copyFromHost(hostCNew.data(), hostCNew.size());
#endif
    }

    /**
     * @brief SAXPY: y = alpha * x + y
     */
    void saxpy(T alpha, const DeviceMemory<T>& x, DeviceMemory<T>& y) const {
        if (x.size() != y.size()) {
            throw KineticsError("SAXPY size mismatch");
        }

#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        auto config = KernelLauncher::make1DConfig(x.size(), 256);

        saxpy_kernel<<<config.gridSize, config.blockSize>>>(
            x.data(), y.data(), alpha, static_cast<int>(x.size())
        );
        CHECK_GPU(cudaGetLastError());
#else
        // CPU fallback
        auto hostX = x.toHost();
        auto hostY = y.toHost();

        for (size_t i = 0; i < hostX.size(); ++i) {
            hostY[i] = alpha * hostX[i] + hostY[i];
        }

        y.copyFromHost(hostY.data(), hostY.size());
#endif
    }

    // Getters
    int getSpeciesCount() const { return n_species_; }
    int getReactionCount() const { return n_reactions_; }
    int getCellCount() const { return n_cells_; }

private:
    int n_species_;
    int n_reactions_;
    int n_cells_;
    T R_;  // Gas constant
};

// Type aliases
using ReactionKernelsF = ReactionKernels<float>;
using ReactionKernelsD = ReactionKernels<double>;

} // namespace kinetics
} // namespace gpu
} // namespace koo

#endif // KOO_GPU_KINETICS_REACTION_KERNELS_H
