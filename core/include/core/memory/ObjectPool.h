/**
 * @file ObjectPool.h
 * @brief Object pooling for efficient memory reuse
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-alpha4
 * @date 2025-11-06
 *
 * This file defines an object pool for efficient allocation and deallocation
 * of objects of the same type. Useful for high-performance applications that
 * frequently create and destroy objects.
 */

#ifndef KOO_CORE_MEMORY_OBJECT_POOL_H
#define KOO_CORE_MEMORY_OBJECT_POOL_H

#include <vector>
#include <memory>
#include <mutex>
#include <stdexcept>

namespace koo {
namespace core {
namespace memory {

/**
 * @brief Object pool for efficient memory reuse
 *
 * This class implements an object pool pattern that pre-allocates a number
 * of objects and reuses them instead of allocating/deallocating repeatedly.
 *
 * Benefits:
 * - Reduced allocation/deallocation overhead
 * - Better cache locality
 * - Predictable memory usage
 * - Thread-safe operations
 *
 * Design Pattern: Object Pool
 *
 * Example usage:
 * @code
 * // Create pool with initial capacity
 * ObjectPool<MyClass> pool(100);
 *
 * // Acquire object from pool
 * MyClass* obj = pool.acquire();
 *
 * // Use the object
 * obj->doSomething();
 *
 * // Return to pool when done
 * pool.release(obj);
 * @endcode
 *
 * @tparam T Type of objects in the pool
 */
template<typename T>
class ObjectPool {
public:
    /**
     * @brief Constructor
     *
     * @param initialCapacity Initial number of objects to allocate
     * @param maxCapacity Maximum pool capacity (0 = unlimited)
     */
    explicit ObjectPool(size_t initialCapacity = 10, size_t maxCapacity = 0)
        : maxCapacity_(maxCapacity), createdObjects_(0), acquiredObjects_(0) {

        pool_.reserve(initialCapacity);

        // Pre-allocate objects
        for (size_t i = 0; i < initialCapacity; ++i) {
            pool_.push_back(new T());
            createdObjects_++;
        }
    }

    /**
     * @brief Destructor - frees all objects
     */
    ~ObjectPool() {
        std::lock_guard<std::mutex> lock(mutex_);

        // Delete all objects
        for (T* obj : pool_) {
            delete obj;
        }
        pool_.clear();
    }

    /**
     * @brief Acquire an object from the pool
     *
     * If the pool is empty, a new object is allocated (unless max capacity reached).
     *
     * @return Pointer to object
     * @throws std::runtime_error if max capacity reached
     */
    T* acquire() {
        std::lock_guard<std::mutex> lock(mutex_);

        T* obj = nullptr;

        if (!pool_.empty()) {
            // Reuse object from pool
            obj = pool_.back();
            pool_.pop_back();
        } else {
            // Check max capacity
            if (maxCapacity_ > 0 && createdObjects_ >= maxCapacity_) {
                throw std::runtime_error("Object pool max capacity reached");
            }

            // Create new object
            obj = new T();
            createdObjects_++;
        }

        acquiredObjects_++;
        return obj;
    }

    /**
     * @brief Acquire an object with constructor arguments
     *
     * Note: Object is default-constructed first, then re-initialized.
     *
     * @tparam Args Constructor argument types
     * @param args Constructor arguments
     * @return Pointer to object
     */
    template<typename... Args>
    T* acquire(Args&&... args) {
        T* obj = acquire();

        // Placement new to re-construct in-place
        obj->~T();  // Destroy old state
        new (obj) T(std::forward<Args>(args)...);  // Construct with args

        return obj;
    }

    /**
     * @brief Release an object back to the pool
     *
     * The object is reset to default state and returned to the pool.
     *
     * @param obj Pointer to object to release
     */
    void release(T* obj) {
        if (obj == nullptr) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        // Reset object to default state (optional, depends on T)
        // For this, we use placement new with default constructor
        obj->~T();
        new (obj) T();

        // Return to pool
        pool_.push_back(obj);
        acquiredObjects_--;
    }

    /**
     * @brief Get number of available objects in pool
     */
    size_t available() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pool_.size();
    }

    /**
     * @brief Get total number of created objects
     */
    size_t created() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return createdObjects_;
    }

    /**
     * @brief Get number of currently acquired objects
     */
    size_t acquired() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return acquiredObjects_;
    }

    /**
     * @brief Reserve additional capacity
     *
     * Pre-allocates more objects to avoid allocation during runtime.
     *
     * @param additionalCapacity Number of additional objects to allocate
     */
    void reserve(size_t additionalCapacity) {
        std::lock_guard<std::mutex> lock(mutex_);

        for (size_t i = 0; i < additionalCapacity; ++i) {
            if (maxCapacity_ > 0 && createdObjects_ >= maxCapacity_) {
                break;
            }

            pool_.push_back(new T());
            createdObjects_++;
        }
    }

    /**
     * @brief Clear all unused objects from pool
     *
     * Frees memory of objects that are currently in the pool.
     * Does not affect acquired objects.
     */
    void shrink() {
        std::lock_guard<std::mutex> lock(mutex_);

        for (T* obj : pool_) {
            delete obj;
            createdObjects_--;
        }
        pool_.clear();
    }

    /**
     * @brief Get statistics
     */
    struct Stats {
        size_t totalCreated;
        size_t currentAcquired;
        size_t available;
        size_t maxCapacity;
    };

    Stats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return Stats{
            createdObjects_,
            acquiredObjects_,
            pool_.size(),
            maxCapacity_
        };
    }

private:
    // Delete copy and move
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;
    ObjectPool(ObjectPool&&) = delete;
    ObjectPool& operator=(ObjectPool&&) = delete;

    mutable std::mutex mutex_;          ///< Thread safety
    std::vector<T*> pool_;              ///< Available objects
    size_t maxCapacity_;                ///< Maximum pool capacity
    size_t createdObjects_;             ///< Total created objects
    size_t acquiredObjects_;            ///< Currently acquired objects
};

/**
 * @brief RAII wrapper for object pool
 *
 * Automatically acquires object on construction and releases on destruction.
 *
 * Example usage:
 * @code
 * ObjectPool<MyClass> pool(100);
 * {
 *     PooledObject<MyClass> obj(pool);
 *     obj->doSomething();  // Use like a pointer
 * }  // Automatically released
 * @endcode
 *
 * @tparam T Type of pooled object
 */
template<typename T>
class PooledObject {
public:
    /**
     * @brief Constructor - acquires object from pool
     */
    explicit PooledObject(ObjectPool<T>& pool)
        : pool_(pool), object_(pool.acquire()) {}

    /**
     * @brief Constructor with arguments - acquires and initializes object
     */
    template<typename... Args>
    PooledObject(ObjectPool<T>& pool, Args&&... args)
        : pool_(pool), object_(pool.template acquire(std::forward<Args>(args)...)) {}

    /**
     * @brief Destructor - releases object back to pool
     */
    ~PooledObject() {
        if (object_) {
            pool_.release(object_);
        }
    }

    /**
     * @brief Get raw pointer
     */
    T* get() { return object_; }
    const T* get() const { return object_; }

    /**
     * @brief Dereference operator
     */
    T& operator*() { return *object_; }
    const T& operator*() const { return *object_; }

    /**
     * @brief Arrow operator
     */
    T* operator->() { return object_; }
    const T* operator->() const { return object_; }

    /**
     * @brief Boolean conversion
     */
    explicit operator bool() const { return object_ != nullptr; }

    /**
     * @brief Release ownership without returning to pool
     */
    T* release() {
        T* tmp = object_;
        object_ = nullptr;
        return tmp;
    }

private:
    // Delete copy and move
    PooledObject(const PooledObject&) = delete;
    PooledObject& operator=(const PooledObject&) = delete;
    PooledObject(PooledObject&&) = delete;
    PooledObject& operator=(PooledObject&&) = delete;

    ObjectPool<T>& pool_;
    T* object_;
};

} // namespace memory
} // namespace core
} // namespace koo

#endif // KOO_CORE_MEMORY_OBJECT_POOL_H
