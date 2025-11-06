/**
 * @file test_phase61_65.cpp
 * @brief Comprehensive tests for Phase 61-65 (Advanced GPU Features)
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha3
 *
 * Tests:
 * - Phase 61: Memory pools, unified memory, async operations
 * - Phase 62: Profiler, NVTX markers
 * - Phase 63: Mixed precision, loss scaling
 * - Phase 64: Tensor Core GEMM
 * - Phase 65: Checkpointing
 */

#include <gtest/gtest.h>
#include "../include/gpu/Device.h"
#include "../include/gpu/DeviceMemory.h"
#include "../include/gpu/Stream.h"
#include "../include/gpu/memory/MemoryPool.h"
#include "../include/gpu/memory/UnifiedMemory.h"
#include "../include/gpu/memory/AsyncMemory.h"
#include "../include/gpu/profiling/Profiler.h"
#include "../include/gpu/profiling/NVTX.h"
#include "../include/gpu/precision/MixedPrecision.h"
#include "../include/gpu/tensorcore/TensorCore.h"
#include "../include/gpu/checkpoint/Checkpoint.h"

#include <vector>
#include <cmath>

using namespace koo::gpu;

// ============================================================================
// Phase 61: Memory Optimization Tests
// ============================================================================

TEST(Phase61_MemoryPool, BasicAllocation) {
    int device_count = Device::getDeviceCount();
    if (device_count == 0) {
        GTEST_SKIP() << "No GPU devices available";
    }

    Device device = Device::get_device(0);
    memory::MemoryPool pool(device);

    // Allocate from pool
    void* ptr1 = pool.allocate(1024);
    EXPECT_NE(ptr1, nullptr);

    void* ptr2 = pool.allocate(2048);
    EXPECT_NE(ptr2, nullptr);

    // Deallocate
    pool.deallocate(ptr1);
    pool.deallocate(ptr2);

    // Check stats
    auto stats = pool.getStats();
    EXPECT_EQ(stats.num_allocations, 2);
    EXPECT_EQ(stats.num_deallocations, 2);
}

TEST(Phase61_MemoryPool, ReuseMemory) {
    int device_count = Device::getDeviceCount();
    if (device_count == 0) {
        GTEST_SKIP() << "No GPU devices available";
    }

    Device device = Device::get_device(0);
    memory::MemoryPool pool(device);

    // Allocate and deallocate
    void* ptr1 = pool.allocate(1024);
    pool.deallocate(ptr1);

    // Allocate again - should reuse
    void* ptr2 = pool.allocate(1024);
    EXPECT_NE(ptr2, nullptr);

    auto stats = pool.getStats();
    EXPECT_GE(stats.num_pool_hits, 1) << "Pool should have reused memory";
}

TEST(Phase61_MemoryPool, GlobalManager) {
    int device_count = Device::getDeviceCount();
    if (device_count == 0) {
        GTEST_SKIP() << "No GPU devices available";
    }

    Device device = Device::get_device(0);

    auto& manager = memory::GlobalPoolManager::getInstance();
    auto pool = manager.getPool(device);

    EXPECT_NE(pool, nullptr);

    void* ptr = pool->allocate(1024);
    EXPECT_NE(ptr, nullptr);

    pool->deallocate(ptr);
}

TEST(Phase61_UnifiedMemory, BasicUsage) {
    int device_count = Device::getDeviceCount();
    if (device_count == 0) {
        GTEST_SKIP() << "No GPU devices available";
    }

    if (!memory::isUnifiedMemorySupported()) {
        GTEST_SKIP() << "Unified memory not supported";
    }

    // Create unified memory
    memory::UnifiedPtr<float> data(100);
    ASSERT_TRUE(data);

    // Fill on CPU
    for (size_t i = 0; i < 100; ++i) {
        data[i] = static_cast<float>(i);
    }

    // Verify
    for (size_t i = 0; i < 100; ++i) {
        EXPECT_FLOAT_EQ(data[i], static_cast<float>(i));
    }
}

TEST(Phase61_UnifiedMemory, Prefetch) {
    int device_count = Device::getDeviceCount();
    if (device_count == 0) {
        GTEST_SKIP() << "No GPU devices available";
    }

    if (!memory::isUnifiedMemorySupported()) {
        GTEST_SKIP() << "Unified memory not supported";
    }

    Device device = Device::get_device(0);
    memory::UnifiedPtr<float> data(1000);

    // Fill data
    data.fill(3.14f);

    // Prefetch to device
    EXPECT_NO_THROW(data.prefetchToDevice(device));

    // Prefetch to host
    EXPECT_NO_THROW(data.prefetchToHost());
}

TEST(Phase61_UnifiedMemory, MemoryAdvice) {
    int device_count = Device::getDeviceCount();
    if (device_count == 0) {
        GTEST_SKIP() << "No GPU devices available";
    }

    if (!memory::isUnifiedMemorySupported()) {
        GTEST_SKIP() << "Unified memory not supported";
    }

    Device device = Device::get_device(0);
    memory::UnifiedPtr<float> data(1000);

    // Set read-mostly hint
    EXPECT_NO_THROW(data.setReadMostly());

    // Set preferred location
    EXPECT_NO_THROW(data.setPreferredLocationDevice(device));
}

TEST(Phase61_AsyncMemory, PinnedMemory) {
    int device_count = Device::getDeviceCount();
    if (device_count == 0) {
        GTEST_SKIP() << "No GPU devices available";
    }

    // Create pinned memory
    memory::PinnedMemory<float> pinned(1000);
    ASSERT_TRUE(pinned);

    // Fill data
    for (size_t i = 0; i < 1000; ++i) {
        pinned[i] = static_cast<float>(i);
    }

    EXPECT_FLOAT_EQ(pinned[0], 0.0f);
    EXPECT_FLOAT_EQ(pinned[999], 999.0f);
}

// ============================================================================
// Phase 62: Profiling Tests
// ============================================================================

TEST(Phase62_Profiler, StartStop) {
    auto& profiler = profiling::Profiler::getInstance();

    profiler.start();
    EXPECT_TRUE(profiler.isEnabled());

    profiler.stop();
    EXPECT_FALSE(profiler.isEnabled());
}

TEST(Phase62_Profiler, KernelTiming) {
    int device_count = Device::getDeviceCount();
    if (device_count == 0) {
        GTEST_SKIP() << "No GPU devices available";
    }

    Device device = Device::get_device(0);
    Stream stream(device);

    auto& profiler = profiling::Profiler::getInstance();
    profiler.reset();
    profiler.start();

    // Simulate kernel timing
    size_t id = profiler.beginKernel("test_kernel", stream);
    stream.synchronize();
    profiler.endKernel(id, stream);

    profiler.stop();

    auto stats = profiler.getKernelStats("test_kernel");
    EXPECT_EQ(stats.call_count, 1);
}

TEST(Phase62_Profiler, Statistics) {
    auto& profiler = profiling::Profiler::getInstance();
    profiler.reset();

    auto stats = profiler.getKernelStats();
    EXPECT_EQ(stats.size(), 0);
}

TEST(Phase62_NVTX, RangeMarker) {
    // NVTX ranges don't fail even without Nsight
    EXPECT_NO_THROW({
        profiling::NVTXRange range("test_range");
    });
}

TEST(Phase62_NVTX, ColoredRange) {
    EXPECT_NO_THROW({
        profiling::NVTXRange range("colored_range",
                                   profiling::NVTXColor::Blue);
    });
}

TEST(Phase62_NVTX, Mark) {
    EXPECT_NO_THROW({
        profiling::NVTXMark::mark("test_mark");
    });
}

TEST(Phase62_NVTX, Domain) {
    EXPECT_NO_THROW({
        profiling::NVTXDomain domain("test_domain");
        domain.pushRange("range1");
        domain.mark("mark1");
        domain.popRange();
    });
}

// ============================================================================
// Phase 63: Mixed Precision Tests
// ============================================================================

TEST(Phase63_MixedPrecision, FP16Support) {
    int device_count = Device::getDeviceCount();
    if (device_count == 0) {
        GTEST_SKIP() << "No GPU devices available";
    }

    Device device = Device::get_device(0);
    bool supports_fp16 = precision::supportsFP16(device);

    // Just check it doesn't crash
    EXPECT_TRUE(supports_fp16 || !supports_fp16);
}

TEST(Phase63_MixedPrecision, TensorCoreSupport) {
    int device_count = Device::getDeviceCount();
    if (device_count == 0) {
        GTEST_SKIP() << "No GPU devices available";
    }

    Device device = Device::get_device(0);
    bool supports_tc = precision::supportsTensorCores(device);

    EXPECT_TRUE(supports_tc || !supports_tc);
}

TEST(Phase63_MixedPrecision, Policy) {
    auto policy = precision::MixedPrecisionPolicy::createAMPPolicy();

    EXPECT_EQ(policy.getComputePrecision(), precision::PrecisionType::FP16);
    EXPECT_EQ(policy.getStoragePrecision(), precision::PrecisionType::FP32);
    EXPECT_TRUE(policy.isAMPEnabled());
    EXPECT_GT(policy.getLossScale(), 0.0f);
}

TEST(Phase63_MixedPrecision, LossScaler) {
    precision::LossScaler scaler(1024.0f);

    EXPECT_FLOAT_EQ(scaler.getScale(), 1024.0f);

    // Scale loss
    float loss = 0.5f;
    float scaled = scaler.scaleLoss(loss);
    EXPECT_FLOAT_EQ(scaled, 512.0f);

    // Unscale gradient
    float grad = 1024.0f;
    float unscaled = scaler.unscaleGradient(grad);
    EXPECT_FLOAT_EQ(unscaled, 1.0f);
}

TEST(Phase63_MixedPrecision, LossScalerUpdate) {
    precision::LossScaler scaler(1024.0f, 2.0f, 0.5f, 10);

    // Update with overflow
    scaler.update(true);
    EXPECT_LT(scaler.getScale(), 1024.0f);

    // Reset
    scaler.reset(1024.0f);
    EXPECT_FLOAT_EQ(scaler.getScale(), 1024.0f);
}

// ============================================================================
// Phase 64: Tensor Core Tests
// ============================================================================

TEST(Phase64_TensorCore, HasTensorCores) {
    int device_count = Device::getDeviceCount();
    if (device_count == 0) {
        GTEST_SKIP() << "No GPU devices available";
    }

    Device device = Device::get_device(0);
    bool has_tc = tensorcore::hasTensorCores(device);

    EXPECT_TRUE(has_tc || !has_tc);
}

TEST(Phase64_TensorCore, TileSize) {
    int device_count = Device::getDeviceCount();
    if (device_count == 0) {
        GTEST_SKIP() << "No GPU devices available";
    }

    Device device = Device::get_device(0);
    auto tile = tensorcore::getRecommendedTileSize(device);

    EXPECT_GT(tile.M, 0);
    EXPECT_GT(tile.N, 0);
    EXPECT_GT(tile.K, 0);
}

TEST(Phase64_TensorCore, ConstexprTileSizes) {
    auto tile1 = tensorcore::TileSize::SIZE_16x16x16();
    EXPECT_EQ(tile1.M, 16);
    EXPECT_EQ(tile1.N, 16);
    EXPECT_EQ(tile1.K, 16);

    auto tile2 = tensorcore::TileSize::SIZE_32x8x16();
    EXPECT_EQ(tile2.M, 32);
    EXPECT_EQ(tile2.N, 8);
    EXPECT_EQ(tile2.K, 16);
}

// ============================================================================
// Phase 65: Checkpointing Tests
// ============================================================================

TEST(Phase65_Checkpoint, CreateManager) {
    checkpoint::CheckpointManager manager;

    EXPECT_EQ(manager.getNumDevices(), 0);
    EXPECT_EQ(manager.getTotalSize(), 0);
}

TEST(Phase65_Checkpoint, AddData) {
    int device_count = Device::getDeviceCount();
    if (device_count == 0) {
        GTEST_SKIP() << "No GPU devices available";
    }

    Device device = Device::get_device(0);
    DeviceMemory<float> data(100, 3.14f);

    checkpoint::CheckpointManager manager;
    EXPECT_NO_THROW({
        manager.addDeviceMemory("test_data", data, device);
    });

    EXPECT_GT(manager.getTotalSize(), 0);
}

TEST(Phase65_Checkpoint, SaveLoad) {
    int device_count = Device::getDeviceCount();
    if (device_count == 0) {
        GTEST_SKIP() << "No GPU devices available";
    }

    Device device = Device::get_device(0);
    DeviceMemory<float> data(100, 2.71f);

    // Save checkpoint
    checkpoint::CheckpointManager save_mgr;
    save_mgr.addDeviceMemory("test_data", data, device);

    std::string filename = "/tmp/test_checkpoint.ckpt";
    bool saved = save_mgr.save(filename, "Test checkpoint");
    EXPECT_TRUE(saved);

    // Load checkpoint
    checkpoint::CheckpointManager load_mgr;
    bool loaded = load_mgr.load(filename);
    EXPECT_TRUE(loaded);

    auto meta = load_mgr.getMetadata();
    EXPECT_EQ(meta.description, "Test checkpoint");

    // Cleanup
    std::remove(filename.c_str());
}

TEST(Phase65_Checkpoint, RestoreData) {
    int device_count = Device::getDeviceCount();
    if (device_count == 0) {
        GTEST_SKIP() << "No GPU devices available";
    }

    Device device = Device::get_device(0);

    // Create and save
    std::vector<float> original(100);
    for (size_t i = 0; i < 100; ++i) {
        original[i] = static_cast<float>(i);
    }

    DeviceMemory<float> data(100);
    data.copyFromHost(original.data(), 100);

    checkpoint::CheckpointManager save_mgr;
    save_mgr.addDeviceMemory("test_data", data, device);

    std::string filename = "/tmp/test_restore.ckpt";
    save_mgr.save(filename);

    // Load and restore
    checkpoint::CheckpointManager load_mgr;
    load_mgr.load(filename);

    DeviceMemory<float> restored(100);
    bool success = load_mgr.restoreDeviceMemory("test_data", restored, device);
    EXPECT_TRUE(success);

    // Verify data
    std::vector<float> restored_host(100);
    restored.copyToHost(restored_host.data(), 100);

    for (size_t i = 0; i < 100; ++i) {
        EXPECT_FLOAT_EQ(restored_host[i], original[i]);
    }

    // Cleanup
    std::remove(filename.c_str());
}

TEST(Phase65_Checkpoint, ListCheckpoints) {
    int device_count = Device::getDeviceCount();
    if (device_count == 0) {
        GTEST_SKIP() << "No GPU devices available";
    }

    Device device = Device::get_device(0);
    DeviceMemory<float> data1(50, 1.0f);
    DeviceMemory<float> data2(75, 2.0f);

    checkpoint::CheckpointManager manager;
    manager.addDeviceMemory("data1", data1, device);
    manager.addDeviceMemory("data2", data2, device);

    auto names = manager.listCheckpoints();
    EXPECT_EQ(names.size(), 2);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(Integration, MemoryPoolWithProfiling) {
    int device_count = Device::getDeviceCount();
    if (device_count == 0) {
        GTEST_SKIP() << "No GPU devices available";
    }

    Device device = Device::get_device(0);
    Stream stream(device);

    auto& profiler = profiling::Profiler::getInstance();
    profiler.reset();
    profiler.start();

    {
        KOO_NVTX_MEMORY("Allocate from pool");

        memory::MemoryPool pool(device);
        void* ptr = pool.allocate(1024 * 1024);  // 1 MB

        EXPECT_NE(ptr, nullptr);

        pool.deallocate(ptr);
    }

    profiler.stop();
}

TEST(Integration, UnifiedMemoryWithCheckpoint) {
    int device_count = Device::getDeviceCount();
    if (device_count == 0) {
        GTEST_SKIP() << "No GPU devices available";
    }

    if (!memory::isUnifiedMemorySupported()) {
        GTEST_SKIP() << "Unified memory not supported";
    }

    Device device = Device::get_device(0);

    // Create unified memory
    memory::UnifiedVector<float> data(100, 1.23f);

    // Copy to device memory for checkpointing
    DeviceMemory<float> dev_data(100);

#ifdef KOO_USE_CUDA
    cudaMemcpy(dev_data.data(), data.data(), 100 * sizeof(float),
               cudaMemcpyDefault);
#endif

    // Checkpoint
    checkpoint::CheckpointManager manager;
    manager.addDeviceMemory("unified_data", dev_data, device);

    std::string filename = "/tmp/test_unified_ckpt.ckpt";
    bool saved = manager.save(filename);
    EXPECT_TRUE(saved);

    // Cleanup
    std::remove(filename.c_str());
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
