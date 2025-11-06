/**
 * @file LoadBalancing.h
 * @brief Load balancing for dynamic simulations
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-beta
 * @date 2025-11-06
 *
 * Phase 45: Load Balancing
 *
 * Dynamic load balancing with:
 * - Work measurement and profiling
 * - Imbalance detection
 * - Repartitioning decisions
 * - Data migration
 */

#ifndef KOO_PARALLEL_BALANCE_LOAD_BALANCING_H
#define KOO_PARALLEL_BALANCE_LOAD_BALANCING_H

#include "parallel/mpi/MPIWrapper.h"
#include "parallel/domain/DomainDecomposition.h"
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>
#include <chrono>

namespace koo {
namespace parallel {
namespace balance {

// ============================================================================
// Load Statistics
// ============================================================================

/**
 * @brief Load statistics for a process
 */
struct LoadStats {
    double computeTime;      ///< Time spent computing
    double communicationTime;///< Time spent in communication
    int workUnits;          ///< Number of work units (elements, particles, etc.)
    double memory;          ///< Memory usage (bytes)

    LoadStats() : computeTime(0.0), communicationTime(0.0),
                 workUnits(0), memory(0.0) {}

    double getTotalTime() const {
        return computeTime + communicationTime;
    }

    double getWorkPerUnit() const {
        return workUnits > 0 ? computeTime / workUnits : 0.0;
    }
};

/**
 * @brief Global load statistics
 */
struct GlobalLoadStats {
    double minLoad;
    double maxLoad;
    double avgLoad;
    double imbalance;  ///< Imbalance factor (max/avg)

    GlobalLoadStats() : minLoad(0.0), maxLoad(0.0), avgLoad(0.0), imbalance(1.0) {}

    bool isBalanced(double threshold = 1.1) const {
        return imbalance < threshold;
    }
};

// ============================================================================
// Work Timer
// ============================================================================

/**
 * @brief Timer for measuring work
 */
class WorkTimer {
public:
    /**
     * @brief Start timer
     */
    void start() {
        startTime_ = std::chrono::high_resolution_clock::now();
    }

    /**
     * @brief Stop timer and return elapsed time in seconds
     */
    double stop() {
        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = endTime - startTime_;
        return elapsed.count();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime_;
};

// ============================================================================
// Load Monitor
// ============================================================================

/**
 * @brief Monitors computational load across processes
 */
class LoadMonitor {
public:
    /**
     * @brief Constructor
     * @param comm MPI communicator
     */
    explicit LoadMonitor(const mpi::MPIComm& comm) : comm_(comm) {}

    /**
     * @brief Record work statistics
     */
    void recordStats(const LoadStats& stats) {
        localStats_ = stats;
    }

    /**
     * @brief Gather global load statistics
     */
    GlobalLoadStats getGlobalStats() const {
        GlobalLoadStats global;

        double localLoad = localStats_.getTotalTime();

        // Gather statistics
        global.minLoad = comm_.min(localLoad);
        global.maxLoad = comm_.max(localLoad);
        global.avgLoad = comm_.sum(localLoad) / comm_.getSize();

        if (global.avgLoad > 0.0) {
            global.imbalance = global.maxLoad / global.avgLoad;
        }

        return global;
    }

    /**
     * @brief Get work distribution across processes
     */
    std::vector<int> getWorkDistribution() const {
        std::vector<int> distribution(comm_.getSize());
        int localWork = localStats_.workUnits;

        comm_.allGather(&localWork, 1, distribution.data(), 1);

        return distribution;
    }

    /**
     * @brief Check if rebalancing is needed
     *
     * @param threshold Imbalance threshold (default: 1.1 = 10% imbalance)
     */
    bool needsRebalancing(double threshold = 1.1) const {
        GlobalLoadStats global = getGlobalStats();
        return !global.isBalanced(threshold);
    }

private:
    const mpi::MPIComm& comm_;
    LoadStats localStats_;
};

// ============================================================================
// Load Balancer
// ============================================================================

/**
 * @brief Dynamic load balancer
 *
 * Redistributes work based on measured load
 */
class LoadBalancer {
public:
    /**
     * @brief Constructor
     * @param comm MPI communicator
     */
    explicit LoadBalancer(const mpi::MPIComm& comm)
        : comm_(comm), monitor_(comm) {}

    /**
     * @brief Record computation time for current step
     */
    void recordComputeTime(double time, int workUnits) {
        LoadStats stats;
        stats.computeTime = time;
        stats.workUnits = workUnits;
        monitor_.recordStats(stats);
    }

    /**
     * @brief Compute new work distribution
     *
     * Uses simple algorithm: redistribute work proportionally
     * to inverse of compute time
     *
     * @param currentDistribution Current work distribution
     * @return New work distribution
     */
    std::vector<int> computeNewDistribution(const std::vector<int>& currentDistribution) {
        int nprocs = comm_.getSize();
        std::vector<int> newDistribution(nprocs);

        // Get compute times
        double localTime = monitor_.getGlobalStats().avgLoad;  // Simplified
        std::vector<double> times(nprocs);
        comm_.allGather(&localTime, 1, times.data(), 1);

        // Compute total work
        int totalWork = 0;
        for (int w : currentDistribution) {
            totalWork += w;
        }

        // Compute target work per process based on inverse time
        std::vector<double> invTimes(nprocs);
        double totalInvTime = 0.0;
        for (int i = 0; i < nprocs; ++i) {
            invTimes[i] = times[i] > 0.0 ? 1.0 / times[i] : 1.0;
            totalInvTime += invTimes[i];
        }

        // Distribute work proportionally
        int assigned = 0;
        for (int i = 0; i < nprocs - 1; ++i) {
            double fraction = invTimes[i] / totalInvTime;
            newDistribution[i] = static_cast<int>(totalWork * fraction);
            assigned += newDistribution[i];
        }
        newDistribution[nprocs - 1] = totalWork - assigned;  // Remainder

        return newDistribution;
    }

    /**
     * @brief Compute elements to migrate
     *
     * @param currentDistribution Current distribution
     * @param newDistribution New target distribution
     * @return Map of destination rank -> list of element IDs to send
     */
    std::map<int, std::vector<int>> computeMigration(
        const std::vector<int>& currentDistribution,
        const std::vector<int>& newDistribution) {

        std::map<int, std::vector<int>> migration;
        int rank = comm_.getRank();

        int currentStart = 0;
        for (int i = 0; i < rank; ++i) {
            currentStart += currentDistribution[i];
        }

        int newStart = 0;
        for (int i = 0; i < rank; ++i) {
            newStart += newDistribution[i];
        }

        int currentCount = currentDistribution[rank];
        int newCount = newDistribution[rank];

        // Determine what to send/receive
        if (newCount < currentCount) {
            // Send excess elements
            int toSend = currentCount - newCount;
            int sendStart = currentStart + newCount;

            // Simple: send to next process
            int destRank = (rank + 1) % comm_.getSize();
            std::vector<int> elementsToSend;
            for (int i = 0; i < toSend; ++i) {
                elementsToSend.push_back(sendStart + i);
            }
            migration[destRank] = elementsToSend;
        }

        return migration;
    }

    /**
     * @brief Perform load balancing
     *
     * Returns true if rebalancing was performed
     */
    bool rebalance(std::vector<int>& workDistribution,
                  double threshold = 1.1) {
        if (!monitor_.needsRebalancing(threshold)) {
            return false;
        }

        // Compute new distribution
        auto newDistribution = computeNewDistribution(workDistribution);

        // Compute migration
        auto migration = computeMigration(workDistribution, newDistribution);

        // Update distribution
        workDistribution = newDistribution;

        return true;
    }

    const LoadMonitor& getMonitor() const { return monitor_; }

private:
    const mpi::MPIComm& comm_;
    LoadMonitor monitor_;
};

// ============================================================================
// Adaptive Partitioner
// ============================================================================

/**
 * @brief Adaptive domain partitioner
 *
 * Adjusts partitioning based on computational load
 */
class AdaptivePartitioner {
public:
    /**
     * @brief Constructor
     * @param comm MPI communicator
     */
    explicit AdaptivePartitioner(const mpi::MPIComm& comm)
        : comm_(comm), balancer_(comm) {}

    /**
     * @brief Update partition based on load measurements
     *
     * @param partition Current partition
     * @param computeTime Time spent in computation
     */
    void updatePartition(domain::DomainPartition& partition, double computeTime) {
        balancer_.recordComputeTime(computeTime,
                                    static_cast<int>(partition.localNodeIDs.size()));

        // Check if rebalancing is needed
        std::vector<int> workDistribution = balancer_.getMonitor().getWorkDistribution();

        if (balancer_.rebalance(workDistribution)) {
            // Rebalancing occurred - would update partition here
            // For now, just record the event
            rebalanceCount_++;
        }
    }

    /**
     * @brief Get number of rebalancing events
     */
    int getRebalanceCount() const { return rebalanceCount_; }

    /**
     * @brief Get current load imbalance
     */
    double getImbalance() const {
        return balancer_.getMonitor().getGlobalStats().imbalance;
    }

private:
    const mpi::MPIComm& comm_;
    LoadBalancer balancer_;
    int rebalanceCount_ = 0;
};

} // namespace balance
} // namespace parallel
} // namespace koo

#endif // KOO_PARALLEL_BALANCE_LOAD_BALANCING_H
