/**
 * @file multi_physics_example.cpp
 * @brief Multi-physics coupling example: Thermal + Chemical + Flow
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha4
 * @date 2025-11-07
 *
 * This example demonstrates:
 * - Thermal conduction with chemical heat release
 * - Arrhenius reaction kinetics (temperature-dependent)
 * - Flow advection of species
 * - Operator splitting for multi-physics coupling
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <chrono>

// Physical constants
constexpr double R_GAS = 8.314;  // J/(mol·K)

/**
 * @brief Multi-physics simulator with thermal-chemical-flow coupling
 */
class MultiPhysicsSimulator {
private:
    // Grid parameters
    int nx_, ny_;
    double Lx_, Ly_;
    double dx_, dy_;
    double dt_;

    // Field variables
    std::vector<std::vector<double>> T_;      // Temperature (K)
    std::vector<std::vector<double>> C_;      // Concentration (mol/m³)
    std::vector<std::vector<double>> u_;      // X-velocity (m/s)
    std::vector<std::vector<double>> v_;      // Y-velocity (m/s)

    // Temporary arrays
    std::vector<std::vector<double>> T_new_;
    std::vector<std::vector<double>> C_new_;

    // Physical parameters
    double k_thermal_;     // Thermal conductivity (W/(m·K))
    double rho_;           // Density (kg/m³)
    double cp_;            // Specific heat (J/(kg·K))
    double D_chemical_;    // Diffusion coefficient (m²/s)
    double A_;             // Pre-exponential factor (1/s)
    double Ea_;            // Activation energy (J/mol)
    double deltaH_;        // Heat of reaction (J/mol)

    int time_step_;

public:
    MultiPhysicsSimulator(int nx, int ny, double Lx, double Ly, double dt)
        : nx_(nx), ny_(ny), Lx_(Lx), Ly_(Ly), dt_(dt), time_step_(0)
    {
        dx_ = Lx_ / (nx_ - 1);
        dy_ = Ly_ / (ny_ - 1);

        // Allocate arrays
        T_.resize(nx_, std::vector<double>(ny_, 300.0));     // Initial T = 300K
        C_.resize(nx_, std::vector<double>(ny_, 1.0));       // Initial C = 1.0 mol/m³
        u_.resize(nx_, std::vector<double>(ny_, 0.0));
        v_.resize(nx_, std::vector<double>(ny_, 0.0));

        T_new_.resize(nx_, std::vector<double>(ny_));
        C_new_.resize(nx_, std::vector<double>(ny_));

        // Physical parameters (example values)
        k_thermal_ = 0.025;      // Air thermal conductivity
        rho_ = 1.2;              // Air density
        cp_ = 1005.0;            // Air specific heat
        D_chemical_ = 1e-5;      // Chemical diffusion
        A_ = 1e8;                // Pre-exponential factor
        Ea_ = 50000.0;           // Activation energy
        deltaH_ = -200000.0;     // Exothermic reaction

        std::cout << "Multi-physics simulator initialized:\n";
        std::cout << "  Grid: " << nx_ << " x " << ny_ << "\n";
        std::cout << "  Domain: " << Lx_ << " x " << Ly_ << " m\n";
        std::cout << "  dt: " << dt_ << " s\n";
        std::cout << "  CFL thermal: " << k_thermal_/(rho_*cp_) * dt_/(dx_*dx_) << "\n";
        std::cout << "  CFL diffusion: " << D_chemical_ * dt_/(dx_*dx_) << "\n\n";
    }

    /**
     * @brief Set initial hot spot to trigger reaction
     */
    void setHotSpot(int i_center, int j_center, int radius, double T_hot) {
        for (int i = 0; i < nx_; ++i) {
            for (int j = 0; j < ny_; ++j) {
                int di = i - i_center;
                int dj = j - j_center;
                if (di*di + dj*dj < radius*radius) {
                    T_[i][j] = T_hot;
                }
            }
        }
        std::cout << "Hot spot set at (" << i_center << ", " << j_center
                  << ") with T = " << T_hot << " K\n\n";
    }

    /**
     * @brief Set flow field (simple vortex)
     */
    void setVortexFlow(double strength) {
        double cx = Lx_ / 2.0;
        double cy = Ly_ / 2.0;

        for (int i = 0; i < nx_; ++i) {
            for (int j = 0; j < ny_; ++j) {
                double x = i * dx_ - cx;
                double y = j * dy_ - cy;
                double r = std::sqrt(x*x + y*y);

                if (r > 1e-10) {
                    u_[i][j] = -strength * y / r;
                    v_[i][j] =  strength * x / r;
                }
            }
        }
        std::cout << "Vortex flow field set with strength = " << strength << " m/s\n\n";
    }

    /**
     * @brief Compute Arrhenius reaction rate
     */
    double reactionRate(double T, double C) const {
        return A_ * std::exp(-Ea_ / (R_GAS * T)) * C;
    }

    /**
     * @brief Laplacian operator
     */
    double laplacian(const std::vector<std::vector<double>>& field, int i, int j) const {
        double d2x = (field[i+1][j] - 2.0*field[i][j] + field[i-1][j]) / (dx_*dx_);
        double d2y = (field[i][j+1] - 2.0*field[i][j] + field[i][j-1]) / (dy_*dy_);
        return d2x + d2y;
    }

    /**
     * @brief Step 1: Chemical reaction (operator splitting)
     */
    void stepChemistry() {
        for (int i = 1; i < nx_-1; ++i) {
            for (int j = 1; j < ny_-1; ++j) {
                double rate = reactionRate(T_[i][j], C_[i][j]);
                C_[i][j] -= rate * dt_;
                if (C_[i][j] < 0.0) C_[i][j] = 0.0;
            }
        }
    }

    /**
     * @brief Step 2: Thermal conduction with heat release
     */
    void stepThermal() {
        double alpha = k_thermal_ / (rho_ * cp_);

        for (int i = 1; i < nx_-1; ++i) {
            for (int j = 1; j < ny_-1; ++j) {
                // Heat release from reaction
                double rate = reactionRate(T_[i][j], C_[i][j]);
                double Q = -deltaH_ * rate / (rho_ * cp_);

                // Thermal diffusion
                double dT = alpha * laplacian(T_, i, j) + Q;

                T_new_[i][j] = T_[i][j] + dt_ * dT;
            }
        }

        // Boundary conditions (Neumann - insulated)
        for (int i = 0; i < nx_; ++i) {
            T_new_[i][0] = T_new_[i][1];
            T_new_[i][ny_-1] = T_new_[i][ny_-2];
        }
        for (int j = 0; j < ny_; ++j) {
            T_new_[0][j] = T_new_[1][j];
            T_new_[nx_-1][j] = T_new_[nx_-2][j];
        }

        T_ = T_new_;
    }

    /**
     * @brief Step 3: Chemical diffusion
     */
    void stepDiffusion() {
        for (int i = 1; i < nx_-1; ++i) {
            for (int j = 1; j < ny_-1; ++j) {
                double dC = D_chemical_ * laplacian(C_, i, j);
                C_new_[i][j] = C_[i][j] + dt_ * dC;
            }
        }

        // Boundary conditions (Neumann)
        for (int i = 0; i < nx_; ++i) {
            C_new_[i][0] = C_new_[i][1];
            C_new_[i][ny_-1] = C_new_[i][ny_-2];
        }
        for (int j = 0; j < ny_; ++j) {
            C_new_[0][j] = C_new_[1][j];
            C_new_[nx_-1][j] = C_new_[nx_-2][j];
        }

        C_ = C_new_;
    }

    /**
     * @brief Step 4: Flow advection (upwind scheme)
     */
    void stepAdvection() {
        for (int i = 1; i < nx_-1; ++i) {
            for (int j = 1; j < ny_-1; ++j) {
                double dC_dx, dC_dy;

                // Upwind for x-direction
                if (u_[i][j] > 0) {
                    dC_dx = (C_[i][j] - C_[i-1][j]) / dx_;
                } else {
                    dC_dx = (C_[i+1][j] - C_[i][j]) / dx_;
                }

                // Upwind for y-direction
                if (v_[i][j] > 0) {
                    dC_dy = (C_[i][j] - C_[i][j-1]) / dy_;
                } else {
                    dC_dy = (C_[i][j+1] - C_[i][j]) / dy_;
                }

                C_new_[i][j] = C_[i][j] - dt_ * (u_[i][j] * dC_dx + v_[i][j] * dC_dy);
            }
        }

        C_ = C_new_;
    }

    /**
     * @brief Full time step with operator splitting
     */
    void step() {
        stepChemistry();   // 1. Reaction
        stepThermal();     // 2. Heat conduction + release
        stepDiffusion();   // 3. Chemical diffusion
        stepAdvection();   // 4. Flow advection
        time_step_++;
    }

    /**
     * @brief Compute statistics
     */
    void printStatistics() const {
        double T_max = 0.0, T_min = 1e10, T_avg = 0.0;
        double C_total = 0.0;

        for (int i = 0; i < nx_; ++i) {
            for (int j = 0; j < ny_; ++j) {
                T_max = std::max(T_max, T_[i][j]);
                T_min = std::min(T_min, T_[i][j]);
                T_avg += T_[i][j];
                C_total += C_[i][j];
            }
        }

        T_avg /= (nx_ * ny_);
        C_total *= dx_ * dy_;

        std::cout << "Step " << std::setw(5) << time_step_
                  << " | T: [" << std::fixed << std::setprecision(1)
                  << T_min << ", " << T_avg << ", " << T_max << "] K"
                  << " | Total C: " << std::scientific << std::setprecision(3)
                  << C_total << " mol\n";
    }

    /**
     * @brief Save fields to file
     */
    void saveFields(const std::string& prefix) const {
        // Save temperature
        std::ofstream file_T(prefix + "_T.dat");
        for (int i = 0; i < nx_; ++i) {
            for (int j = 0; j < ny_; ++j) {
                file_T << T_[i][j] << " ";
            }
            file_T << "\n";
        }
        file_T.close();

        // Save concentration
        std::ofstream file_C(prefix + "_C.dat");
        for (int i = 0; i < nx_; ++i) {
            for (int j = 0; j < ny_; ++j) {
                file_C << C_[i][j] << " ";
            }
            file_C << "\n";
        }
        file_C.close();
    }

    int getTimeStep() const { return time_step_; }
};

int main() {
    std::cout << "========================================\n";
    std::cout << "Multi-Physics Coupling Example\n";
    std::cout << "Thermal + Chemical + Flow Simulation\n";
    std::cout << "========================================\n\n";

    // Create simulator
    const int nx = 64, ny = 64;
    const double Lx = 1.0, Ly = 1.0;
    const double dt = 0.001;
    const int num_steps = 5000;
    const int output_interval = 500;

    MultiPhysicsSimulator sim(nx, ny, Lx, Ly, dt);

    // Setup initial conditions
    sim.setHotSpot(nx/2, ny/2, 5, 800.0);  // Hot spot at center
    sim.setVortexFlow(0.1);                // Add swirling flow

    std::cout << "Starting simulation...\n";
    std::cout << "Time step size: " << dt << " s\n";
    std::cout << "Total steps: " << num_steps << "\n";
    std::cout << "Output interval: " << output_interval << " steps\n\n";

    auto start_time = std::chrono::high_resolution_clock::now();

    // Time-stepping loop
    for (int step = 0; step < num_steps; ++step) {
        sim.step();

        // Output progress
        if ((step + 1) % output_interval == 0) {
            sim.printStatistics();
            sim.saveFields("multiphysics_step_" + std::to_string(step + 1));
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "\n========================================\n";
    std::cout << "Simulation Complete!\n";
    std::cout << "Total time: " << duration.count() / 1000.0 << " s\n";
    std::cout << "Time per step: " << duration.count() / (double)num_steps << " ms\n";
    std::cout << "========================================\n\n";

    std::cout << "Output files created:\n";
    for (int step = output_interval; step <= num_steps; step += output_interval) {
        std::cout << "  multiphysics_step_" << step << "_T.dat (temperature)\n";
        std::cout << "  multiphysics_step_" << step << "_C.dat (concentration)\n";
    }

    std::cout << "\nVisualize with Python:\n";
    std::cout << "  import numpy as np\n";
    std::cout << "  import matplotlib.pyplot as plt\n";
    std::cout << "  T = np.loadtxt('multiphysics_step_5000_T.dat')\n";
    std::cout << "  plt.imshow(T, cmap='hot')\n";
    std::cout << "  plt.colorbar(label='Temperature (K)')\n";
    std::cout << "  plt.show()\n";

    return 0;
}
