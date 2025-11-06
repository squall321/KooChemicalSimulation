/**
 * @file test_phase55.cpp
 * @brief Unit tests for Phase 55: GPU Domain Decomposition
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha2
 */

#include "gpu/Device.h"
#include "gpu/Memory.h"
#include "gpu/parallel/MultiGPU.h"
#include "gpu/parallel/GPUComm.h"
#include "gpu/parallel/HybridMPI.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include <numeric>

using namespace koo::gpu;
using namespace koo::gpu::parallel;

// Test counter
int tests_passed = 0;
int tests_total = 0;

#define TEST(name) \
    void test_##name(); \
    void run_test_##name() { \
        tests_total++; \
        std::cout << "Test " << tests_total << ": " << #name << " ... "; \
        try { \
            test_##name(); \
            tests_passed++; \
            std::cout << "PASSED" << std::endl; \
        } catch (const std::exception& e) { \
            std::cout << "FAILED: " << e.what() << std::endl; \
        } \
    } \
    void test_##name()

// ============================================
// Phase 55 Tests: Multi-GPU Management
// ============================================

TEST(multigpu_initialization) {
    MultiGPUManager mgr;

    // Initialize with all available GPUs
    mgr.initialize();

    int numGPUs = mgr.getNumGPUs();
    std::cout << "\n    GPUs initialized: " << numGPUs << std::endl;

    assert(numGPUs > 0);
    assert(mgr.isInitialized());

    mgr.finalize();
    assert(!mgr.isInitialized());
}

TEST(multigpu_device_access) {
    if (Device::getDeviceCount() == 0) {
        std::cout << "\n    Skipping (no GPU)" << std::endl;
        return;
    }

    MultiGPUManager mgr;
    mgr.initialize();

    int numGPUs = mgr.getNumGPUs();
    assert(numGPUs > 0);

    // Access each device
    for (int i = 0; i < numGPUs; ++i) {
        Device& dev = mgr.getDevice(i);
        int devId = mgr.getDeviceId(i);

        assert(dev.getId() == devId);
        assert(!dev.getName().empty());

        std::cout << "\n    GPU " << i << ": " << dev.getName() << std::endl;
    }

    mgr.finalize();
}

TEST(multigpu_synchronize_all) {
    if (Device::getDeviceCount() == 0) {
        std::cout << "\n    Skipping (no GPU)" << std::endl;
        return;
    }

    MultiGPUManager mgr;
    mgr.initialize();

    // Synchronize all GPUs
    mgr.synchronizeAll();

    std::cout << "\n    All GPUs synchronized" << std::endl;

    mgr.finalize();
}

TEST(domain_partition_1d) {
    MultiGPUManager mgr;
    mgr.initialize();

    size_t globalSize = 1000;
    size_t haloSize = 2;

    auto partitions = mgr.partition1D(globalSize, haloSize);

    int numGPUs = mgr.getNumGPUs();
    assert(static_cast<int>(partitions.size()) == numGPUs);

    std::cout << "\n    1D Domain Partitioning:" << std::endl;
    std::cout << "    Global size: " << globalSize << std::endl;
    std::cout << "    Halo size: " << haloSize << std::endl;

    size_t totalSize = 0;
    for (int i = 0; i < numGPUs; ++i) {
        const auto& part = partitions[i];

        assert(part.isValid());
        assert(part.gpuId >= 0);
        assert(part.localSize > 0);
        assert(part.endIndex > part.startIndex);

        totalSize += part.localSize;

        std::cout << "    GPU " << i << ": [" << part.startIndex << ", "
                  << part.endIndex << ") size=" << part.localSize
                  << " halos=[" << part.halos[0] << ", " << part.halos[1] << "]"
                  << std::endl;
    }

    // Verify total size matches
    assert(totalSize == globalSize);

    // Verify halos
    for (int i = 0; i < numGPUs; ++i) {
        const auto& part = partitions[i];

        if (i == 0) {
            assert(part.halos[0] == 0);  // No left halo for first GPU
        } else {
            assert(part.halos[0] == haloSize);
        }

        if (i == numGPUs - 1) {
            assert(part.halos[1] == 0);  // No right halo for last GPU
        } else {
            assert(part.halos[1] == haloSize);
        }
    }

    mgr.finalize();
}

TEST(domain_partition_2d) {
    MultiGPUManager mgr;
    mgr.initialize();

    size_t nx = 100, ny = 100;
    size_t haloSize = 1;

    auto partitions = mgr.partition2D(nx, ny, haloSize);

    int numGPUs = mgr.getNumGPUs();
    assert(static_cast<int>(partitions.size()) == numGPUs);

    std::cout << "\n    2D Domain Partitioning:" << std::endl;
    std::cout << "    Grid: " << nx << " x " << ny << std::endl;

    for (int i = 0; i < numGPUs; ++i) {
        const auto& part = partitions[i];
        assert(part.isValid());

        std::cout << "    GPU " << i << ": size=" << part.localSize << std::endl;
    }

    mgr.finalize();
}

TEST(workload_tracking) {
    MultiGPUManager mgr;
    mgr.initialize();

    int numGPUs = mgr.getNumGPUs();
    if (numGPUs == 0) {
        std::cout << "\n    Skipping (no GPU)" << std::endl;
        return;
    }

    // Update utilization for each GPU
    for (int i = 0; i < numGPUs; ++i) {
        double util = 0.5 + 0.1 * i;  // Varying utilization
        mgr.updateUtilization(i, util);
    }

    std::cout << "\n    GPU Workloads:" << std::endl;
    for (int i = 0; i < numGPUs; ++i) {
        const auto& wl = mgr.getWorkload(i);
        std::cout << "    GPU " << i << ": util=" << wl.utilization
                  << " load=" << wl.loadFactor() << std::endl;
    }

    int leastLoaded = mgr.getLeastLoadedGPU();
    std::cout << "    Least loaded GPU: " << leastLoaded << std::endl;
    assert(leastLoaded >= 0 && leastLoaded < numGPUs);

    mgr.finalize();
}

TEST(load_balancing_check) {
    MultiGPUManager mgr;
    mgr.initialize();

    int numGPUs = mgr.getNumGPUs();
    if (numGPUs <= 1) {
        std::cout << "\n    Skipping (need multiple GPUs)" << std::endl;
        return;
    }

    // Initially balanced (no work assigned)
    bool balanced = mgr.isLoadBalanced();
    std::cout << "\n    Initially balanced: " << (balanced ? "Yes" : "No") << std::endl;

    // Create imbalance
    mgr.updateUtilization(0, 0.9);
    mgr.updateUtilization(1, 0.1);

    balanced = mgr.isLoadBalanced(0.2);  // 20% threshold
    std::cout << "    After imbalance: " << (balanced ? "Balanced" : "Imbalanced") << std::endl;

    mgr.finalize();
}

// ============================================
// Phase 55 Tests: GPU Communication
// ============================================

TEST(gpu_comm_peer_access) {
    MultiGPUManager mgr;
    mgr.initialize();

    int numGPUs = mgr.getNumGPUs();
    if (numGPUs < 2) {
        std::cout << "\n    Skipping (need 2+ GPUs)" << std::endl;
        return;
    }

    GPUCommunicator comm(mgr);

    std::cout << "\n    Peer Access Matrix:" << std::endl;
    for (int i = 0; i < numGPUs; ++i) {
        for (int j = 0; j < numGPUs; ++j) {
            if (i != j) {
                bool canAccess = comm.canAccessPeer(i, j);
                std::cout << "    GPU " << i << " -> GPU " << j << ": "
                          << (canAccess ? "Yes" : "No") << std::endl;
            }
        }
    }

    mgr.finalize();
}

TEST(gpu_comm_transfer) {
    MultiGPUManager mgr;
    mgr.initialize();

    int numGPUs = mgr.getNumGPUs();
    if (numGPUs < 2) {
        std::cout << "\n    Skipping (need 2+ GPUs)" << std::endl;
        return;
    }

    GPUCommunicator comm(mgr);

    // Allocate memory on two GPUs
    size_t n = 1000;

    mgr.getDevice(0).setCurrent();
    DeviceMemory<double> src(n);

    mgr.getDevice(1).setCurrent();
    DeviceMemory<double> dst(n);

    // Initialize source data
    std::vector<double> hostData(n);
    std::iota(hostData.begin(), hostData.end(), 1.0);
    src.copyFromHost(hostData.data(), n);

    // Transfer GPU 0 -> GPU 1
    auto stats = comm.transfer(0, src.data(), 1, dst.data(), n);

    std::cout << "\n    Transfer Stats:" << std::endl;
    std::cout << "    Bytes: " << stats.bytesTransferred << std::endl;
    std::cout << "    Time: " << stats.timeElapsed << " s" << std::endl;
    std::cout << "    Bandwidth: " << stats.bandwidth() << " GB/s" << std::endl;

    // Verify data
    std::vector<double> resultData(n);
    dst.copyToHost(resultData.data(), n);

    for (size_t i = 0; i < n; ++i) {
        assert(std::abs(resultData[i] - hostData[i]) < 1e-10);
    }

    std::cout << "    Data verified successfully" << std::endl;

    mgr.finalize();
}

TEST(gpu_comm_broadcast) {
    MultiGPUManager mgr;
    mgr.initialize();

    int numGPUs = mgr.getNumGPUs();
    if (numGPUs < 2) {
        std::cout << "\n    Skipping (need 2+ GPUs)" << std::endl;
        return;
    }

    GPUCommunicator comm(mgr);

    size_t n = 100;

    // Allocate memory on all GPUs
    std::vector<DeviceMemory<double>> gpuMemory;
    for (int i = 0; i < numGPUs; ++i) {
        mgr.getDevice(i).setCurrent();
        gpuMemory.emplace_back(n);
    }

    // Initialize data on GPU 0
    std::vector<double> hostData(n, 42.0);
    gpuMemory[0].copyFromHost(hostData.data(), n);

    // Broadcast from GPU 0 to all
    std::vector<double*> dstPtrs;
    for (int i = 0; i < numGPUs; ++i) {
        dstPtrs.push_back(gpuMemory[i].data());
    }

    comm.broadcast(0, gpuMemory[0].data(), dstPtrs, n);

    std::cout << "\n    Broadcasted " << n << " elements from GPU 0" << std::endl;

    // Verify all GPUs have the same data
    for (int i = 1; i < numGPUs; ++i) {
        std::vector<double> result(n);
        gpuMemory[i].copyToHost(result.data(), n);

        for (size_t j = 0; j < n; ++j) {
            assert(std::abs(result[j] - 42.0) < 1e-10);
        }
    }

    std::cout << "    Broadcast verified on all GPUs" << std::endl;

    mgr.finalize();
}

TEST(gpu_comm_gather) {
    MultiGPUManager mgr;
    mgr.initialize();

    int numGPUs = mgr.getNumGPUs();
    if (numGPUs < 2) {
        std::cout << "\n    Skipping (need 2+ GPUs)" << std::endl;
        return;
    }

    GPUCommunicator comm(mgr);

    size_t nPerGPU = 100;
    size_t totalSize = nPerGPU * numGPUs;

    // Allocate memory on each GPU
    std::vector<DeviceMemory<double>> gpuMemory;
    for (int i = 0; i < numGPUs; ++i) {
        mgr.getDevice(i).setCurrent();
        gpuMemory.emplace_back(nPerGPU);

        // Initialize with unique values
        std::vector<double> hostData(nPerGPU, static_cast<double>(i));
        gpuMemory[i].copyFromHost(hostData.data(), nPerGPU);
    }

    // Gather to GPU 0
    mgr.getDevice(0).setCurrent();
    DeviceMemory<double> gathered(totalSize);

    std::vector<const double*> srcPtrs;
    std::vector<size_t> counts;
    for (int i = 0; i < numGPUs; ++i) {
        srcPtrs.push_back(gpuMemory[i].data());
        counts.push_back(nPerGPU);
    }

    comm.gather(0, gathered.data(), srcPtrs, counts);

    std::cout << "\n    Gathered " << totalSize << " elements to GPU 0" << std::endl;

    // Verify
    std::vector<double> result(totalSize);
    gathered.copyToHost(result.data(), totalSize);

    for (int i = 0; i < numGPUs; ++i) {
        for (size_t j = 0; j < nPerGPU; ++j) {
            size_t idx = static_cast<size_t>(i) * nPerGPU + j;
            assert(std::abs(result[idx] - static_cast<double>(i)) < 1e-10);
        }
    }

    std::cout << "    Gather verified successfully" << std::endl;

    mgr.finalize();
}

// ============================================
// Phase 55 Tests: Hybrid MPI + GPU
// ============================================

TEST(hybrid_mpi_initialization) {
    HybridMPIManager hybrid;
    hybrid.initialize();

    const auto& rankInfo = hybrid.getRankInfo();

    std::cout << "\n    MPI Rank: " << rankInfo.rank << " / " << rankInfo.numRanks << std::endl;
    std::cout << "    GPUs per rank: " << rankInfo.numGPUsPerRank << std::endl;
    std::cout << "    Is master: " << (hybrid.isMaster() ? "Yes" : "No") << std::endl;

    if (rankInfo.numGPUsPerRank > 0) {
        auto& localMgr = hybrid.getLocalGPUManager();
        assert(localMgr.isInitialized());
        std::cout << "    Local GPUs initialized: " << localMgr.getNumGPUs() << std::endl;
    }

    hybrid.finalize();
}

TEST(hybrid_mpi_barrier) {
    HybridMPIManager hybrid;
    hybrid.initialize();

    std::cout << "\n    Performing MPI barrier..." << std::endl;
    hybrid.barrier();
    std::cout << "    Barrier complete" << std::endl;

    hybrid.finalize();
}

TEST(hybrid_mpi_broadcast) {
    HybridMPIManager hybrid;
    hybrid.initialize();

    const int n = 10;
    std::vector<int> data(n);

    if (hybrid.isMaster()) {
        // Master initializes data
        std::iota(data.begin(), data.end(), 1);
        std::cout << "\n    Master broadcasting data..." << std::endl;
    }

    hybrid.broadcast(data.data(), n, 0);

    // Verify all ranks have the data
    for (int i = 0; i < n; ++i) {
        assert(data[i] == i + 1);
    }

    std::cout << "    Broadcast verified on rank " << hybrid.getRankInfo().rank << std::endl;

    hybrid.finalize();
}

// ============================================
// Main Test Runner
// ============================================

int main() {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << " Phase 55: GPU Domain Decomposition    \n";
    std::cout << "========================================\n";
    std::cout << "\n";

    std::cout << "Runtime: " << Device::getRuntime() << std::endl;
    std::cout << "GPU Devices: " << Device::getDeviceCount() << std::endl;
    std::cout << "\n";

    // Multi-GPU Management Tests
    std::cout << "--- Multi-GPU Management Tests ---\n";
    run_test_multigpu_initialization();
    run_test_multigpu_device_access();
    run_test_multigpu_synchronize_all();
    run_test_domain_partition_1d();
    run_test_domain_partition_2d();
    run_test_workload_tracking();
    run_test_load_balancing_check();

    // GPU Communication Tests
    std::cout << "\n--- GPU Communication Tests ---\n";
    run_test_gpu_comm_peer_access();
    run_test_gpu_comm_transfer();
    run_test_gpu_comm_broadcast();
    run_test_gpu_comm_gather();

    // Hybrid MPI + GPU Tests
    std::cout << "\n--- Hybrid MPI + GPU Tests ---\n";
    run_test_hybrid_mpi_initialization();
    run_test_hybrid_mpi_barrier();
    run_test_hybrid_mpi_broadcast();

    // Summary
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << " Test Summary\n";
    std::cout << "========================================\n";
    std::cout << "Tests passed: " << tests_passed << " / " << tests_total << "\n";

    if (tests_passed == tests_total) {
        std::cout << "Status: ALL TESTS PASSED ✓\n";
    } else {
        std::cout << "Status: SOME TESTS FAILED ✗\n";
    }
    std::cout << "\n";

    return (tests_passed == tests_total) ? 0 : 1;
}
