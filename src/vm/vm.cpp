#include "vm.h"

#include <iostream>

void VM::execute()
{
    IrValidator        validator{program, function};
    IrValidatorResults validationResults = validator.validate();
    if (validationResults.hadError())
    {
        return;
    }

    ip = 0;

    while (ip < function.instructions.size())
    {
        const Instruction& instr = function.instructions[ip];
        const Operand&     op = instr.operand;

        switch (instr.opcode)
        {
        case PushConst:
        {
            ConstId cId = std::get<ConstId>(op);

            Constant c = program.constants[cId.value];

            Value v;
            v.tag = c.type;
            v.payload = c.value;

            stack.push_back(std::move(v));
            break;
        }
        case Pop:
        {
            stack.pop_back();
            break;
        }
        case LoadLocal:
        {
            LocalId lId = std::get<LocalId>(op);
            stack.push_back(locals[lId.value]);
            break;
        }
        case StoreLocal:
        {
            LocalId lId = std::get<LocalId>(op);
            Value   v = stack.back();
            stack.pop_back();

            locals[lId.value] = std::move(v);
            break;
        }
        // Binary operators
        case AddI32:
        case MulI32:
        case DivI32:
        case SubI32:
        case CmpEqI32:
        case CmpNEqI32:
        case CmpLtI32:
        case CmpLtEqI32:
        case CmpGtI32:
        case CmpGtEqI32:
        case CmpEqBool32:
        case CmpNEqBool32:
        case CmpEqString32:
        case CmpNEqString32:
        case ConcatString:
        {
            Value rhs = stack.back();
            stack.pop_back();
            Value lhs = stack.back();
            stack.pop_back();

            Value out;
            switch (instr.opcode)
            {
            case AddI32:
                out.tag = I32;
                out.payload = std::get<int>(lhs.payload) + std::get<int>(rhs.payload);
                break;
            case MulI32:
                out.tag = I32;
                out.payload = std::get<int>(lhs.payload) * std::get<int>(rhs.payload);
                break;
            case SubI32:
                out.tag = I32;
                out.payload = std::get<int>(lhs.payload) - std::get<int>(rhs.payload);
                break;
            case DivI32:
            {
                out.tag = I32;
                int divisor = std::get<int>(rhs.payload);
                if (divisor == 0)
                {
                    std::cout << "RuntimeError: division by zero";
                    return;
                }
                out.payload = std::get<int>(lhs.payload) / divisor;
                break;
            }
            case CmpEqI32:
            {
                out.tag = Bool32;
                out.payload = std::get<int>(lhs.payload) == std::get<int>(rhs.payload);
                break;
            }
            case CmpNEqI32:
            {
                out.tag = Bool32;
                out.payload = std::get<int>(lhs.payload) != std::get<int>(rhs.payload);
                break;
            }
            case CmpLtI32:
            {
                out.tag = Bool32;
                out.payload = std::get<int>(lhs.payload) < std::get<int>(rhs.payload);
                break;
            }
            case CmpLtEqI32:
            {
                out.tag = Bool32;
                out.payload = std::get<int>(lhs.payload) <= std::get<int>(rhs.payload);
                break;
            }
            case CmpGtI32:
            {
                out.tag = Bool32;
                out.payload = std::get<int>(lhs.payload) > std::get<int>(rhs.payload);
                break;
            }
            case CmpGtEqI32:
            {
                out.tag = Bool32;
                out.payload = std::get<int>(lhs.payload) >= std::get<int>(rhs.payload);
                break;
            }
            case CmpEqBool32:
            {
                out.tag = Bool32;
                out.payload = std::get<bool>(lhs.payload) == std::get<bool>(rhs.payload);
                break;
            }
            case CmpNEqBool32:
            {
                out.tag = Bool32;
                out.payload = std::get<bool>(lhs.payload) != std::get<bool>(rhs.payload);
                break;
            }
            case CmpEqString32:
            {
                out.tag = Bool32;
                out.payload =
                    std::get<std::string>(lhs.payload) == std::get<std::string>(rhs.payload);
                break;
            }
            case CmpNEqString32:
            {
                out.tag = Bool32;
                out.payload =
                    std::get<std::string>(lhs.payload) != std::get<std::string>(rhs.payload);
                break;
            }
            case ConcatString:
            {
                out.tag = String32;
                out.payload =
                    std::get<std::string>(lhs.payload) + std::get<std::string>(rhs.payload);
                break;
            }
            default:
                return;
            }
            stack.push_back(std::move(out));
            break;
        }
        // Unary operators
        case NegI32:
        {
            Value v = stack.back();
            stack.pop_back();

            Value out;
            out.tag = I32;
            out.payload = -std::get<int>(v.payload);
            stack.push_back(std::move(out));
            break;
        }
        case NotBool:
        {
            Value v = stack.back();
            stack.pop_back();

            Value out;
            out.payload = !std::get<bool>(v.payload);
            stack.push_back(std::move(out));
            break;
        }
        // Conversions
        case ToString:
        {
            Value v = stack.back();
            stack.pop_back();

            Value out;
            out.tag = String32;
            if (std::holds_alternative<int>(v.payload))
            {
                out.payload = std::to_string(std::get<int>(v.payload));
            }
            else if (std::holds_alternative<bool>(v.payload))
            {
                out.payload = std::get<bool>(v.payload) ? "affirmative" : "negative";
            }
            else if (std::holds_alternative<std::string>(v.payload))
            {
                out.payload = std::get<std::string>(v.payload);
            }
            else
            {
                return;
            }
            stack.push_back(std::move(out));
            break;
        }
        case PrintString:
        {
            Value v = stack.back();
            stack.pop_back();

            std::cout << std::get<std::string>(v.payload);
            break;
        }
        case JumpIfFalse:
        {
            LabelId target = std::get<LabelId>(op);

            Value cond = stack.back();
            stack.pop_back();

            if (!std::get<bool>(cond.payload))
            {
                ip = function.labelTable.position.at(target);
                continue; // do NOT ip++
            }
            break;
        }
        case Jump:
        {
            LabelId target = std::get<LabelId>(op);
            ip = function.labelTable.position.at(target);
            continue; // do NOT ip++
        }
        case JLabel:
        case Nop:
        default:
            break;
            ;
        }
        ip++;
    }
}