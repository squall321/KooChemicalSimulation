/**
 * @file SmartPtr.h
 * @brief Smart pointer wrappers with custom memory management
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-alpha4
 * @date 2025-11-06
 *
 * This file defines smart pointer wrappers that integrate with the
 * framework's memory management system.
 */

#ifndef KOO_CORE_MEMORY_SMART_PTR_H
#define KOO_CORE_MEMORY_SMART_PTR_H

#include "MemoryManager.h"
#include <memory>
#include <functional>

namespace koo {
namespace core {
namespace memory {

/**
 * @brief Custom deleter for unique_ptr that uses MemoryManager
 *
 * @tparam T Type of object
 */
template<typename T>
struct TrackedDeleter {
    void operator()(T* ptr) const {
        if (ptr) {
            ptr->~T();  // Call destructor
            MemoryManager::getInstance().deallocate(ptr);
        }
    }
};

/**
 * @brief Unique pointer with memory tracking
 *
 * This is a typedef for std::unique_ptr with a custom deleter that
 * integrates with MemoryManager.
 *
 * Example usage:
 * @code
 * auto ptr = makeUnique<MyClass>(arg1, arg2);
 * @endcode
 *
 * @tparam T Type of managed object
 */
template<typename T>
using UniquePtr = std::unique_ptr<T, TrackedDeleter<T>>;

/**
 * @brief Create unique pointer with tracking
 *
 * @tparam T Type to create
 * @tparam Args Constructor argument types
 * @param args Constructor arguments
 * @return UniquePtr to new object
 */
template<typename T, typename... Args>
UniquePtr<T> makeUnique(Args&&... args) {
    void* memory = MemoryManager::getInstance().allocate(
        sizeof(T), __FILE__, __LINE__, __FUNCTION__);

    T* ptr = new (memory) T(std::forward<Args>(args)...);
    return UniquePtr<T>(ptr, TrackedDeleter<T>());
}

/**
 * @brief Shared pointer with memory tracking
 *
 * For shared pointers, we use std::shared_ptr with a custom deleter.
 *
 * @tparam T Type of managed object
 */
template<typename T>
using SharedPtr = std::shared_ptr<T>;

/**
 * @brief Create shared pointer with tracking
 *
 * @tparam T Type to create
 * @tparam Args Constructor argument types
 * @param args Constructor arguments
 * @return SharedPtr to new object
 */
template<typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
    void* memory = MemoryManager::getInstance().allocate(
        sizeof(T), __FILE__, __LINE__, __FUNCTION__);

    T* ptr = new (memory) T(std::forward<Args>(args)...);

    auto deleter = [](T* p) {
        if (p) {
            p->~T();
            MemoryManager::getInstance().deallocate(p);
        }
    };

    return SharedPtr<T>(ptr, deleter);
}

/**
 * @brief Weak pointer (standard)
 *
 * @tparam T Type of managed object
 */
template<typename T>
using WeakPtr = std::weak_ptr<T>;

// ============================================================================
// RAII Helper Classes
// ============================================================================

/**
 * @brief RAII wrapper for resources
 *
 * Generic RAII wrapper that calls a cleanup function on destruction.
 *
 * Example usage:
 * @code
 * FILE* file = fopen("data.txt", "r");
 * auto guard = makeScopeGuard([file]() { fclose(file); });
 * // File automatically closed when guard goes out of scope
 * @endcode
 */
class ScopeGuard {
public:
    /**
     * @brief Constructor with cleanup function
     */
    explicit ScopeGuard(std::function<void()> cleanup)
        : cleanup_(std::move(cleanup)), active_(true) {}

    /**
     * @brief Destructor - calls cleanup
     */
    ~ScopeGuard() {
        if (active_ && cleanup_) {
            cleanup_();
        }
    }

    /**
     * @brief Dismiss the guard (don't call cleanup)
     */
    void dismiss() {
        active_ = false;
    }

    /**
     * @brief Execute cleanup now
     */
    void execute() {
        if (active_ && cleanup_) {
            cleanup_();
            active_ = false;
        }
    }

private:
    // Delete copy and move
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard(ScopeGuard&&) = delete;
    ScopeGuard& operator=(ScopeGuard&&) = delete;

    std::function<void()> cleanup_;
    bool active_;
};

/**
 * @brief Create scope guard
 */
inline ScopeGuard makeScopeGuard(std::function<void()> cleanup) {
    return ScopeGuard(std::move(cleanup));
}

/**
 * @brief RAII wrapper for generic resources
 *
 * Manages a resource with acquire/release functions.
 *
 * Example usage:
 * @code
 * auto resource = ResourceGuard<FILE*>(
 *     fopen("data.txt", "r"),
 *     [](FILE* f) { if (f) fclose(f); }
 * );
 * fprintf(resource.get(), "Hello\n");
 * @endcode
 *
 * @tparam T Resource type
 */
template<typename T>
class ResourceGuard {
public:
    /**
     * @brief Constructor
     *
     * @param resource The resource to manage
     * @param deleter Function to release resource
     */
    ResourceGuard(T resource, std::function<void(T)> deleter)
        : resource_(resource), deleter_(std::move(deleter)), ownsResource_(true) {}

    /**
     * @brief Destructor - releases resource
     */
    ~ResourceGuard() {
        if (ownsResource_ && deleter_) {
            deleter_(resource_);
        }
    }

    /**
     * @brief Get the resource
     */
    T get() { return resource_; }
    const T get() const { return resource_; }

    /**
     * @brief Release ownership
     */
    T release() {
        ownsResource_ = false;
        return resource_;
    }

    /**
     * @brief Reset with new resource
     */
    void reset(T newResource = T()) {
        if (ownsResource_ && deleter_) {
            deleter_(resource_);
        }
        resource_ = newResource;
        ownsResource_ = true;
    }

    /**
     * @brief Boolean conversion
     */
    explicit operator bool() const { return ownsResource_; }

private:
    // Delete copy and move
    ResourceGuard(const ResourceGuard&) = delete;
    ResourceGuard& operator=(const ResourceGuard&) = delete;
    ResourceGuard(ResourceGuard&&) = delete;
    ResourceGuard& operator=(ResourceGuard&&) = delete;

    T resource_;
    std::function<void(T)> deleter_;
    bool ownsResource_;
};

/**
 * @brief Create resource guard
 */
template<typename T, typename Deleter>
ResourceGuard<T> makeResourceGuard(T resource, Deleter&& deleter) {
    return ResourceGuard<T>(resource, std::function<void(T)>(std::forward<Deleter>(deleter)));
}

// ============================================================================
// Array Wrappers
// ============================================================================

/**
 * @brief Unique pointer for arrays with tracking
 *
 * @tparam T Array element type
 */
template<typename T>
class UniqueArrayPtr {
public:
    /**
     * @brief Constructor
     *
     * @param size Array size
     */
    explicit UniqueArrayPtr(size_t size)
        : size_(size), ptr_(nullptr) {

        if (size > 0) {
            void* memory = MemoryManager::getInstance().allocate(
                sizeof(T) * size, __FILE__, __LINE__, __FUNCTION__);
            ptr_ = static_cast<T*>(memory);

            // Construct elements
            for (size_t i = 0; i < size; ++i) {
                new (&ptr_[i]) T();
            }
        }
    }

    /**
     * @brief Destructor
     */
    ~UniqueArrayPtr() {
        if (ptr_) {
            // Destroy elements
            for (size_t i = 0; i < size_; ++i) {
                ptr_[i].~T();
            }
            MemoryManager::getInstance().deallocate(ptr_);
        }
    }

    /**
     * @brief Array access
     */
    T& operator[](size_t index) { return ptr_[index]; }
    const T& operator[](size_t index) const { return ptr_[index]; }

    /**
     * @brief Get raw pointer
     */
    T* get() { return ptr_; }
    const T* get() const { return ptr_; }

    /**
     * @brief Get size
     */
    size_t size() const { return size_; }

    /**
     * @brief Boolean conversion
     */
    explicit operator bool() const { return ptr_ != nullptr; }

private:
    // Delete copy and move
    UniqueArrayPtr(const UniqueArrayPtr&) = delete;
    UniqueArrayPtr& operator=(const UniqueArrayPtr&) = delete;
    UniqueArrayPtr(UniqueArrayPtr&&) = delete;
    UniqueArrayPtr& operator=(UniqueArrayPtr&&) = delete;

    size_t size_;
    T* ptr_;
};

/**
 * @brief Create unique array pointer
 */
template<typename T>
UniqueArrayPtr<T> makeUniqueArray(size_t size) {
    return UniqueArrayPtr<T>(size);
}

} // namespace memory
} // namespace core
} // namespace koo

#endif // KOO_CORE_MEMORY_SMART_PTR_H
