#include "ir/validator.h"

#include <cstddef>
#include <variant>

struct Value
{
    IrType                               tag;
    std::variant<bool, int, std::string> payload;
};

struct VM
{
    // Execution state
    size_t ip = 0;

    // Operand stack
    std::vector<Value> stack;

    // Locals (indexed by LocalId)
    std::vector<Value> locals;

    // IR references (no ownership)
    const IrProgram&  program;
    const IrFunction& function;

    // Constructor initializes locals from function metadata
    VM(const IrProgram& prog, const IrFunction& func) : program(prog), function(func)
    {
        locals.resize(function.localTable.locals.size());
    }

    void execute();
};