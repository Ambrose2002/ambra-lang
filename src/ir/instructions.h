#include "ast/expr.h"
#include "types.h"

#include <variant>

struct Label
{
    LabelId     id;
    std::string debugName;
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

using Operand = std::variant<std::monostate, LocalId, LabelId, ConstId>;

struct Instruction
{
    Opcode  opcode;
    Operand operand;

    SourceLoc loc;
};