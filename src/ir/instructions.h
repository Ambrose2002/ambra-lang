/**
 * @file instructions.h
 * @brief IR instruction set definition
 *
 * This file defines the complete instruction set for the Ambra IR,
 * including:
 * - Opcode enumeration for all instructions
 * - Instruction structure combining opcode with operand
 * - Operand type (union of different ID types)
 * - Label structure for control flow targets
 *
 * The instruction set is stack-based, meaning:
 * - Most operations pop operands from the stack
 * - Results are pushed back onto the stack
 * - No explicit register allocation required
 *
 * Instruction categories:
 * - Stack manipulation (push, pop)
 * - Local variable access (load, store)
 * - Arithmetic operations (add, sub, mul, div)
 * - Unary operations (negation, logical not)
 * - Comparison operations (eq, lt, gt, etc.)
 * - Control flow (jumps, labels)
 * - Side effects (print)
 * - Type conversions (toString)
 * - String operations (concatenation)
 */

#include "ast/expr.h"
#include "types.h"

#include <variant>

/**
 * @brief Control flow label with metadata
 *
 * Represents a position in the instruction stream that can be jumped to.
 * Labels are emitted using JLabel instruction and referenced by Jump/JumpIfFalse.
 */
struct Label
{
    LabelId     id;        ///< Unique identifier for this label
    std::string debugName; ///< Human-readable name for debugging
};

/**
 * @brief IR instruction opcode enumeration
 *
 * Defines all operations available in the Ambra IR. Instructions are
 * organized by category for clarity. Most instructions operate on the
 * implicit operand stack.
 *
 * Stack effect notation:
 * - "... a b -- result" means pops b, pops a, pushes result
 * - "... value --" means pops value (no result)
 * - "-- value" means pushes value (no operands)
 */
enum Opcode
{
    // ==================================================================================
    // STACK OPERATIONS
    // ==================================================================================

    /**
     * @brief Push constant onto stack
     * Operand: ConstId (index into constant pool)
     * Stack effect: -- value
     */
    PushConst,

    /**
     * @brief Pop value from stack and discard
     * Operand: none
     * Stack effect: ... value --
     */
    Pop,

    // ==================================================================================
    // LOCAL VARIABLE OPERATIONS
    // ==================================================================================

    /**
     * @brief Load local variable onto stack
     * Operand: LocalId (variable slot number)
     * Stack effect: -- value
     */
    LoadLocal,

    /**
     * @brief Store top of stack into local variable
     * Operand: LocalId (variable slot number)
     * Stack effect: ... value --
     */
    StoreLocal,

    // ==================================================================================
    // ARITHMETIC OPERATIONS (32-bit integers)
    // ==================================================================================

    /**
     * @brief Add two integers
     * Operand: none
     * Stack effect: ... a b -- (a + b)
     */
    AddI32,

    /**
     * @brief Subtract two integers
     * Operand: none
     * Stack effect: ... a b -- (a - b)
     */
    SubI32,

    /**
     * @brief Multiply two integers
     * Operand: none
     * Stack effect: ... a b -- (a * b)
     */
    MulI32,

    /**
     * @brief Divide two integers
     * Operand: none
     * Stack effect: ... a b -- (a / b)
     */
    DivI32,

    // ==================================================================================
    // UNARY OPERATIONS
    // ==================================================================================

    /**
     * @brief Logical NOT for boolean
     * Operand: none
     * Stack effect: ... bool -- (!bool)
     */
    NotBool,

    /**
     * @brief Arithmetic negation for integer
     * Operand: none
     * Stack effect: ... int -- (-int)
     */
    NegI32,

    // ==================================================================================
    // COMPARISON OPERATIONS - INTEGERS
    // ==================================================================================

    /**
     * @brief Compare integers for equality
     * Operand: none
     * Stack effect: ... a b -- (a == b)
     */
    CmpEqI32,

    /**
     * @brief Compare integers for inequality
     * Operand: none
     * Stack effect: ... a b -- (a != b)
     */
    CmpNEqI32,

    /**
     * @brief Compare integers: less than
     * Operand: none
     * Stack effect: ... a b -- (a < b)
     */
    CmpLtI32,

    /**
     * @brief Compare integers: less than or equal
     * Operand: none
     * Stack effect: ... a b -- (a <= b)
     */
    CmpLtEqI32,

    /**
     * @brief Compare integers: greater than
     * Operand: none
     * Stack effect: ... a b -- (a > b)
     */
    CmpGtI32,

    /**
     * @brief Compare integers: greater than or equal
     * Operand: none
     * Stack effect: ... a b -- (a >= b)
     */
    CmpGtEqI32,

    // ==================================================================================
    // COMPARISON OPERATIONS - BOOLEANS
    // ==================================================================================

    /**
     * @brief Compare booleans for equality
     * Operand: none
     * Stack effect: ... a b -- (a == b)
     */
    CmpEqBool32,

    /**
     * @brief Compare booleans for inequality
     * Operand: none
     * Stack effect: ... a b -- (a != b)
     */
    CmpNEqBool32,

    // ==================================================================================
    // COMPARISON OPERATIONS - STRINGS
    // ==================================================================================

    /**
     * @brief Compare strings for equality
     * Operand: none
     * Stack effect: ... a b -- (a == b)
     */
    CmpEqString32,

    /**
     * @brief Compare strings for inequality
     * Operand: none
     * Stack effect: ... a b -- (a != b)
     */
    CmpNEqString32,

    // ==================================================================================
    // CONTROL FLOW OPERATIONS
    // ==================================================================================

    /**
     * @brief Unconditional jump to label
     * Operand: LabelId (target label)
     * Stack effect: none
     * Used for: loop iteration, skip else branch
     */
    Jump,

    /**
     * @brief Conditional jump if top of stack is false
     * Operand: LabelId (target label)
     * Stack effect: ... bool --
     * Used for: if conditions, loop conditions
     */
    JumpIfFalse,

    /**
     * @brief Label marker (jump target)
     * Operand: LabelId (this label's ID)
     * Stack effect: none
     * Marks a position in instruction stream for jumps
     */
    JLabel,

    // ==================================================================================
    // SIDE EFFECT OPERATIONS
    // ==================================================================================

    /**
     * @brief Print string to output
     * Operand: none
     * Stack effect: ... string --
     * Consumes string from stack and prints it
     */
    PrintString,

    // ==================================================================================
    // TYPE CONVERSION OPERATIONS
    // ==================================================================================

    /**
     * @brief Convert value to string representation
     * Operand: none
     * Stack effect: ... value -- string
     * Works for integers, booleans, and strings (identity)
     */
    ToString,

    /**
     * @brief Concatenate two strings
     * Operand: none
     * Stack effect: ... str1 str2 -- (str1 + str2)
     * Used for string interpolation
     */
    ConcatString,

    // ==================================================================================
    // STRUCTURAL OPERATIONS
    // ==================================================================================

    /**
     * @brief No operation (placeholder)
     * Operand: none
     * Stack effect: none
     * May be used for padding or future optimizations
     */
    Nop
};

/**
 * @brief Instruction operand union type
 *
 * Most instructions have an operand that specifies what they operate on.
 * The operand can be:
 * - std::monostate: No operand (instruction is fully specified by opcode)
 * - LocalId: References a local variable
 * - LabelId: References a control flow label
 * - ConstId: References a constant in the constant pool
 *
 * The variant ensures type safety - you can't accidentally use a
 * local ID where a constant ID is expected.
 */
using Operand = std::variant<std::monostate, LocalId, LabelId, ConstId>;

/**
 * @brief Single IR instruction
 *
 * Combines an opcode (what operation to perform) with an operand
 * (what to operate on). The operand meaning depends on the opcode:
 * - PushConst: operand is ConstId
 * - LoadLocal/StoreLocal: operand is LocalId
 * - Jump/JumpIfFalse/JLabel: operand is LabelId
 * - Arithmetic/comparison ops: no operand (std::monostate)
 *
 * Each instruction also tracks its source location for error reporting.
 */
struct Instruction
{
    Opcode  opcode;  ///< The operation to perform
    Operand operand; ///< The operand (if any) - interpretation depends on opcode

    SourceLoc loc; ///< Source location for error reporting and debugging
};
