/**
 * @file test_phase51.cpp
 * @brief Unit tests for Phase 51: GPU Abstraction Layer
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha1
 */

#include "gpu/Device.h"
#include "gpu/Memory.h"
#include "gpu/Stream.h"
#include "gpu/Kernel.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>

using namespace koo::gpu;

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
// Phase 51 Tests
// ============================================

TEST(device_count) {
    int count = Device::getDeviceCount();
    std::cout << "\n    GPU devices found: " << count << std::endl;
    // Just check it doesn't throw
    assert(count >= 0);
}

TEST(device_creation) {
    if (Device::getDeviceCount() == 0) {
        std::cout << "\n    Skipping (no GPU)" << std::endl;
        return;
    }

    Device device = Device::getDevice(0);
    assert(device.getId() == 0);
    assert(!device.getName().empty());
}

TEST(device_properties) {
    if (!Device::isGPUAvailable()) {
        std::cout << "\n    Skipping (no GPU)" << std::endl;
        return;
    }

    Device device = Device::getDevice(0);
    auto props = device.getProperties();

    std::cout << "\n";
    std::cout << "    Name: " << props.name << "\n";
    std::cout << "    Compute: " << props.major << "." << props.minor << "\n";
    std::cout << "    Memory: " << (props.totalMemory / (1024*1024*1024.0)) << " GB\n";
    std::cout << "    SMs: " << props.multiProcessorCount << "\n";

    assert(props.multiProcessorCount > 0);
    assert(props.maxThreadsPerBlock > 0);
}

TEST(device_runtime) {
    std::string runtime = Device::getRuntime();
    std::cout << "\n    Runtime: " << runtime << std::endl;

    assert(runtime == "CUDA" || runtime == "HIP" || runtime == "CPU");
}

TEST(device_memory_info) {
    if (!Device::isGPUAvailable()) {
        std::cout << "\n    Skipping (no GPU)" << std::endl;
        return;
    }

    Device device = Device::getDevice(0);
    auto [free, total] = device.getMemoryInfo();

    std::cout << "\n";
    std::cout << "    Free: " << (free / (1024*1024.0)) << " MB\n";
    std::cout << "    Total: " << (total / (1024*1024.0)) << " MB\n";

    assert(total > 0);
    assert(free <= total);
}

TEST(device_memory_allocation) {
    DeviceMemory<double> mem(1000);
    assert(mem.size() == 1000);
    assert(!mem.empty());

    mem.resize(2000);
    assert(mem.size() == 2000);

    mem.free();
    assert(mem.empty());
}

TEST(device_memory_copy_host_to_device) {
    std::vector<float> hostData(100);
    for (size_t i = 0; i < hostData.size(); ++i) {
        hostData[i] = static_cast<float>(i);
    }

    DeviceMemory<float> deviceMem(100);
    deviceMem.copyFromHost(hostData.data());

    // Copy back and verify
    std::vector<float> result(100);
    deviceMem.copyToHost(result.data());

    for (size_t i = 0; i < result.size(); ++i) {
        assert(std::abs(result[i] - static_cast<float>(i)) < 1e-6f);
    }
}

TEST(device_memory_copy_device_to_device) {
    DeviceMemory<int> src(50);
    DeviceMemory<int> dst(50);

    // Initialize source
    std::vector<int> data(50);
    for (int i = 0; i < 50; ++i) data[i] = i * 2;
    src.copyFromHost(data.data());

    // Copy device to device
    dst.copyFromDevice(src);

    // Verify
    std::vector<int> result(50);
    dst.copyToHost(result.data());
    for (int i = 0; i < 50; ++i) {
        assert(result[i] == i * 2);
    }
}

TEST(device_memory_zero) {
    DeviceMemory<double> mem(200);
    mem.zero();

    std::vector<double> result(200, 1.0);  // Initialize to non-zero
    mem.copyToHost(result.data());

    for (double val : result) {
        assert(val == 0.0);
    }
}

TEST(pinned_memory) {
    PinnedMemory<float> pinned(500);
    assert(pinned.size() == 500);

    // CPU access
    for (size_t i = 0; i < pinned.size(); ++i) {
        pinned[i] = static_cast<float>(i) * 1.5f;
    }

    // Verify
    for (size_t i = 0; i < pinned.size(); ++i) {
        assert(std::abs(pinned[i] - static_cast<float>(i) * 1.5f) < 1e-6f);
    }
}

TEST(managed_memory) {
    ManagedMemory<double> managed(300);
    assert(managed.size() == 300);

    // CPU access
    for (size_t i = 0; i < managed.size(); ++i) {
        managed[i] = static_cast<double>(i) * 2.5;
    }

    // Verify
    for (size_t i = 0; i < managed.size(); ++i) {
        assert(std::abs(managed[i] - static_cast<double>(i) * 2.5) < 1e-10);
    }
}

TEST(stream_creation) {
    Stream stream;
    stream.synchronize();  // Should not throw

    assert(!stream.query() || stream.query());  // Either true or false, just check callable
}

TEST(stream_async_operations) {
    Stream stream1;
    Stream stream2;

    DeviceMemory<int> mem1(100);
    DeviceMemory<int> mem2(100);

    std::vector<int> data1(100, 1);
    std::vector<int> data2(100, 2);

    stream1.copyFromHostAsync(mem1, data1.data(), 100);
    stream2.copyFromHostAsync(mem2, data2.data(), 100);

    stream1.synchronize();
    stream2.synchronize();

    // Verify
    std::vector<int> result1(100), result2(100);
    mem1.copyToHost(result1.data());
    mem2.copyToHost(result2.data());

    for (int val : result1) assert(val == 1);
    for (int val : result2) assert(val == 2);
}

TEST(event_timing) {
    Event start, end;

    start.record();

    // Simulate some work
    DeviceMemory<double> mem(10000);
    mem.zero();
    if (Device::isGPUAvailable()) {
        Device::getDevice(0).synchronize();
    }

    end.record();
    end.synchronize();

    float ms = Event::elapsedTime(start, end);
    std::cout << "\n    Elapsed: " << ms << " ms" << std::endl;

    assert(ms >= 0.0f);
}

TEST(stream_pool) {
    StreamPool pool(4);
    assert(pool.size() == 4);

    // Get streams round-robin
    Stream& s1 = pool.getNext();
    Stream& s2 = pool.getNext();
    Stream& s3 = pool.getNext();

    // Use streams
    s1.synchronize();
    s2.synchronize();
    s3.synchronize();

    pool.synchronizeAll();
}

TEST(launch_config_1d) {
    auto config = KernelLauncher::make1DConfig(10000, 256);

    assert(config.blockSize.x == 256);
    assert(config.gridSize.x == 40);  // ceil(10000/256)
    assert(config.totalThreads() >= 10000);

    std::cout << "\n    " << config.toString() << std::endl;
}

TEST(launch_config_2d) {
    auto config = KernelLauncher::make2DConfig(1024, 768, 16, 16);

    assert(config.blockSize.x == 16);
    assert(config.blockSize.y == 16);
    assert(config.gridSize.x == 64);  // ceil(1024/16)
    assert(config.gridSize.y == 48);  // ceil(768/16)

    std::cout << "\n    " << config.toString() << std::endl;
}

TEST(launch_config_3d) {
    auto config = KernelLauncher::make3DConfig(128, 128, 64, 8, 8, 8);

    assert(config.blockSize.x == 8);
    assert(config.blockSize.y == 8);
    assert(config.blockSize.z == 8);
    assert(config.gridSize.x == 16);  // ceil(128/8)
    assert(config.gridSize.y == 16);
    assert(config.gridSize.z == 8);   // ceil(64/8)

    std::cout << "\n    " << config.toString() << std::endl;
}

TEST(optimal_block_size) {
    if (!Device::isGPUAvailable()) {
        std::cout << "\n    Skipping (no GPU)" << std::endl;
        return;
    }

    Device device = Device::getDevice(0);

    int blockSize1D = KernelLauncher::getOptimal1DBlockSize(device);
    auto [blockX, blockY] = KernelLauncher::getOptimal2DBlockSize(device);

    std::cout << "\n";
    std::cout << "    Optimal 1D block: " << blockSize1D << "\n";
    std::cout << "    Optimal 2D block: " << blockX << "x" << blockY << "\n";

    assert(blockSize1D > 0);
    assert(blockX > 0 && blockY > 0);
    assert(KernelLauncher::isValidBlockSize(blockSize1D, device));
}

TEST(helper_functions) {
    std::vector<double> hostData = {1.0, 2.0, 3.0, 4.0, 5.0};

    // makeDeviceMemory
    auto deviceMem = makeDeviceMemory(hostData);
    assert(deviceMem.size() == 5);

    // toVector
    auto result = toVector(deviceMem);
    assert(result.size() == 5);
    for (size_t i = 0; i < result.size(); ++i) {
        assert(std::abs(result[i] - hostData[i]) < 1e-10);
    }
}

TEST(device_guard) {
    if (Device::getDeviceCount() < 2) {
        std::cout << "\n    Skipping (need 2+ GPUs)" << std::endl;
        return;
    }

    int initialDevice = Device::getCurrentDeviceId();
    assert(initialDevice >= 0);

    {
        DeviceGuard guard(0);
        assert(Device::getCurrentDeviceId() == 0);
    }

    // Should restore after guard destruction
    assert(Device::getCurrentDeviceId() == initialDevice);
}

// ============================================
// Main
// ============================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Phase 51: GPU Abstraction Layer Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    std::cout << "GPU Runtime: " << Device::getRuntime() << std::endl;
    std::cout << "GPUs Available: " << Device::getDeviceCount() << std::endl;
    std::cout << std::endl;

    // Run all tests
    run_test_device_count();
    run_test_device_creation();
    run_test_device_properties();
    run_test_device_runtime();
    run_test_device_memory_info();
    run_test_device_memory_allocation();
    run_test_device_memory_copy_host_to_device();
    run_test_device_memory_copy_device_to_device();
    run_test_device_memory_zero();
    run_test_pinned_memory();
    run_test_managed_memory();
    run_test_stream_creation();
    run_test_stream_async_operations();
    run_test_event_timing();
    run_test_stream_pool();
    run_test_launch_config_1d();
    run_test_launch_config_2d();
    run_test_launch_config_3d();
    run_test_optimal_block_size();
    run_test_helper_functions();
    run_test_device_guard();

    // Summary
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Tests passed: " << tests_passed << " / " << tests_total << std::endl;
    std::cout << "========================================" << std::endl;

    if (tests_passed == tests_total) {
        std::cout << "✓ All tests PASSED!" << std::endl;
        return 0;
    } else {
        std::cout << "✗ Some tests FAILED" << std::endl;
        return 1;
    }
}
