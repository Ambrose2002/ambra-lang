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

    bool maintainStack(size_t popCount, size_t pushCount, IrType popType, IrType pushType,
                       std::vector<IrType>& stack, size_t ip)
    {
        if (stack.size() < popCount)
        {
            diagnostics.push_back({"Stack underflow", ip});
            return false;
        }

        for (size_t i = 0; i < popCount; i++)
        {
            if (stack.back() != popType)
            {
                diagnostics.push_back({std::string("type mismatch: expected") +
                                           getTypeName(popType) + (", got") +
                                           getTypeName(stack.back()),
                                       ip});
                return false;
            }
            stack.pop_back();
        }

        for (size_t i = 0; i < pushCount; i++)
        {
            stack.push_back(pushType);
        }

        return true;
    }

    void validateInstruction(size_t ip, std::vector<IrType>& stack)
    {
        switch (function.instructions[ip].opcode)
        {
        case AddI32:
        case SubI32:
        case MulI32:
        case DivI32:
        {
            bool res = maintainStack(2, 1, I32, I32, stack, ip);
            if (!res)
                return;
            break;
        }
        case NotBool:
        {
            bool res = maintainStack(1, 1, Bool32, Bool32, stack, ip);
            if (!res)
                return;
            break;
        }
        case NegI32:
        {
            bool res = maintainStack(1, 1, I32, I32, stack, ip);
            if (!res)
                return;
            break;
        }
        case CmpEqI32:
        case CmpNEqI32:
        case CmpLtI32:
        case CmpLtEqI32:
        case CmpGtI32:
        case CmpGtEqI32:
        {
            bool res = maintainStack(2, 1, I32, Bool32, stack, ip);
            if (!res)
                return;
            break;
        }
        case CmpEqBool32:
        case CmpNEqBool32:
        {
            bool res = maintainStack(2, 1, Bool32, Bool32, stack, ip);
            if (!res)
                return;
            break;
        }
        case CmpEqString32:
        case CmpNEqString32:
        {
            bool res = maintainStack(2, 1, String32, Bool32, stack, ip);
            if (!res)
                return;
            break;
        }
        case LoadLocal:
        {
            LocalId lid = std::get<LocalId>(function.instructions[ip].operand);

            if (lid.value >= function.localTable.locals.size())
            {
                diagnostics.push_back({"Invalid LocalId", ip});
                return;
            }

            stack.push_back(function.localTable.locals[lid.value].type);
            break;
        }
        case StoreLocal:
        {
            LocalId lid = std::get<LocalId>(function.instructions[ip].operand);

            if (lid.value >= function.localTable.locals.size())
            {
                diagnostics.push_back({"Invalid LocalId", ip});
                return;
            }

            if (stack.empty())
            {
                diagnostics.push_back({"Stack underflow", ip});
                return;
            }

            IrType expected = function.localTable.locals[lid.value].type;
            IrType actual = stack.back();

            if (actual != expected)
            {
                diagnostics.push_back({std::string("type mismatch: expected ") +
                                           getTypeName(expected) + ", got " + getTypeName(actual),
                                       ip});
                return;
            }

            stack.pop_back();
            break;
        }
        case PushConst:
        {
            ConstId cid = std::get<ConstId>(function.instructions[ip].operand);

            if (cid.value >= program.constants.size())
            {
                diagnostics.push_back({"Invalid ConstId", ip});
                return;
            }

            stack.push_back(program.constants[cid.value].type);
            break;
        }
        case ToString:
        {
            if (!stack.size())
            {
                diagnostics.push_back({"Stack underflow", ip});
                return;
            }

            if (stack.back() == Void32)
            {
                diagnostics.push_back({std::string("type mismatch: expected ") +
                                           getTypeName(String32) + " or " + getTypeName(I32) +
                                           " or " + getTypeName(Bool32) + (", got") +
                                           getTypeName(stack.back()),
                                       ip});
                return;
            }
            stack.pop_back();
            stack.push_back(String32);
            break;
        }
        case ConcatString:
        {
            bool res = maintainStack(2, 1, String32, String32, stack, ip);
            if (!res)
                return;
            break;
        }
        case PrintString:
        {
            bool res = maintainStack(1, 0, String32, Void32, stack, ip);
            if (!res)
                return;
            break;
        }
        case JumpIfFalse:
        {
            bool res = maintainStack(1, 0, Bool32, Void32, stack, ip);
            if (!res)
                return;
            break;
        }
        case Jump:
        case JLabel:
        case Nop:
            break;
        default:
            diagnostics.push_back({"Unexpected opcode", ip});
        }
    }
    void validateStack();
    void validateControlFlow();
};