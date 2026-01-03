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
    LabelId     id;
    std::string debugName;
    SourceLoc   loc;
};

enum Opcode
{
    // Stack
    PushConst,
    Pop,

    // Locals
    LoadLocal,
    StoreLocal,

    // Arithmetic
    AddI32,
    SubI32,
    MulI32,
    DivI32,

    // Unary
    NotBool,
    NegI32,

    // Comparison for ints
    CmpEqI32,
    CmpNEqI32,
    CmpLtI32,
    CmpLtEqI32,
    CmpGtI32,
    CmpGtEqI32,

    // Comparison for bool
    CmpEqBool32,
    CmpNEqBool32,

    // Comparison for string
    CmpEqString32,
    CmpNEqString32,

    // Control flow
    Jump,
    JumpIfFalse,
    JLabel,

    // Side effects
    PrintString,

    ToString,
    ConcatString,

    // Structural
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
    Opcode  opcode;
    Operand operand;

    SourceLoc loc;
};