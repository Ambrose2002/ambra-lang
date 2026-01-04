#include "lowering.h"

#include <unordered_map>
#include <vector>

static const char* getOpcodeName(Opcode op)
{
    switch (op)
    {
    case PushConst:
        return "PushConst";
    case Pop:
        return "Pop";
    case LoadLocal:
        return "LoadLocal";
    case StoreLocal:
        return "StoreLocal";
    case AddI32:
        return "AddI32";
    case SubI32:
        return "SubI32";
    case MulI32:
        return "MulI32";
    case DivI32:
        return "DivI32";
    case NotBool:
        return "NotBool";
    case NegI32:
        return "NegI32";
    case CmpEqI32:
        return "CmpEqI32";
    case CmpNEqI32:
        return "CmpNEqI32";
    case CmpLtI32:
        return "CmpLtI32";
    case CmpLtEqI32:
        return "CmpLtEqI32";
    case CmpGtI32:
        return "CmpGtI32";
    case CmpGtEqI32:
        return "CmpGtEqI32";
    case CmpEqBool32:
        return "CmpEqBool32";
    case CmpNEqBool32:
        return "CmpNEqBool32";
    case CmpEqString32:
        return "CmpEqString32";
    case CmpNEqString32:
        return "CmpNEqString32";
    case Jump:
        return "Jump";
    case JumpIfFalse:
        return "JumpIfFalse";
    case JLabel:
        return "JLabel";
    case PrintString:
        return "PrintString";
    case ToString:
        return "ToString";
    case ConcatString:
        return "ConcatString";
    case Nop:
        return "Nop";
    default:
        return "Unknown";
    }
}

static const char* getTypeName(IrType t)
{
    switch (t)
    {
    case I32:
        return "I32";
    case Bool32:
        return "Bool32";
    case Void32:
        return "Void32";
    case String32:
        return "String32";
    default:
        return "Unknown";
    }
}
struct IrDiagnostic
{
    std::string message;
    size_t      ip;
    // SourceLoc   loc;
};

struct IrValidatorResults
{
    std::vector<IrDiagnostic> diagnostics;

    bool hadError() const
    {
        return diagnostics.size() > 0;
    }
};

struct IrValidator
{
    const IrProgram&  program;
    const IrFunction& function;

    std::vector<IrDiagnostic> diagnostics;

    IrValidatorResults validate()
    {
        diagnostics.clear();
        validateFunction();

        return IrValidatorResults{diagnostics};
    };

  private:
    void validateFunction()
    {
        std::vector<IrType> stack;

        std::unordered_map<LabelId, std::vector<IrType>> labelStacks;

        const auto& instructions = function.instructions;

        for (size_t ip = 0; ip < instructions.size(); ++ip)
        {
            const Instruction& inst = instructions[ip];

            // Validate instruction + update stack
            validateInstruction(ip, stack);

            // Handle control-flow edges
            if (inst.opcode == Jump || inst.opcode == JumpIfFalse)
            {
                LabelId target = std::get<LabelId>(inst.operand);

                auto it = labelStacks.find(target);
                if (it == labelStacks.end())
                {
                    labelStacks[target] = stack;
                }
                else if (it->second != stack)
                {
                    diagnostics.push_back({"Stack mismatch at jump target", ip});
                }
            }

            // Handle label definition
            if (inst.opcode == JLabel)
            {
                LabelId id = std::get<LabelId>(inst.operand);

                auto it = labelStacks.find(id);
                if (it == labelStacks.end())
                {
                    labelStacks[id] = stack;
                }
                else if (it->second != stack)
                {
                    diagnostics.push_back({"Stack mismatch at label", ip});
                }
            }
        }
    }

    void validateInstruction(size_t ip, std::vector<IrType> stack)
    {
        switch (function.instructions[ip].opcode)
        {
        case AddI32:
        case SubI32:
        case MulI32:
        case DivI32:
        {
            if (stack.size() < 2)
            {
                diagnostics.push_back({"Stack underflow", ip});
                return;
            }
            if (stack.back() != I32 && stack[stack.size() - 2])
            {
                diagnostics.push_back(
                    {std::string("type mismatch: expected I32, got") + getTypeName(stack.back()),
                     ip});
            }
        }
        default:
            diagnostics.push_back({"Unexpected opcode", ip});
        }
    }
    void validateStack();
    void validateControlFlow();
};