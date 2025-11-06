/**
 * @file PhysicalQuantity.h
 * @brief Physical quantities with units
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-alpha3
 * @date 2025-11-06
 *
 * This file defines a type-safe system for handling physical quantities
 * with units. It helps prevent unit conversion errors and makes code
 * more readable and maintainable.
 */

#ifndef KOO_CORE_TYPES_PHYSICAL_QUANTITY_H
#define KOO_CORE_TYPES_PHYSICAL_QUANTITY_H

#include <string>
#include <cmath>
#include <stdexcept>

namespace koo {
namespace core {
namespace types {

// ============================================================================
// SI Base Units
// ============================================================================

/**
 * @brief Enumeration of SI base units
 */
enum class BaseUnit {
    METER,      ///< Length (m)
    KILOGRAM,   ///< Mass (kg)
    SECOND,     ///< Time (s)
    AMPERE,     ///< Electric current (A)
    KELVIN,     ///< Temperature (K)
    MOLE,       ///< Amount of substance (mol)
    CANDELA,    ///< Luminous intensity (cd)
    DIMENSIONLESS  ///< Dimensionless quantity
};

/**
 * @brief Unit exponents for dimensional analysis
 *
 * Represents a physical unit as a combination of SI base units.
 * For example, velocity [m/s] is represented as:
 *   length=1, time=-1, others=0
 */
struct UnitDimension {
    int length{0};      ///< Meter exponent
    int mass{0};        ///< Kilogram exponent
    int time{0};        ///< Second exponent
    int current{0};     ///< Ampere exponent
    int temperature{0}; ///< Kelvin exponent
    int amount{0};      ///< Mole exponent
    int luminosity{0};  ///< Candela exponent

    /**
     * @brief Check if dimensions match
     */
    bool operator==(const UnitDimension& other) const {
        return length == other.length &&
               mass == other.mass &&
               time == other.time &&
               current == other.current &&
               temperature == other.temperature &&
               amount == other.amount &&
               luminosity == other.luminosity;
    }

    /**
     * @brief Check if dimensions differ
     */
    bool operator!=(const UnitDimension& other) const {
        return !(*this == other);
    }

    /**
     * @brief Multiply dimensions (for multiplication of quantities)
     */
    UnitDimension operator*(const UnitDimension& other) const {
        return UnitDimension{
            length + other.length,
            mass + other.mass,
            time + other.time,
            current + other.current,
            temperature + other.temperature,
            amount + other.amount,
            luminosity + other.luminosity
        };
    }

    /**
     * @brief Divide dimensions (for division of quantities)
     */
    UnitDimension operator/(const UnitDimension& other) const {
        return UnitDimension{
            length - other.length,
            mass - other.mass,
            time - other.time,
            current - other.current,
            temperature - other.temperature,
            amount - other.amount,
            luminosity - other.luminosity
        };
    }

    /**
     * @brief Check if dimensionless
     */
    bool isDimensionless() const {
        return length == 0 && mass == 0 && time == 0 &&
               current == 0 && temperature == 0 &&
               amount == 0 && luminosity == 0;
    }
};

// ============================================================================
// Physical Quantity Class
// ============================================================================

/**
 * @brief Physical quantity with units
 *
 * This class represents a physical quantity (value + unit). It provides
 * type-safe operations that enforce dimensional consistency.
 *
 * Example usage:
 * @code
 * PhysicalQuantity length(5.0, Units::METER);
 * PhysicalQuantity time(2.0, Units::SECOND);
 * PhysicalQuantity velocity = length / time;  // 2.5 m/s
 * @endcode
 */
class PhysicalQuantity {
public:
    /**
     * @brief Default constructor - creates dimensionless zero
     */
    PhysicalQuantity()
        : value_(0.0), dimension_(), name_("dimensionless") {}

    /**
     * @brief Constructor with value and dimension
     */
    PhysicalQuantity(double value, const UnitDimension& dimension,
                     const std::string& name = "")
        : value_(value), dimension_(dimension), name_(name) {}

    /**
     * @brief Constructor for dimensionless quantity
     */
    explicit PhysicalQuantity(double value)
        : value_(value), dimension_(), name_("dimensionless") {}

    /**
     * @brief Get the numerical value
     */
    double value() const { return value_; }

    /**
     * @brief Get the unit dimension
     */
    const UnitDimension& dimension() const { return dimension_; }

    /**
     * @brief Get the unit name
     */
    const std::string& name() const { return name_; }

    /**
     * @brief Set the unit name
     */
    void setName(const std::string& name) { name_ = name; }

    /**
     * @brief Check if dimensionless
     */
    bool isDimensionless() const { return dimension_.isDimensionless(); }

    // ========================================================================
    // Arithmetic Operations
    // ========================================================================

    /**
     * @brief Addition (requires same dimensions)
     */
    PhysicalQuantity operator+(const PhysicalQuantity& other) const {
        if (dimension_ != other.dimension_) {
            throw std::invalid_argument(
                "Cannot add quantities with different dimensions");
        }
        return PhysicalQuantity(value_ + other.value_, dimension_, name_);
    }

    /**
     * @brief Subtraction (requires same dimensions)
     */
    PhysicalQuantity operator-(const PhysicalQuantity& other) const {
        if (dimension_ != other.dimension_) {
            throw std::invalid_argument(
                "Cannot subtract quantities with different dimensions");
        }
        return PhysicalQuantity(value_ - other.value_, dimension_, name_);
    }

    /**
     * @brief Multiplication
     */
    PhysicalQuantity operator*(const PhysicalQuantity& other) const {
        return PhysicalQuantity(
            value_ * other.value_,
            dimension_ * other.dimension_,
            name_ + "*" + other.name_
        );
    }

    /**
     * @brief Division
     */
    PhysicalQuantity operator/(const PhysicalQuantity& other) const {
        if (std::abs(other.value_) < 1e-15) {
            throw std::invalid_argument("Division by zero");
        }
        return PhysicalQuantity(
            value_ / other.value_,
            dimension_ / other.dimension_,
            name_ + "/" + other.name_
        );
    }

    /**
     * @brief Scalar multiplication
     */
    PhysicalQuantity operator*(double scalar) const {
        return PhysicalQuantity(value_ * scalar, dimension_, name_);
    }

    /**
     * @brief Scalar division
     */
    PhysicalQuantity operator/(double scalar) const {
        if (std::abs(scalar) < 1e-15) {
            throw std::invalid_argument("Division by zero");
        }
        return PhysicalQuantity(value_ / scalar, dimension_, name_);
    }

    /**
     * @brief Unary minus
     */
    PhysicalQuantity operator-() const {
        return PhysicalQuantity(-value_, dimension_, name_);
    }

    // ========================================================================
    // Comparison Operations (only for same dimensions)
    // ========================================================================

    /**
     * @brief Equality comparison
     */
    bool operator==(const PhysicalQuantity& other) const {
        if (dimension_ != other.dimension_) {
            return false;
        }
        return std::abs(value_ - other.value_) < 1e-10;
    }

    /**
     * @brief Inequality comparison
     */
    bool operator!=(const PhysicalQuantity& other) const {
        return !(*this == other);
    }

    /**
     * @brief Less than comparison
     */
    bool operator<(const PhysicalQuantity& other) const {
        if (dimension_ != other.dimension_) {
            throw std::invalid_argument(
                "Cannot compare quantities with different dimensions");
        }
        return value_ < other.value_;
    }

    /**
     * @brief Less than or equal comparison
     */
    bool operator<=(const PhysicalQuantity& other) const {
        return *this < other || *this == other;
    }

    /**
     * @brief Greater than comparison
     */
    bool operator>(const PhysicalQuantity& other) const {
        return !(*this <= other);
    }

    /**
     * @brief Greater than or equal comparison
     */
    bool operator>=(const PhysicalQuantity& other) const {
        return !(*this < other);
    }

    // ========================================================================
    // Mathematical Functions
    // ========================================================================

    /**
     * @brief Absolute value
     */
    PhysicalQuantity abs() const {
        return PhysicalQuantity(std::abs(value_), dimension_, name_);
    }

    /**
     * @brief Power function (for dimensionless only)
     */
    PhysicalQuantity pow(double exponent) const {
        if (!isDimensionless()) {
            throw std::invalid_argument(
                "Power function only works for dimensionless quantities");
        }
        return PhysicalQuantity(std::pow(value_, exponent));
    }

    /**
     * @brief Square root (for dimensionless only)
     */
    PhysicalQuantity sqrt() const {
        if (!isDimensionless()) {
            throw std::invalid_argument(
                "Square root only works for dimensionless quantities");
        }
        if (value_ < 0) {
            throw std::invalid_argument("Square root of negative number");
        }
        return PhysicalQuantity(std::sqrt(value_));
    }

private:
    double value_;              ///< Numerical value
    UnitDimension dimension_;   ///< Unit dimension
    std::string name_;          ///< Unit name (for display)
};

/**
 * @brief Scalar multiplication (scalar * quantity)
 */
inline PhysicalQuantity operator*(double scalar, const PhysicalQuantity& qty) {
    return qty * scalar;
}

// ============================================================================
// Common Units (Factory Functions)
// ============================================================================

namespace Units {

// Length units
inline PhysicalQuantity Meter(double value) {
    return PhysicalQuantity(value, UnitDimension{1, 0, 0, 0, 0, 0, 0}, "m");
}

inline PhysicalQuantity Centimeter(double value) {
    return PhysicalQuantity(value * 0.01, UnitDimension{1, 0, 0, 0, 0, 0, 0}, "cm");
}

inline PhysicalQuantity Millimeter(double value) {
    return PhysicalQuantity(value * 0.001, UnitDimension{1, 0, 0, 0, 0, 0, 0}, "mm");
}

// Time units
inline PhysicalQuantity Second(double value) {
    return PhysicalQuantity(value, UnitDimension{0, 0, 1, 0, 0, 0, 0}, "s");
}

inline PhysicalQuantity Minute(double value) {
    return PhysicalQuantity(value * 60.0, UnitDimension{0, 0, 1, 0, 0, 0, 0}, "min");
}

inline PhysicalQuantity Hour(double value) {
    return PhysicalQuantity(value * 3600.0, UnitDimension{0, 0, 1, 0, 0, 0, 0}, "h");
}

// Mass units
inline PhysicalQuantity Kilogram(double value) {
    return PhysicalQuantity(value, UnitDimension{0, 1, 0, 0, 0, 0, 0}, "kg");
}

inline PhysicalQuantity Gram(double value) {
    return PhysicalQuantity(value * 0.001, UnitDimension{0, 1, 0, 0, 0, 0, 0}, "g");
}

// Temperature units
inline PhysicalQuantity Kelvin(double value) {
    return PhysicalQuantity(value, UnitDimension{0, 0, 0, 0, 1, 0, 0}, "K");
}

inline PhysicalQuantity Celsius(double value) {
    return PhysicalQuantity(value + 273.15, UnitDimension{0, 0, 0, 0, 1, 0, 0}, "°C");
}

// Amount of substance
inline PhysicalQuantity Mole(double value) {
    return PhysicalQuantity(value, UnitDimension{0, 0, 0, 0, 0, 1, 0}, "mol");
}

// Derived units
inline PhysicalQuantity Joule(double value) {
    // J = kg⋅m²/s²
    return PhysicalQuantity(value, UnitDimension{2, 1, -2, 0, 0, 0, 0}, "J");
}

inline PhysicalQuantity Pascal(double value) {
    // Pa = kg/(m⋅s²)
    return PhysicalQuantity(value, UnitDimension{-1, 1, -2, 0, 0, 0, 0}, "Pa");
}

// Dimensionless
inline PhysicalQuantity Dimensionless(double value) {
    return PhysicalQuantity(value);
}

} // namespace Units

} // namespace types
} // namespace core
} // namespace koo

#endif // KOO_CORE_TYPES_PHYSICAL_QUANTITY_H
