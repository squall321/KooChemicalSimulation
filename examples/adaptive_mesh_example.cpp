/**
 * @file adaptive_mesh_example.cpp
 * @brief Adaptive mesh refinement example
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha4
 * @date 2025-11-07
 *
 * This example demonstrates:
 * - Error estimation for mesh refinement
 * - Adaptive mesh refinement based on solution gradients
 * - Dynamic memory management
 * - Mesh coarsening
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <fstream>

/**
 * @brief Refinement level for each cell
 */
enum class RefinementLevel {
    COARSE = 0,   // Base level
    MEDIUM = 1,   // 1 refinement
    FINE = 2,     // 2 refinements
    VERY_FINE = 3 // Maximum refinement
};

/**
 * @brief Adaptive mesh cell
 */
struct AdaptiveCell {
    double x_min, x_max, y_min, y_max;  // Cell bounds
    double value;                        // Solution value
    double error_estimate;               // Local error estimate
    RefinementLevel level;               // Current refinement level
    bool marked_for_refinement;
    bool marked_for_coarsening;

    AdaptiveCell(double xmin, double xmax, double ymin, double ymax,
                 RefinementLevel lv = RefinementLevel::COARSE)
        : x_min(xmin), x_max(xmax), y_min(ymin), y_max(ymax),
          value(0.0), error_estimate(0.0), level(lv),
          marked_for_refinement(false), marked_for_coarsening(false) {}

    double getArea() const {
        return (x_max - x_min) * (y_max - y_min);
    }

    double getCenterX() const { return 0.5 * (x_min + x_max); }
    double getCenterY() const { return 0.5 * (y_min + y_max); }
    double getWidth() const { return x_max - x_min; }
    double getHeight() const { return y_max - y_min; }
};

/**
 * @brief Adaptive mesh manager
 */
class AdaptiveMesh {
private:
    std::vector<AdaptiveCell> cells_;
    double domain_xmin_, domain_xmax_;
    double domain_ymin_, domain_ymax_;
    int refinement_threshold_percent_;
    int coarsening_threshold_percent_;

public:
    AdaptiveMesh(double xmin, double xmax, double ymin, double ymax,
                 int nx_initial, int ny_initial)
        : domain_xmin_(xmin), domain_xmax_(xmax),
          domain_ymin_(ymin), domain_ymax_(ymax),
          refinement_threshold_percent_(75),
          coarsening_threshold_percent_(10)
    {
        // Create initial uniform mesh
        double dx = (xmax - xmin) / nx_initial;
        double dy = (ymax - ymin) / ny_initial;

        for (int j = 0; j < ny_initial; ++j) {
            for (int i = 0; i < nx_initial; ++i) {
                double x0 = xmin + i * dx;
                double x1 = x0 + dx;
                double y0 = ymin + j * dy;
                double y1 = y0 + dy;

                cells_.emplace_back(x0, x1, y0, y1);
            }
        }

        std::cout << "Initial mesh created: " << cells_.size() << " cells\n";
    }

    /**
     * @brief Set solution values (e.g., from a function)
     */
    void setSolution(double (*func)(double, double)) {
        for (auto& cell : cells_) {
            double x = cell.getCenterX();
            double y = cell.getCenterY();
            cell.value = func(x, y);
        }
    }

    /**
     * @brief Estimate error for each cell based on gradient
     */
    void estimateError() {
        // For each cell, compute error based on solution variation
        // In this simple example, use gradient magnitude

        for (auto& cell : cells_) {
            // Find neighboring cells
            double gradient_x = 0.0;
            double gradient_y = 0.0;
            int count = 0;

            double cx = cell.getCenterX();
            double cy = cell.getCenterY();

            // Look for neighbors
            for (const auto& other : cells_) {
                if (&cell == &other) continue;

                double ox = other.getCenterX();
                double oy = other.getCenterY();
                double dist = std::sqrt((ox - cx)*(ox - cx) + (oy - cy)*(oy - cy));

                if (dist < 2.0 * cell.getWidth()) {
                    gradient_x += (other.value - cell.value) * (ox - cx) / (dist * dist);
                    gradient_y += (other.value - cell.value) * (oy - cy) / (dist * dist);
                    count++;
                }
            }

            if (count > 0) {
                gradient_x /= count;
                gradient_y /= count;
            }

            cell.error_estimate = std::sqrt(gradient_x * gradient_x +
                                           gradient_y * gradient_y) *
                                 cell.getArea();
        }
    }

    /**
     * @brief Mark cells for refinement/coarsening
     */
    void markCells() {
        // Find error statistics
        double max_error = 0.0;
        for (const auto& cell : cells_) {
            max_error = std::max(max_error, cell.error_estimate);
        }

        double refine_threshold = max_error * refinement_threshold_percent_ / 100.0;
        double coarsen_threshold = max_error * coarsening_threshold_percent_ / 100.0;

        int refine_count = 0, coarsen_count = 0;

        for (auto& cell : cells_) {
            cell.marked_for_refinement = false;
            cell.marked_for_coarsening = false;

            if (cell.error_estimate > refine_threshold &&
                static_cast<int>(cell.level) < 3) {
                cell.marked_for_refinement = true;
                refine_count++;
            } else if (cell.error_estimate < coarsen_threshold &&
                      static_cast<int>(cell.level) > 0) {
                cell.marked_for_coarsening = true;
                coarsen_count++;
            }
        }

        std::cout << "  Marked for refinement: " << refine_count << " cells\n";
        std::cout << "  Marked for coarsening: " << coarsen_count << " cells\n";
    }

    /**
     * @brief Refine marked cells
     */
    void refineCells() {
        std::vector<AdaptiveCell> new_cells;
        new_cells.reserve(cells_.size() * 2);

        for (const auto& cell : cells_) {
            if (cell.marked_for_refinement) {
                // Split into 4 subcells (2D quadtree)
                double xmid = 0.5 * (cell.x_min + cell.x_max);
                double ymid = 0.5 * (cell.y_min + cell.y_max);

                RefinementLevel new_level = static_cast<RefinementLevel>(
                    static_cast<int>(cell.level) + 1);

                // Bottom-left
                AdaptiveCell c1(cell.x_min, xmid, cell.y_min, ymid, new_level);
                c1.value = cell.value;  // Inherit parent value

                // Bottom-right
                AdaptiveCell c2(xmid, cell.x_max, cell.y_min, ymid, new_level);
                c2.value = cell.value;

                // Top-right
                AdaptiveCell c3(xmid, cell.x_max, ymid, cell.y_max, new_level);
                c3.value = cell.value;

                // Top-left
                AdaptiveCell c4(cell.x_min, xmid, ymid, cell.y_max, new_level);
                c4.value = cell.value;

                new_cells.push_back(c1);
                new_cells.push_back(c2);
                new_cells.push_back(c3);
                new_cells.push_back(c4);
            } else {
                new_cells.push_back(cell);
            }
        }

        cells_ = std::move(new_cells);
    }

    /**
     * @brief Coarsen marked cells (simplified - just remove)
     */
    void coarsenCells() {
        // In a full implementation, would merge adjacent cells
        // Here we just remove cells marked for coarsening
        cells_.erase(
            std::remove_if(cells_.begin(), cells_.end(),
                [](const AdaptiveCell& c) { return c.marked_for_coarsening; }),
            cells_.end()
        );
    }

    /**
     * @brief Perform one adaptive refinement cycle
     */
    void adapt() {
        std::cout << "\nAdaptive refinement cycle:\n";
        std::cout << "  Current cells: " << cells_.size() << "\n";

        estimateError();
        markCells();
        refineCells();
        coarsenCells();

        std::cout << "  New cells: " << cells_.size() << "\n";
    }

    /**
     * @brief Get mesh statistics
     */
    void printStatistics() const {
        int count_coarse = 0, count_medium = 0, count_fine = 0, count_very_fine = 0;
        double total_area = 0.0;
        double min_area = 1e10, max_area = 0.0;

        for (const auto& cell : cells_) {
            switch (cell.level) {
                case RefinementLevel::COARSE: count_coarse++; break;
                case RefinementLevel::MEDIUM: count_medium++; break;
                case RefinementLevel::FINE: count_fine++; break;
                case RefinementLevel::VERY_FINE: count_very_fine++; break;
            }

            double area = cell.getArea();
            total_area += area;
            min_area = std::min(min_area, area);
            max_area = std::max(max_area, area);
        }

        std::cout << "\nMesh Statistics:\n";
        std::cout << "  Total cells: " << cells_.size() << "\n";
        std::cout << "  Refinement levels:\n";
        std::cout << "    Coarse (0): " << count_coarse << "\n";
        std::cout << "    Medium (1): " << count_medium << "\n";
        std::cout << "    Fine (2): " << count_fine << "\n";
        std::cout << "    Very fine (3): " << count_very_fine << "\n";
        std::cout << "  Cell area range: [" << std::scientific
                  << min_area << ", " << max_area << "]\n";
        std::cout << "  Total area: " << total_area << "\n";
    }

    /**
     * @brief Save mesh to file for visualization
     */
    void saveMesh(const std::string& filename) const {
        std::ofstream file(filename);

        file << "# x_min y_min x_max y_max value level\n";
        for (const auto& cell : cells_) {
            file << cell.x_min << " " << cell.y_min << " "
                 << cell.x_max << " " << cell.y_max << " "
                 << cell.value << " " << static_cast<int>(cell.level) << "\n";
        }

        file.close();
        std::cout << "Mesh saved to " << filename << "\n";
    }

    size_t getNumCells() const { return cells_.size(); }
};

// Test functions
double gaussianPulse(double x, double y) {
    double x0 = 0.5, y0 = 0.5;
    double sigma = 0.1;
    double r2 = (x - x0) * (x - x0) + (y - y0) * (y - y0);
    return std::exp(-r2 / (2.0 * sigma * sigma));
}

double multiPulse(double x, double y) {
    double v1 = gaussianPulse(x - 0.2, y - 0.2);
    double v2 = gaussianPulse(x + 0.2, y + 0.2);
    double v3 = gaussianPulse(x - 0.2, y + 0.2);
    return v1 + 0.5 * v2 + 0.7 * v3;
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  Adaptive Mesh Refinement Example\n";
    std::cout << "========================================\n\n";

    // Create initial mesh
    AdaptiveMesh mesh(0.0, 1.0, 0.0, 1.0, 8, 8);

    // Set solution
    std::cout << "\nSetting solution (multiple Gaussian pulses)...\n";
    mesh.setSolution(multiPulse);

    mesh.printStatistics();
    mesh.saveMesh("adaptive_mesh_initial.dat");

    // Perform adaptive refinement cycles
    const int num_cycles = 4;
    std::cout << "\n========================================\n";
    std::cout << "Performing " << num_cycles << " refinement cycles\n";
    std::cout << "========================================\n";

    for (int cycle = 1; cycle <= num_cycles; ++cycle) {
        std::cout << "\n--- Cycle " << cycle << " ---\n";
        mesh.adapt();
        mesh.setSolution(multiPulse);  // Update solution on new mesh

        if (cycle == num_cycles) {
            mesh.printStatistics();
            mesh.saveMesh("adaptive_mesh_final.dat");
        }
    }

    std::cout << "\n========================================\n";
    std::cout << "Adaptive refinement complete!\n";
    std::cout << "========================================\n\n";

    std::cout << "Visualization with Python:\n";
    std::cout << "```python\n";
    std::cout << "import numpy as np\n";
    std::cout << "import matplotlib.pyplot as plt\n";
    std::cout << "from matplotlib.patches import Rectangle\n\n";
    std::cout << "data = np.loadtxt('adaptive_mesh_final.dat')\n";
    std::cout << "fig, ax = plt::subplots(figsize=(10, 10))\n\n";
    std::cout << "for row in data:\n";
    std::cout << "    xmin, ymin, xmax, ymax, value, level = row\n";
    std::cout << "    width = xmax - xmin\n";
    std::cout << "    height = ymax - ymin\n";
    std::cout << "    rect = Rectangle((xmin, ymin), width, height,\n";
    std::cout << "                     facecolor=plt.cm.hot(value),\n";
    std::cout << "                     edgecolor='black', linewidth=0.5)\n";
    std::cout << "    ax.add_patch(rect)\n\n";
    std::cout << "ax.set_xlim(0, 1)\n";
    std::cout << "ax.set_ylim(0, 1)\n";
    std::cout << "ax.set_aspect('equal')\n";
    std::cout << "plt.title('Adaptive Mesh Refinement')\n";
    std::cout << "plt.show()\n";
    std::cout << "```\n";

    return 0;
}
