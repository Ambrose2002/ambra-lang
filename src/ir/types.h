/**
 * @file types.h
 * @brief Core type definitions for the Ambra IR
 *
 * This file defines the fundamental types and identifiers used throughout
 * the Intermediate Representation (IR) layer. It includes:
 * - IR type system (IrType enum)
 * - Strongly-typed identifiers (LocalId, LabelId, ConstId)
 * - Local variable metadata (LocalInfo)
 * - Constant pool entries (Constant)
 *
 * The IR uses a simple type system with four types:
 * - I32: 32-bit signed integers
 * - Bool32: Boolean values
 * - String32: String values
 * - Void32: Unit type for expressions without meaningful value
 *
 * Identifiers are strongly typed to prevent confusion between different
 * ID spaces (locals vs labels vs constants).
 */

#include "ast/expr.h"

#include <cstdint>
#include <string>
#include <variant>

/**
 * @brief IR type system enumeration
 *
 * Represents the runtime type of values in the IR. All types are
 * represented as 32-bit values on the stack for simplicity.
 */
enum IrType
{
    I32,    ///< 32-bit signed integer
    Bool32, ///< Boolean value (true/false), stored as 32-bit

    String32,
    Void32
};

struct LocalId
{
    uint32_t value;

    bool operator==(LocalId other) const
    {
        return value == other.value;
    }
};

/**
 * @brief Strongly-typed identifier for control flow labels
 *
 * Labels mark locations in the instruction stream that can be jumped to.
 * Used for implementing conditionals, loops, and other control flow.
 * Each label gets a unique ID within a function.
 */
struct LabelId
{
    uint32_t value; ///< Unique label identifier within function

    bool operator==(LabelId other) const
    {
        return value == other.value;
    }
};

/**
 * @brief Strongly-typed identifier for constant pool entries
 *
 * Constants (literal values) are stored in a constant pool and referenced
 * by ID. This avoids duplicating literal values in the instruction stream.
 * The constant pool is shared across the entire program.
 */
struct ConstId
{
    uint32_t value; ///< Index into program's constant pool

    bool operator==(ConstId other) const
    {
        return value == other.value;
    }
};

/**
 * @brief Metadata for a local variable
 *
 * Stores information about a local variable including its type, name,
 * and source location. This information is used for debugging, error
 * reporting, and type checking during lowering.
 */
struct LocalInfo
{
    LocalId     id;        ///< Unique identifier for this local
    IrType      type;      ///< Runtime type of the variable
    std::string debugName; ///< Variable name from source code
    SourceLoc   declLoc;   ///< Source location where variable was declared
};

/**
 * @brief Constant pool entry holding a literal value
 *
 * Represents a compile-time constant that can be pushed onto the stack.
 * The value variant holds the actual literal (int, bool, or string).
 * Constants are deduplicated in the constant pool.
 */
struct Constant
{
    IrType  type;    ///< Type of the constant value
    ConstId constId; ///< Unique identifier for this constant

    /**
     * @brief The actual literal value
     *
     * Holds one of:
     * - std::string for String32 constants
     * - int for I32 constants
     * - bool for Bool32 constants
     */
    std::variant<std::string, int, bool> value;

    bool operator==(Constant other) const
    {
        return type == other.type && constId == other.constId && value == other.value;
    }
};

// Hash function specializations for using IDs in unordered containers
namespace std
{
template <> struct hash<LabelId>
{
    size_t operator()(const LabelId& id) const noexcept
    {
        return hash<uint32_t>()(id.value);
    }
};

template <> struct hash<LocalId>
{
    size_t operator()(const LocalId& id) const noexcept
    {
        return hash<uint32_t>()(id.value);
    }
};

template <> struct hash<ConstId>
{
    size_t operator()(const ConstId& id) const noexcept
    {
        return hash<uint32_t>()(id.value);
    }
};
} // namespace std
