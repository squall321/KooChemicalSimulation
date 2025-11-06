#ifndef KOO_PATTERN_ANALYSIS_H
#define KOO_PATTERN_ANALYSIS_H

#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <string>

namespace koo {
namespace physics {
namespace coupling {

/**
 * @class PatternAnalysis
 * @brief Tools for analyzing pattern formation in reaction-diffusion systems
 *
 * Phase 23: Reaction-Diffusion Coupling (v0.5.0-alpha3)
 *
 * Provides analysis for:
 * - Spatial pattern characteristics
 * - Temporal dynamics
 * - Bifurcation detection
 * - Pattern classification
 */
class PatternAnalysis {
public:
    /**
     * @enum PatternType
     * @brief Classification of pattern types
     */
    enum class PatternType {
        UNIFORM,      ///< Uniform steady state
        SPOTS,        ///< Localized spots
        STRIPES,      ///< Stripe patterns
        SPIRALS,      ///< Spiral waves
        LABYRINTH,    ///< Labyrinthine patterns
        HEXAGONS,     ///< Hexagonal patterns
        OSCILLATORY,  ///< Temporal oscillations
        CHAOTIC       ///< Chaotic dynamics
    };

    /**
     * @struct PatternMetrics
     * @brief Quantitative metrics for pattern characterization
     */
    struct PatternMetrics {
        double meanValue;           ///< Mean concentration
        double variance;            ///< Spatial variance
        double correlation;         ///< Spatial correlation length
        double dominantWavelength;  ///< Dominant spatial wavelength
        double amplitude;           ///< Pattern amplitude
        double symmetry;            ///< Symmetry measure
        PatternType type;           ///< Classified pattern type
    };

    /**
     * @brief Calculate spatial variance of a field
     * σ² = <(u - <u>)²>
     */
    static double calculateVariance(const std::vector<double>& field) {
        if (field.empty()) return 0.0;

        double mean = std::accumulate(field.begin(), field.end(), 0.0) / field.size();

        double variance = 0.0;
        for (double val : field) {
            double diff = val - mean;
            variance += diff * diff;
        }

        return variance / field.size();
    }

    /**
     * @brief Calculate mean value of a field
     */
    static double calculateMean(const std::vector<double>& field) {
        if (field.empty()) return 0.0;
        return std::accumulate(field.begin(), field.end(), 0.0) / field.size();
    }

    /**
     * @brief Calculate amplitude of pattern
     * Amplitude = max - min
     */
    static double calculateAmplitude(const std::vector<double>& field) {
        if (field.empty()) return 0.0;

        auto minmax = std::minmax_element(field.begin(), field.end());
        return *minmax.second - *minmax.first;
    }

    /**
     * @brief Calculate spatial correlation function
     *
     * C(r) = <u(x) u(x+r)> - <u>²
     *
     * Simplified version: correlation between adjacent points
     */
    static double calculateCorrelation(const std::vector<double>& field) {
        if (field.size() < 2) return 0.0;

        double mean = calculateMean(field);
        double correlation = 0.0;
        int count = 0;

        for (size_t i = 0; i < field.size() - 1; ++i) {
            correlation += (field[i] - mean) * (field[i+1] - mean);
            count++;
        }

        if (count == 0) return 0.0;

        double variance = calculateVariance(field);
        if (variance < 1.0e-15) return 0.0;

        return correlation / (count * variance);
    }

    /**
     * @brief Estimate dominant wavelength using autocorrelation
     *
     * Finds the first peak in the autocorrelation function
     */
    static double estimateDominantWavelength(const std::vector<double>& field,
                                            double spacing = 1.0) {
        if (field.size() < 4) return 0.0;

        double mean = calculateMean(field);
        std::vector<double> centered(field.size());
        for (size_t i = 0; i < field.size(); ++i) {
            centered[i] = field[i] - mean;
        }

        // Calculate autocorrelation for small lags
        size_t maxLag = std::min(field.size() / 4, size_t(50));
        std::vector<double> autocorr(maxLag, 0.0);

        double variance = calculateVariance(field);
        if (variance < 1.0e-15) return 0.0;

        for (size_t lag = 1; lag < maxLag; ++lag) {
            double corr = 0.0;
            int count = 0;
            for (size_t i = 0; i < field.size() - lag; ++i) {
                corr += centered[i] * centered[i + lag];
                count++;
            }
            autocorr[lag] = corr / (count * variance);
        }

        // Find first peak (where correlation becomes positive again after dip)
        for (size_t i = 2; i < maxLag - 1; ++i) {
            if (autocorr[i-1] < autocorr[i] && autocorr[i] > autocorr[i+1] && autocorr[i] > 0.0) {
                return 2.0 * i * spacing; // Wavelength is twice the half-period
            }
        }

        return 0.0;
    }

    /**
     * @brief Calculate pattern symmetry measure
     *
     * Compares field with its reflection
     */
    static double calculateSymmetry(const std::vector<double>& field) {
        if (field.size() < 2) return 1.0;

        double diff = 0.0;
        double norm = 0.0;
        size_t n = field.size();

        for (size_t i = 0; i < n / 2; ++i) {
            double val1 = field[i];
            double val2 = field[n - 1 - i];
            diff += (val1 - val2) * (val1 - val2);
            norm += val1 * val1 + val2 * val2;
        }

        if (norm < 1.0e-15) return 1.0;
        return 1.0 - std::sqrt(diff / norm);
    }

    /**
     * @brief Classify pattern type based on metrics
     */
    static PatternType classifyPattern(const PatternMetrics& metrics) {
        // Uniform if variance is very small
        if (metrics.variance < 1.0e-6) {
            return PatternType::UNIFORM;
        }

        // Oscillatory if no clear spatial structure but non-zero variance
        if (metrics.dominantWavelength < 1.0e-6 && metrics.variance > 0.01) {
            return PatternType::OSCILLATORY;
        }

        // Spots: high amplitude, low correlation
        if (metrics.amplitude > 0.5 && metrics.correlation < 0.3) {
            return PatternType::SPOTS;
        }

        // Stripes: moderate wavelength, high correlation
        if (metrics.dominantWavelength > 0.0 && metrics.correlation > 0.5) {
            return PatternType::STRIPES;
        }

        // Hexagons: high symmetry, moderate correlation
        if (metrics.symmetry > 0.8 && metrics.correlation > 0.3) {
            return PatternType::HEXAGONS;
        }

        // Labyrinth: complex structure
        if (metrics.correlation > 0.2 && metrics.correlation < 0.5) {
            return PatternType::LABYRINTH;
        }

        // Chaotic: high variance, low correlation
        if (metrics.variance > 0.1 && metrics.correlation < 0.2) {
            return PatternType::CHAOTIC;
        }

        return PatternType::UNIFORM;
    }

    /**
     * @brief Analyze pattern in a concentration field
     */
    static PatternMetrics analyzePattern(const std::vector<double>& field,
                                        double spacing = 1.0) {
        PatternMetrics metrics;

        metrics.meanValue = calculateMean(field);
        metrics.variance = calculateVariance(field);
        metrics.amplitude = calculateAmplitude(field);
        metrics.correlation = calculateCorrelation(field);
        metrics.dominantWavelength = estimateDominantWavelength(field, spacing);
        metrics.symmetry = calculateSymmetry(field);
        metrics.type = classifyPattern(metrics);

        return metrics;
    }

    /**
     * @brief Get pattern type name
     */
    static std::string getPatternTypeName(PatternType type) {
        switch (type) {
            case PatternType::UNIFORM:
                return "Uniform";
            case PatternType::SPOTS:
                return "Spots";
            case PatternType::STRIPES:
                return "Stripes";
            case PatternType::SPIRALS:
                return "Spirals";
            case PatternType::LABYRINTH:
                return "Labyrinth";
            case PatternType::HEXAGONS:
                return "Hexagons";
            case PatternType::OSCILLATORY:
                return "Oscillatory";
            case PatternType::CHAOTIC:
                return "Chaotic";
            default:
                return "Unknown";
        }
    }

    /**
     * @brief Calculate spatial gradient magnitude
     * |∇u| ≈ |u[i+1] - u[i]| / Δx
     */
    static std::vector<double> calculateGradientMagnitude(const std::vector<double>& field,
                                                         double spacing = 1.0) {
        if (field.size() < 2) return {};

        std::vector<double> gradient(field.size() - 1);
        for (size_t i = 0; i < field.size() - 1; ++i) {
            gradient[i] = std::abs(field[i+1] - field[i]) / spacing;
        }

        return gradient;
    }

    /**
     * @brief Calculate pattern energy
     *
     * E = ∫ u² dx (L2 norm squared)
     */
    static double calculatePatternEnergy(const std::vector<double>& field) {
        double energy = 0.0;
        for (double val : field) {
            energy += val * val;
        }
        return energy;
    }

    /**
     * @brief Calculate pattern entropy (as a measure of disorder)
     *
     * H = -Σ p_i log(p_i)
     *
     * where p_i is the normalized histogram
     */
    static double calculatePatternEntropy(const std::vector<double>& field,
                                         int numBins = 20) {
        if (field.empty()) return 0.0;

        // Find range
        auto minmax = std::minmax_element(field.begin(), field.end());
        double minVal = *minmax.first;
        double maxVal = *minmax.second;
        double range = maxVal - minVal;

        if (range < 1.0e-15) return 0.0;

        // Create histogram
        std::vector<int> histogram(numBins, 0);
        for (double val : field) {
            int bin = static_cast<int>((val - minVal) / range * (numBins - 1));
            bin = std::max(0, std::min(numBins - 1, bin));
            histogram[bin]++;
        }

        // Calculate entropy
        double entropy = 0.0;
        int total = field.size();
        for (int count : histogram) {
            if (count > 0) {
                double p = static_cast<double>(count) / total;
                entropy -= p * std::log(p);
            }
        }

        return entropy;
    }

    /**
     * @brief Detect if pattern is stable or evolving
     *
     * Compares two consecutive time snapshots
     */
    static bool isPatternStable(const std::vector<double>& field1,
                               const std::vector<double>& field2,
                               double tolerance = 1.0e-3) {
        if (field1.size() != field2.size()) return false;

        double maxDiff = 0.0;
        for (size_t i = 0; i < field1.size(); ++i) {
            double diff = std::abs(field1[i] - field2[i]);
            maxDiff = std::max(maxDiff, diff);
        }

        return maxDiff < tolerance;
    }
};

} // namespace coupling
} // namespace physics
} // namespace koo

#endif // KOO_PATTERN_ANALYSIS_H
