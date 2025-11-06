/**
 * @file BaseObject.h
 * @brief Base class for all framework objects
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-alpha2
 * @date 2025-11-06
 *
 * This file defines the BaseObject class, which serves as the foundation
 * for all objects in the KooChemicalSimulation framework.
 */

#ifndef KOO_CORE_BASE_BASEOBJECT_H
#define KOO_CORE_BASE_BASEOBJECT_H

#include <memory>
#include <string>
#include <chrono>
#include <atomic>

namespace koo {
namespace core {

/**
 * @brief Base class for all framework objects
 *
 * BaseObject provides common functionality that all framework objects need:
 * - Unique object identification
 * - Name/description management
 * - Creation timestamp tracking
 * - Reference counting support
 * - Type information (RTTI alternative)
 *
 * This class implements common design patterns:
 * - Template Method Pattern (for initialization)
 * - Observer Pattern support (via callbacks)
 * - RAII (Resource Acquisition Is Initialization)
 *
 * Design Principles:
 * - Single Responsibility: Only handles object metadata and lifecycle
 * - Open/Closed: Extensible via inheritance, closed for modification
 *
 * @note This class is abstract and cannot be instantiated directly
 *
 * Example usage:
 * @code
 * class MyClass : public BaseObject {
 * public:
 *     MyClass(const std::string& name)
 *         : BaseObject(name, "MyClass") {}
 *
 *     std::string getTypeName() const override {
 *         return "MyClass";
 *     }
 * };
 * @endcode
 */
class BaseObject {
public:
    /**
     * @brief Virtual destructor
     *
     * Ensures proper cleanup of derived classes.
     */
    virtual ~BaseObject() = default;

    /**
     * @brief Get the unique object ID
     *
     * Each object is assigned a unique ID upon construction.
     *
     * @return Unique object identifier
     */
    size_t getObjectId() const {
        return objectId_;
    }

    /**
     * @brief Get the object name
     *
     * @return Object name
     */
    std::string getName() const {
        return name_;
    }

    /**
     * @brief Set the object name
     *
     * @param name New name for the object
     */
    void setName(const std::string& name) {
        name_ = name;
    }

    /**
     * @brief Get the object description
     *
     * @return Object description
     */
    std::string getDescription() const {
        return description_;
    }

    /**
     * @brief Set the object description
     *
     * @param description New description
     */
    void setDescription(const std::string& description) {
        description_ = description;
    }

    /**
     * @brief Get the type name of this object
     *
     * Returns the concrete type name of the object. This is a lightweight
     * alternative to RTTI (typeid).
     *
     * @return Type name as string
     */
    virtual std::string getTypeName() const = 0;

    /**
     * @brief Get creation timestamp
     *
     * Returns the time when this object was created.
     *
     * @return Creation time point
     */
    std::chrono::system_clock::time_point getCreationTime() const {
        return creationTime_;
    }

    /**
     * @brief Get age of the object in seconds
     *
     * @return Seconds since object creation
     */
    double getAgeInSeconds() const {
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            now - creationTime_);
        return static_cast<double>(duration.count());
    }

    /**
     * @brief Check if object is enabled
     *
     * Disabled objects can be skipped in processing loops.
     *
     * @return true if object is enabled
     */
    bool isEnabled() const {
        return enabled_;
    }

    /**
     * @brief Enable the object
     */
    void enable() {
        enabled_ = true;
    }

    /**
     * @brief Disable the object
     */
    void disable() {
        enabled_ = false;
    }

    /**
     * @brief Get a string representation of the object
     *
     * Generates a formatted string with object information.
     * Derived classes can override this for custom formatting.
     *
     * @return String representation
     */
    virtual std::string toString() const {
        std::string str;
        str += "Type: " + getTypeName() + "\n";
        str += "Name: " + name_ + "\n";
        str += "ID: " + std::to_string(objectId_) + "\n";
        str += "Description: " + description_ + "\n";
        str += "Enabled: " + std::string(enabled_ ? "yes" : "no") + "\n";
        return str;
    }

    /**
     * @brief Clone the object (deep copy)
     *
     * Creates a deep copy of the object. Derived classes must implement
     * this to support cloning.
     *
     * @return Shared pointer to cloned object
     */
    virtual std::shared_ptr<BaseObject> clone() const = 0;

    /**
     * @brief Check if two objects are equal
     *
     * Default implementation compares object IDs. Derived classes can
     * override for custom equality semantics.
     *
     * @param other Object to compare with
     * @return true if objects are equal
     */
    virtual bool equals(const BaseObject& other) const {
        return objectId_ == other.objectId_;
    }

    /**
     * @brief Equality operator
     *
     * @param other Object to compare with
     * @return true if objects are equal
     */
    bool operator==(const BaseObject& other) const {
        return equals(other);
    }

    /**
     * @brief Inequality operator
     *
     * @param other Object to compare with
     * @return true if objects are not equal
     */
    bool operator!=(const BaseObject& other) const {
        return !equals(other);
    }

    /**
     * @brief Get total number of objects created
     *
     * Returns the global count of BaseObject instances created
     * (including destroyed ones).
     *
     * @return Total object count
     */
    static size_t getTotalObjectCount() {
        return objectCounter_.load();
    }

protected:
    /**
     * @brief Protected constructor
     *
     * Objects can only be created through derived classes.
     *
     * @param name Object name
     * @param description Object description (optional)
     */
    explicit BaseObject(const std::string& name = "Unnamed",
                       const std::string& description = "")
        : objectId_(generateObjectId())
        , name_(name)
        , description_(description)
        , creationTime_(std::chrono::system_clock::now())
        , enabled_(true) {
    }

    /**
     * @brief Protected copy constructor
     *
     * Copies name and description, but generates a new unique ID.
     *
     * @param other Object to copy from
     */
    BaseObject(const BaseObject& other)
        : objectId_(generateObjectId())  // New ID for copy
        , name_(other.name_ + "_copy")
        , description_(other.description_)
        , creationTime_(std::chrono::system_clock::now())
        , enabled_(other.enabled_) {
    }

    /**
     * @brief Protected copy assignment
     *
     * Copies name and description, preserves object ID.
     *
     * @param other Object to copy from
     * @return Reference to this object
     */
    BaseObject& operator=(const BaseObject& other) {
        if (this != &other) {
            // Keep objectId_ unchanged
            name_ = other.name_;
            description_ = other.description_;
            enabled_ = other.enabled_;
            // Keep creationTime_ unchanged
        }
        return *this;
    }

    /**
     * @brief Protected move constructor
     *
     * @param other Object to move from
     */
    BaseObject(BaseObject&& other) noexcept
        : objectId_(other.objectId_)
        , name_(std::move(other.name_))
        , description_(std::move(other.description_))
        , creationTime_(other.creationTime_)
        , enabled_(other.enabled_) {
    }

    /**
     * @brief Protected move assignment
     *
     * @param other Object to move from
     * @return Reference to this object
     */
    BaseObject& operator=(BaseObject&& other) noexcept {
        if (this != &other) {
            objectId_ = other.objectId_;
            name_ = std::move(other.name_);
            description_ = std::move(other.description_);
            creationTime_ = other.creationTime_;
            enabled_ = other.enabled_;
        }
        return *this;
    }

private:
    /**
     * @brief Generate a unique object ID
     *
     * Thread-safe generation of unique IDs using atomic counter.
     *
     * @return Unique ID
     */
    static size_t generateObjectId() {
        return objectCounter_.fetch_add(1, std::memory_order_relaxed);
    }

    // Member variables
    size_t objectId_;                                    ///< Unique object ID
    std::string name_;                                   ///< Object name
    std::string description_;                            ///< Object description
    std::chrono::system_clock::time_point creationTime_; ///< Creation timestamp
    bool enabled_;                                       ///< Enable/disable flag

    // Static members
    static std::atomic<size_t> objectCounter_;           ///< Global object counter
};

// Initialize static member
inline std::atomic<size_t> BaseObject::objectCounter_{0};

/**
 * @brief Shared pointer type for BaseObject
 */
using BaseObjectPtr = std::shared_ptr<BaseObject>;

/**
 * @brief Unique pointer type for BaseObject
 */
using BaseObjectUniquePtr = std::unique_ptr<BaseObject>;

} // namespace core
} // namespace koo

#endif // KOO_CORE_BASE_BASEOBJECT_H
