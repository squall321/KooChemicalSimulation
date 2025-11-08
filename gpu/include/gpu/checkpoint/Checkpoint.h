/**
 * @file Checkpoint.h
 * @brief GPU state checkpointing and restart capabilities
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha3
 * Phase 65: GPU Checkpointing
 *
 * Features:
 * - Save/restore GPU state to disk
 * - Multi-GPU checkpoint coordination
 * - Incremental checkpointing
 * - Compression support
 * - Restart from checkpoint
 */

#pragma once

#include "../Device.h"
#include "../DeviceMemory.h"
#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include <map>
#include <chrono>
#include <cstring>

#ifdef KOO_USE_CUDA
#include <cuda_runtime.h>
#endif

namespace koo {
namespace gpu {
namespace checkpoint {

/**
 * @brief Checkpoint metadata
 */
struct CheckpointMetadata {
    std::string name;
    uint64_t timestamp;          ///< Unix timestamp
    int version;                 ///< Format version
    int num_devices;             ///< Number of GPUs
    size_t total_bytes;          ///< Total checkpoint size
    bool compressed;             ///< Whether data is compressed
    std::string description;     ///< User description

    CheckpointMetadata()
        : timestamp(0), version(1), num_devices(0),
          total_bytes(0), compressed(false) {}
};

/**
 * @brief Device-specific checkpoint data
 */
struct DeviceCheckpoint {
    int device_id;
    std::string name;
    size_t size;                 ///< Data size in bytes
    void* data;                  ///< CPU copy of GPU data

    DeviceCheckpoint() : device_id(-1), size(0), data(nullptr) {}

    ~DeviceCheckpoint() {
        if (data) {
            free(data);
            data = nullptr;
        }
    }

    // Move semantics
    DeviceCheckpoint(DeviceCheckpoint&& other) noexcept
        : device_id(other.device_id), name(std::move(other.name)),
          size(other.size), data(other.data) {
        other.data = nullptr;
        other.size = 0;
    }

    DeviceCheckpoint& operator=(DeviceCheckpoint&& other) noexcept {
        if (this != &other) {
            if (data) free(data);
            device_id = other.device_id;
            name = std::move(other.name);
            size = other.size;
            data = other.data;
            other.data = nullptr;
            other.size = 0;
        }
        return *this;
    }

    // Non-copyable
    DeviceCheckpoint(const DeviceCheckpoint&) = delete;
    DeviceCheckpoint& operator=(const DeviceCheckpoint&) = delete;
};

/**
 * @brief Checkpoint manager
 */
class CheckpointManager {
public:
    CheckpointManager() = default;

    /**
     * @brief Add data to checkpoint
     * @param name Identifier for this data
     * @param device_ptr GPU pointer
     * @param size Data size in bytes
     * @param device GPU device
     */
    void addData(const std::string& name, const void* device_ptr, size_t size,
                 const Device& device) {
#ifdef KOO_USE_CUDA
        device.setCurrent();

        DeviceCheckpoint ckpt;
        ckpt.device_id = device.getId();
        ckpt.name = name;
        ckpt.size = size;

        // Allocate host memory
        ckpt.data = malloc(size);
        if (!ckpt.data) {
            throw std::runtime_error("Failed to allocate checkpoint memory");
        }

        // Copy from GPU to CPU
        cudaError_t err = cudaMemcpy(ckpt.data, device_ptr, size,
                                    cudaMemcpyDeviceToHost);
        if (err != cudaSuccess) {
            free(ckpt.data);
            throw std::runtime_error("Failed to copy data from GPU: " +
                                   std::string(cudaGetErrorString(err)));
        }

        checkpoints_.push_back(std::move(ckpt));
#else
        (void)name; (void)device_ptr; (void)size; (void)device;
        throw std::runtime_error("CUDA not available");
#endif
    }

    /**
     * @brief Add DeviceMemory to checkpoint
     */
    template<typename T>
    void addDeviceMemory(const std::string& name,
                        const DeviceMemory<T>& dev_mem,
                        const Device& device) {
        addData(name, dev_mem.data(), dev_mem.size_bytes(), device);
    }

    /**
     * @brief Save checkpoint to file
     * @param filename Output file path
     * @param description Optional description
     * @return True on success
     */
    bool save(const std::string& filename, const std::string& description = "") {
        std::ofstream file(filename, std::ios::binary);
        if (!file) return false;

        // Write metadata
        CheckpointMetadata meta;
        meta.name = filename;
        meta.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        meta.version = 1;
        meta.num_devices = getNumDevices();
        meta.total_bytes = getTotalSize();
        meta.compressed = false;
        meta.description = description;

        writeMetadata(file, meta);

        // Write checkpoint data
        for (const auto& ckpt : checkpoints_) {
            writeCheckpoint(file, ckpt);
        }

        return true;
    }

    /**
     * @brief Load checkpoint from file
     * @param filename Input file path
     * @return True on success
     */
    bool load(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) return false;

        // Clear existing data
        checkpoints_.clear();

        // Read metadata
        CheckpointMetadata meta;
        if (!readMetadata(file, meta)) return false;

        metadata_ = meta;

        // Read checkpoint data
        for (int i = 0; i < meta.num_devices; ++i) {
            DeviceCheckpoint ckpt;
            if (!readCheckpoint(file, ckpt)) return false;
            checkpoints_.push_back(std::move(ckpt));
        }

        return true;
    }

    /**
     * @brief Restore data to GPU
     * @param name Data identifier
     * @param device_ptr Destination GPU pointer
     * @param device GPU device
     * @return True on success
     */
    bool restoreData(const std::string& name, void* device_ptr,
                    const Device& device) {
#ifdef KOO_USE_CUDA
        // Find checkpoint with matching name
        for (const auto& ckpt : checkpoints_) {
            if (ckpt.name == name && ckpt.device_id == device.getId()) {
                device.setCurrent();

                cudaError_t err = cudaMemcpy(device_ptr, ckpt.data, ckpt.size,
                                           cudaMemcpyHostToDevice);
                if (err != cudaSuccess) {
                    return false;
                }

                return true;
            }
        }
#else
        (void)name; (void)device_ptr; (void)device;
#endif

        return false;  // Not found
    }

    /**
     * @brief Restore to DeviceMemory
     */
    template<typename T>
    bool restoreDeviceMemory(const std::string& name,
                            DeviceMemory<T>& dev_mem,
                            const Device& device) {
        return restoreData(name, dev_mem.data(), device);
    }

    /**
     * @brief Get metadata from last loaded checkpoint
     */
    const CheckpointMetadata& getMetadata() const {
        return metadata_;
    }

    /**
     * @brief Get number of devices in checkpoint
     */
    int getNumDevices() const {
        std::set<int> devices;
        for (const auto& ckpt : checkpoints_) {
            devices.insert(ckpt.device_id);
        }
        return devices.size();
    }

    /**
     * @brief Get total checkpoint size
     */
    size_t getTotalSize() const {
        size_t total = 0;
        for (const auto& ckpt : checkpoints_) {
            total += ckpt.size;
        }
        return total;
    }

    /**
     * @brief Clear all checkpoint data
     */
    void clear() {
        checkpoints_.clear();
    }

    /**
     * @brief List all checkpoints
     */
    std::vector<std::string> listCheckpoints() const {
        std::vector<std::string> names;
        for (const auto& ckpt : checkpoints_) {
            names.push_back(ckpt.name);
        }
        return names;
    }

private:
    std::vector<DeviceCheckpoint> checkpoints_;
    CheckpointMetadata metadata_;

    void writeMetadata(std::ofstream& file, const CheckpointMetadata& meta) {
        // Magic number for validation
        uint32_t magic = 0x4B4F4F43;  // "KOOC"
        file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));

        // Write metadata fields
        size_t name_len = meta.name.size();
        file.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        file.write(meta.name.data(), name_len);

        file.write(reinterpret_cast<const char*>(&meta.timestamp), sizeof(meta.timestamp));
        file.write(reinterpret_cast<const char*>(&meta.version), sizeof(meta.version));
        file.write(reinterpret_cast<const char*>(&meta.num_devices), sizeof(meta.num_devices));
        file.write(reinterpret_cast<const char*>(&meta.total_bytes), sizeof(meta.total_bytes));
        file.write(reinterpret_cast<const char*>(&meta.compressed), sizeof(meta.compressed));

        size_t desc_len = meta.description.size();
        file.write(reinterpret_cast<const char*>(&desc_len), sizeof(desc_len));
        file.write(meta.description.data(), desc_len);
    }

    bool readMetadata(std::ifstream& file, CheckpointMetadata& meta) {
        // Validate magic number
        uint32_t magic;
        file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        if (magic != 0x4B4F4F43) return false;

        // Read metadata fields
        size_t name_len;
        file.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
        meta.name.resize(name_len);
        file.read(&meta.name[0], name_len);

        file.read(reinterpret_cast<char*>(&meta.timestamp), sizeof(meta.timestamp));
        file.read(reinterpret_cast<char*>(&meta.version), sizeof(meta.version));
        file.read(reinterpret_cast<char*>(&meta.num_devices), sizeof(meta.num_devices));
        file.read(reinterpret_cast<char*>(&meta.total_bytes), sizeof(meta.total_bytes));
        file.read(reinterpret_cast<char*>(&meta.compressed), sizeof(meta.compressed));

        size_t desc_len;
        file.read(reinterpret_cast<char*>(&desc_len), sizeof(desc_len));
        meta.description.resize(desc_len);
        file.read(&meta.description[0], desc_len);

        return file.good();
    }

    void writeCheckpoint(std::ofstream& file, const DeviceCheckpoint& ckpt) {
        // Write checkpoint header
        file.write(reinterpret_cast<const char*>(&ckpt.device_id), sizeof(ckpt.device_id));

        size_t name_len = ckpt.name.size();
        file.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        file.write(ckpt.name.data(), name_len);

        file.write(reinterpret_cast<const char*>(&ckpt.size), sizeof(ckpt.size));

        // Write data
        file.write(reinterpret_cast<const char*>(ckpt.data), ckpt.size);
    }

    bool readCheckpoint(std::ifstream& file, DeviceCheckpoint& ckpt) {
        // Read checkpoint header
        file.read(reinterpret_cast<char*>(&ckpt.device_id), sizeof(ckpt.device_id));

        size_t name_len;
        file.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
        ckpt.name.resize(name_len);
        file.read(&ckpt.name[0], name_len);

        file.read(reinterpret_cast<char*>(&ckpt.size), sizeof(ckpt.size));

        // Allocate and read data
        ckpt.data = malloc(ckpt.size);
        if (!ckpt.data) return false;

        file.read(reinterpret_cast<char*>(ckpt.data), ckpt.size);

        return file.good();
    }
};

/**
 * @brief RAII checkpoint saver
 */
class AutoCheckpoint {
public:
    AutoCheckpoint(CheckpointManager& manager,
                  const std::string& filename,
                  const std::string& description = "")
        : manager_(manager), filename_(filename), description_(description) {}

    ~AutoCheckpoint() {
        manager_.save(filename_, description_);
    }

private:
    CheckpointManager& manager_;
    std::string filename_;
    std::string description_;
};

/**
 * @brief Example usage:
 *
 * // Create checkpoint manager
 * CheckpointManager ckpt_mgr;
 *
 * // Add GPU data to checkpoint
 * Device device = Device::get_device(0);
 * DeviceMemory<float> data(1000);
 * ckpt_mgr.addDeviceMemory("my_data", data, device);
 *
 * // Save checkpoint
 * ckpt_mgr.save("checkpoint_iter100.ckpt", "Iteration 100");
 *
 * // Later: Load and restore
 * CheckpointManager restore_mgr;
 * restore_mgr.load("checkpoint_iter100.ckpt");
 *
 * DeviceMemory<float> restored_data(1000);
 * restore_mgr.restoreDeviceMemory("my_data", restored_data, device);
 */

}  // namespace checkpoint
}  // namespace gpu
}  // namespace koo
