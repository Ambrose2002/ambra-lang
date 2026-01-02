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

    // Comparison
    CmpEqI32,
    CmpNEqI32,
    CmpLtI32,
    CmpLtEqI32,
    CmpGtI32,
    CmpGtEqI32,

    // Control flow
    Jump,
    JumpIfFalse,

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