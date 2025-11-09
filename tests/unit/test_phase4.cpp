/**
 * @file test_phase4.cpp
 * @brief Unit tests for Phase 4 - Memory Management
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-alpha4
 * @date 2025-11-06
 *
 * Tests:
 * 1. MemoryManager allocation/deallocation tracking
 * 2. Memory leak detection
 * 3. ObjectPool acquire/release cycles
 * 4. Smart pointer usage (makeUnique, makeShared)
 * 5. RAII patterns (ScopeGuard, ResourceGuard)
 * 6. UniqueArrayPtr functionality
 */

#include "core/memory/MemoryManager.h"
#include "core/memory/ObjectPool.h"
#include "core/memory/SmartPtr.h"

#include <iostream>
#include <cassert>
#include <string>
#include <cmath>

using namespace koo::core::memory;

// ============================================================================
// Test Helper Class
// ============================================================================

class TestObject {
public:
    TestObject() : value_(0), id_(nextId_++) {
        constructorCalls_++;
    }

    explicit TestObject(int value) : value_(value), id_(nextId_++) {
        constructorCalls_++;
    }

    TestObject(int value, const std::string& name)
        : value_(value), name_(name), id_(nextId_++) {
        constructorCalls_++;
    }

    ~TestObject() {
        destructorCalls_++;
    }

    int getValue() const { return value_; }
    void setValue(int value) { value_ = value; }
    std::string getName() const { return name_; }
    size_t getId() const { return id_; }

    static void resetCounters() {
        constructorCalls_ = 0;
        destructorCalls_ = 0;
        nextId_ = 0;
    }

    static size_t getConstructorCalls() { return constructorCalls_; }
    static size_t getDestructorCalls() { return destructorCalls_; }

private:
    int value_;
    std::string name_;
    size_t id_;

    static size_t constructorCalls_;
    static size_t destructorCalls_;
    static size_t nextId_;
};

size_t TestObject::constructorCalls_ = 0;
size_t TestObject::destructorCalls_ = 0;
size_t TestObject::nextId_ = 0;

// ============================================================================
// Test 1: MemoryManager Basic Operations
// ============================================================================

void testMemoryManagerBasics() {
    std::cout << "\n[Test 1] MemoryManager Basic Operations\n";
    std::cout << "==========================================\n";

    auto& memMgr = MemoryManager::getInstance();

    // Reset and enable tracking
    memMgr.resetStats();
    memMgr.enableTracking();

    // Test allocation
    void* ptr1 = memMgr.allocate(1024, __FILE__, __LINE__, __FUNCTION__);
    assert(ptr1 != nullptr && "Allocation should succeed");

    [[maybe_unused]] auto stats1 = memMgr.getStats();
    assert(stats1.allocationCount == 1 && "Should have 1 allocation");
    assert(stats1.totalAllocated == 1024 && "Should have allocated 1024 bytes");
    assert(stats1.currentUsage == 1024 && "Current usage should be 1024 bytes");
    assert(stats1.activeAllocations == 1 && "Should have 1 active allocation");

    // Test deallocation
    memMgr.deallocate(ptr1);

    [[maybe_unused]] auto stats2 = memMgr.getStats();
    assert(stats2.freeCount == 1 && "Should have 1 deallocation");
    assert(stats2.totalFreed == 1024 && "Should have freed 1024 bytes");
    assert(stats2.currentUsage == 0 && "Current usage should be 0");
    assert(stats2.activeAllocations == 0 && "Should have 0 active allocations");
    assert(!stats2.hasLeaks() && "Should have no leaks");

    std::cout << "✓ Basic allocation/deallocation works\n";
    std::cout << "✓ Statistics tracking works\n";
}

// ============================================================================
// Test 2: MemoryManager Leak Detection
// ============================================================================

void testMemoryLeakDetection() {
    std::cout << "\n[Test 2] Memory Leak Detection\n";
    std::cout << "==========================================\n";

    auto& memMgr = MemoryManager::getInstance();

    // Reset and enable tracking
    memMgr.resetStats();
    memMgr.enableTracking();

    // Allocate without deallocating to simulate leak
    void* leak1 = memMgr.allocate(512, __FILE__, __LINE__, __FUNCTION__);
    void* leak2 = memMgr.allocate(256, __FILE__, __LINE__, __FUNCTION__);

    [[maybe_unused]] auto stats = memMgr.getStats();
    assert(stats.hasLeaks() && "Should detect leaks");
    assert(stats.leakAmount() == 768 && "Should report 768 bytes leaked");
    assert(stats.activeAllocations == 2 && "Should have 2 active allocations");

    // Get active allocations
    auto activeAllocs = memMgr.getActiveAllocations();
    assert(activeAllocs.size() == 2 && "Should have 2 active allocations");

    std::cout << "✓ Leak detection works\n";
    std::cout << "✓ Active allocation tracking works\n";

    // Clean up leaks
    memMgr.deallocate(leak1);
    memMgr.deallocate(leak2);

    auto statsClean = memMgr.getStats();
    (void)statsClean;  // Used in assert, suppress unused warning in Release mode
    assert(!statsClean.hasLeaks() && "Leaks should be cleaned up");
}

// ============================================================================
// Test 3: MemoryManager Macros
// ============================================================================

void testMemoryManagerMacros() {
    std::cout << "\n[Test 3] MemoryManager Macros\n";
    std::cout << "==========================================\n";

    auto& memMgr = MemoryManager::getInstance();

    // Reset and enable tracking
    memMgr.resetStats();
    memMgr.enableTracking();

    // Test KOO_ALLOCATE macro
    void* ptr = KOO_ALLOCATE(2048);
    assert(ptr != nullptr && "KOO_ALLOCATE should work");

    [[maybe_unused]] auto stats1 = memMgr.getStats();
    assert(stats1.totalAllocated == 2048 && "Should have allocated 2048 bytes");

    // Test KOO_DEALLOCATE macro
    KOO_DEALLOCATE(ptr);

    [[maybe_unused]] auto stats2 = memMgr.getStats();
    assert(stats2.totalFreed == 2048 && "Should have freed 2048 bytes");

    // Test KOO_NEW_ARRAY macro
    int* arr = KOO_NEW_ARRAY(int, 100);
    assert(arr != nullptr && "KOO_NEW_ARRAY should work");

    [[maybe_unused]] auto stats3 = memMgr.getStats();
    assert(stats3.totalAllocated == 2048 + sizeof(int) * 100 && "Array allocation tracked");

    // Test KOO_DELETE_ARRAY macro
    KOO_DELETE_ARRAY(arr);

    [[maybe_unused]] auto stats4 = memMgr.getStats();
    assert(!stats4.hasLeaks() && "Should have no leaks");

    std::cout << "✓ KOO_ALLOCATE/DEALLOCATE macros work\n";
    std::cout << "✓ KOO_NEW_ARRAY/DELETE_ARRAY macros work\n";
}

// ============================================================================
// Test 4: ObjectPool Basic Operations
// ============================================================================

void testObjectPoolBasics() {
    std::cout << "\n[Test 4] ObjectPool Basic Operations\n";
    std::cout << "==========================================\n";

    TestObject::resetCounters();

    // Create pool with initial capacity of 5
    ObjectPool<TestObject> pool(5);

    // Check initial state
    assert(pool.available() == 5 && "Should have 5 available objects");
    assert(pool.created() == 5 && "Should have created 5 objects");
    assert(pool.acquired() == 0 && "Should have 0 acquired objects");

    // Acquire an object
    TestObject* obj1 = pool.acquire();
    assert(obj1 != nullptr && "Should acquire object");
    assert(pool.available() == 4 && "Should have 4 available objects");
    assert(pool.acquired() == 1 && "Should have 1 acquired object");

    // Use the object
    obj1->setValue(42);
    assert(obj1->getValue() == 42 && "Object should work");

    // Release the object
    pool.release(obj1);
    assert(pool.available() == 5 && "Should have 5 available objects after release");
    assert(pool.acquired() == 0 && "Should have 0 acquired objects");

    // Acquire multiple objects
    TestObject* objs[7];
    for (int i = 0; i < 7; ++i) {
        objs[i] = pool.acquire();
        assert(objs[i] != nullptr && "Should acquire object");
    }

    assert(pool.available() == 0 && "Should have 0 available objects");
    assert(pool.acquired() == 7 && "Should have 7 acquired objects");
    assert(pool.created() == 7 && "Should have created 7 objects total (5 + 2 new)");

    // Release all
    for (int i = 0; i < 7; ++i) {
        pool.release(objs[i]);
    }

    assert(pool.available() == 7 && "Should have 7 available objects");
    assert(pool.acquired() == 0 && "Should have 0 acquired objects");

    std::cout << "✓ ObjectPool acquire/release works\n";
    std::cout << "✓ ObjectPool dynamic growth works\n";
    std::cout << "✓ ObjectPool statistics work\n";
}

// ============================================================================
// Test 5: ObjectPool with Arguments
// ============================================================================

void testObjectPoolWithArgs() {
    std::cout << "\n[Test 5] ObjectPool with Arguments\n";
    std::cout << "==========================================\n";

    TestObject::resetCounters();

    ObjectPool<TestObject> pool(3);

    // Acquire with arguments
    TestObject* obj1 = pool.acquire(100, "test1");
    assert(obj1 != nullptr && "Should acquire object");
    assert(obj1->getValue() == 100 && "Should initialize with value");
    assert(obj1->getName() == "test1" && "Should initialize with name");

    TestObject* obj2 = pool.acquire(200, "test2");
    assert(obj2->getValue() == 200 && "Should initialize with different value");
    assert(obj2->getName() == "test2" && "Should initialize with different name");

    // Release and reacquire
    pool.release(obj1);
    TestObject* obj3 = pool.acquire(300, "test3");
    assert(obj3->getValue() == 300 && "Should reinitialize with new value");
    assert(obj3->getName() == "test3" && "Should reinitialize with new name");

    pool.release(obj2);
    pool.release(obj3);

    std::cout << "✓ ObjectPool acquire with arguments works\n";
}

// ============================================================================
// Test 6: ObjectPool Max Capacity
// ============================================================================

void testObjectPoolMaxCapacity() {
    std::cout << "\n[Test 6] ObjectPool Max Capacity\n";
    std::cout << "==========================================\n";

    TestObject::resetCounters();

    // Create pool with max capacity of 3
    ObjectPool<TestObject> pool(2, 3);

    assert(pool.created() == 2 && "Should have created 2 objects");

    // Acquire all objects
    TestObject* obj1 = pool.acquire();
    TestObject* obj2 = pool.acquire();
    TestObject* obj3 = pool.acquire(); // This should create a new one (total = 3)

    assert(pool.created() == 3 && "Should have created 3 objects");

    // Try to acquire beyond max capacity - should throw
    [[maybe_unused]] bool exceptionThrown = false;
    try {
        TestObject* obj4 = pool.acquire();
        (void)obj4; // Suppress unused variable warning
    } catch (const std::runtime_error& e) {
        exceptionThrown = true;
    }

    assert(exceptionThrown && "Should throw when max capacity reached");

    // Release and verify we can acquire again
    pool.release(obj1);
    TestObject* obj4 = pool.acquire(); // Should succeed now
    assert(obj4 != nullptr && "Should acquire after release");

    pool.release(obj2);
    pool.release(obj3);
    pool.release(obj4);

    std::cout << "✓ ObjectPool max capacity enforcement works\n";
}

// ============================================================================
// Test 7: PooledObject RAII Wrapper
// ============================================================================

void testPooledObject() {
    std::cout << "\n[Test 7] PooledObject RAII Wrapper\n";
    std::cout << "==========================================\n";

    TestObject::resetCounters();

    ObjectPool<TestObject> pool(5);

    // Test automatic release via RAII
    {
        PooledObject<TestObject> obj(pool);
        assert(obj.get() != nullptr && "Should have object");
        assert(pool.acquired() == 1 && "Should have 1 acquired object");

        obj->setValue(99);
        assert(obj->getValue() == 99 && "Arrow operator should work");

        (*obj).setValue(88);
        assert(obj->getValue() == 88 && "Dereference operator should work");

    } // obj destroyed here, should auto-release

    assert(pool.acquired() == 0 && "Should have 0 acquired objects after scope");
    assert(pool.available() == 5 && "Object should be returned to pool");

    // Test with arguments
    {
        PooledObject<TestObject> obj(pool, 123, "pooled");
        assert(obj->getValue() == 123 && "Should initialize with arguments");
        assert(obj->getName() == "pooled" && "Should initialize name");
    }

    assert(pool.acquired() == 0 && "Should auto-release with args too");

    std::cout << "✓ PooledObject RAII wrapper works\n";
    std::cout << "✓ Automatic release on destruction works\n";
}

// ============================================================================
// Test 8: Smart Pointers (UniquePtr)
// ============================================================================

void testUniquePtr() {
    std::cout << "\n[Test 8] Smart Pointers (UniquePtr)\n";
    std::cout << "==========================================\n";

    auto& memMgr = MemoryManager::getInstance();
    memMgr.resetStats();
    memMgr.enableTracking();

    TestObject::resetCounters();

    {
        // Create unique pointer
        auto ptr = makeUnique<TestObject>(42, "unique");
        assert(ptr != nullptr && "makeUnique should work");
        assert(ptr->getValue() == 42 && "Should initialize with value");
        assert(ptr->getName() == "unique" && "Should initialize with name");

        [[maybe_unused]] auto stats1 = memMgr.getStats();
        assert(stats1.activeAllocations == 1 && "Should track allocation");

    } // ptr destroyed here

    [[maybe_unused]] auto stats2 = memMgr.getStats();
    assert(stats2.activeAllocations == 0 && "Should deallocate on destruction");
    assert(!stats2.hasLeaks() && "Should have no leaks");

    assert(TestObject::getConstructorCalls() == 1 && "Should call constructor once");
    assert(TestObject::getDestructorCalls() == 1 && "Should call destructor once");

    std::cout << "✓ makeUnique works\n";
    std::cout << "✓ Automatic deallocation works\n";
    std::cout << "✓ TrackedDeleter integrates with MemoryManager\n";
}

// ============================================================================
// Test 9: Smart Pointers (SharedPtr)
// ============================================================================

void testSharedPtr() {
    std::cout << "\n[Test 9] Smart Pointers (SharedPtr)\n";
    std::cout << "==========================================\n";

    auto& memMgr = MemoryManager::getInstance();
    memMgr.resetStats();
    memMgr.enableTracking();

    TestObject::resetCounters();

    {
        auto ptr1 = makeShared<TestObject>(100, "shared");
        assert(ptr1 != nullptr && "makeShared should work");
        assert(ptr1.use_count() == 1 && "Should have 1 reference");

        {
            auto ptr2 = ptr1; // Share ownership
            assert(ptr1.use_count() == 2 && "Should have 2 references");
            assert(ptr2->getValue() == 100 && "Both pointers should access same object");

            [[maybe_unused]] auto stats = memMgr.getStats();
            assert(stats.activeAllocations == 1 && "Should track only 1 allocation");

        } // ptr2 destroyed

        assert(ptr1.use_count() == 1 && "Should have 1 reference again");

    } // ptr1 destroyed, object should be deleted

    [[maybe_unused]] auto stats = memMgr.getStats();
    assert(stats.activeAllocations == 0 && "Should deallocate when last ref gone");
    assert(!stats.hasLeaks() && "Should have no leaks");

    assert(TestObject::getConstructorCalls() == 1 && "Should construct once");
    assert(TestObject::getDestructorCalls() == 1 && "Should destruct once");

    std::cout << "✓ makeShared works\n";
    std::cout << "✓ Reference counting works\n";
    std::cout << "✓ Automatic deallocation on last reference works\n";
}

// ============================================================================
// Test 10: ScopeGuard RAII
// ============================================================================

void testScopeGuard() {
    std::cout << "\n[Test 10] ScopeGuard RAII\n";
    std::cout << "==========================================\n";

    int cleanupCalled = 0;

    // Test automatic cleanup
    {
        auto guard = makeScopeGuard([&cleanupCalled]() {
            cleanupCalled++;
        });

        assert(cleanupCalled == 0 && "Cleanup not called yet");

    } // guard destroyed, cleanup should be called

    assert(cleanupCalled == 1 && "Cleanup should be called on destruction");

    // Test dismiss
    cleanupCalled = 0;
    {
        auto guard = makeScopeGuard([&cleanupCalled]() {
            cleanupCalled++;
        });

        guard.dismiss(); // Cancel cleanup

    } // guard destroyed but dismissed

    assert(cleanupCalled == 0 && "Cleanup should not be called after dismiss");

    // Test manual execute
    cleanupCalled = 0;
    {
        auto guard = makeScopeGuard([&cleanupCalled]() {
            cleanupCalled++;
        });

        guard.execute(); // Manual cleanup
        assert(cleanupCalled == 1 && "Manual execute should call cleanup");

    } // guard destroyed but already executed

    assert(cleanupCalled == 1 && "Cleanup should only be called once");

    std::cout << "✓ ScopeGuard automatic cleanup works\n";
    std::cout << "✓ ScopeGuard dismiss works\n";
    std::cout << "✓ ScopeGuard manual execute works\n";
}

// ============================================================================
// Test 11: ResourceGuard
// ============================================================================

void testResourceGuard() {
    std::cout << "\n[Test 11] ResourceGuard\n";
    std::cout << "==========================================\n";

    int resourceValue = 100;
    int deleteCalled = 0;

    // Test automatic resource cleanup
    {
        auto guard = makeResourceGuard(&resourceValue, [&deleteCalled](int* p) {
            deleteCalled++;
            (void)p; // Use pointer to simulate cleanup
        });

        assert(guard.get() == &resourceValue && "Should store resource");
        assert(deleteCalled == 0 && "Deleter not called yet");

    } // guard destroyed

    assert(deleteCalled == 1 && "Deleter should be called");

    // Test release (give up ownership)
    deleteCalled = 0;
    {
        auto guard = makeResourceGuard(&resourceValue, [&deleteCalled](int* p) {
            deleteCalled++;
            (void)p;
        });

        int* released = guard.release();
        (void)released;  // Used in assert
        assert(released == &resourceValue && "Should return resource");

    } // guard destroyed but ownership released

    assert(deleteCalled == 0 && "Deleter should not be called after release");

    // Test reset
    int anotherValue = 200;
    deleteCalled = 0;
    {
        auto guard = makeResourceGuard(&resourceValue, [&deleteCalled](int* p) {
            deleteCalled++;
            (void)p;
        });

        guard.reset(&anotherValue);
        assert(deleteCalled == 1 && "Should delete old resource");
        assert(guard.get() == &anotherValue && "Should have new resource");

    }

    assert(deleteCalled == 2 && "Should delete new resource too");

    std::cout << "✓ ResourceGuard automatic cleanup works\n";
    std::cout << "✓ ResourceGuard release works\n";
    std::cout << "✓ ResourceGuard reset works\n";
}

// ============================================================================
// Test 12: UniqueArrayPtr
// ============================================================================

void testUniqueArrayPtr() {
    std::cout << "\n[Test 12] UniqueArrayPtr\n";
    std::cout << "==========================================\n";

    auto& memMgr = MemoryManager::getInstance();
    memMgr.resetStats();
    memMgr.enableTracking();

    TestObject::resetCounters();

    {
        // Create array of 10 TestObjects
        auto arr = makeUniqueArray<TestObject>(10);

        assert(arr.size() == 10 && "Should have 10 elements");
        assert(arr.get() != nullptr && "Should have pointer");
        assert(static_cast<bool>(arr) && "Boolean conversion should work");

        // Check all elements are default-constructed
        assert(TestObject::getConstructorCalls() == 10 && "Should construct 10 objects");

        // Test element access
        arr[0].setValue(0);
        arr[1].setValue(1);
        arr[9].setValue(9);

        assert(arr[0].getValue() == 0 && "Element 0 should have value 0");
        assert(arr[1].getValue() == 1 && "Element 1 should have value 1");
        assert(arr[9].getValue() == 9 && "Element 9 should have value 9");

        // Check memory tracking
        [[maybe_unused]] auto stats = memMgr.getStats();
        assert(stats.activeAllocations == 1 && "Should track 1 array allocation");

    } // arr destroyed

    // Check all destructors called
    assert(TestObject::getDestructorCalls() == 10 && "Should destruct 10 objects");

    [[maybe_unused]] auto stats = memMgr.getStats();
    assert(stats.activeAllocations == 0 && "Should deallocate array");
    assert(!stats.hasLeaks() && "Should have no leaks");

    std::cout << "✓ makeUniqueArray works\n";
    std::cout << "✓ Array element construction/destruction works\n";
    std::cout << "✓ Array memory tracking works\n";
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  Phase 4 Unit Tests\n";
    std::cout << "  Memory Management System\n";
    std::cout << "========================================\n";

    try {
        // Memory Manager tests
        testMemoryManagerBasics();
        testMemoryLeakDetection();
        testMemoryManagerMacros();

        // Object Pool tests
        testObjectPoolBasics();
        testObjectPoolWithArgs();
        testObjectPoolMaxCapacity();
        testPooledObject();

        // Smart Pointer tests
        testUniquePtr();
        testSharedPtr();

        // RAII Helper tests
        testScopeGuard();
        testResourceGuard();
        testUniqueArrayPtr();

        std::cout << "\n========================================\n";
        std::cout << "  All Tests Passed! ✓\n";
        std::cout << "========================================\n\n";

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\n========================================\n";
        std::cerr << "  Test Failed! ✗\n";
        std::cerr << "  Error: " << e.what() << "\n";
        std::cerr << "========================================\n\n";
        return 1;
    }
}
