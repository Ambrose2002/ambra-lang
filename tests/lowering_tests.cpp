#include "ir/lowering.h"
#include "parser/parser.h"
#include "sema/analyzer.h"

#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <vector>

// Helper to get opcode name for debugging
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

// Helper to print IR program for debugging
static void printIrProgram(const IrProgram& ir)
{
    std::cout << "\n=== IR Program Debug Output ===\n";

    std::cout << "Constants (" << ir.constants.size() << "):\n";
    for (size_t i = 0; i < ir.constants.size(); i++)
    {
        std::cout << "  [" << i << "] type=" << ir.constants[i].type << " value=";
        if (std::holds_alternative<int>(ir.constants[i].value))
        {
            std::cout << std::get<int>(ir.constants[i].value);
        }
        else if (std::holds_alternative<bool>(ir.constants[i].value))
        {
            std::cout << (std::get<bool>(ir.constants[i].value) ? "true" : "false");
        }
        else if (std::holds_alternative<std::string>(ir.constants[i].value))
        {
            std::cout << "\"" << std::get<std::string>(ir.constants[i].value) << "\"";
        }
        std::cout << "\n";
    }

    std::cout << "\nMain Instructions (" << ir.main.instructions.size() << "):\n";
    for (size_t i = 0; i < ir.main.instructions.size(); i++)
    {
        const auto& instr = ir.main.instructions[i];
        std::cout << "  [" << i << "] " << getOpcodeName(instr.opcode);

        if (std::holds_alternative<ConstId>(instr.operand))
        {
            std::cout << " ConstId=" << std::get<ConstId>(instr.operand).value;
        }
        else if (std::holds_alternative<LocalId>(instr.operand))
        {
            std::cout << " LocalId=" << std::get<LocalId>(instr.operand).value;
        }
        else if (std::holds_alternative<LabelId>(instr.operand))
        {
            std::cout << " LabelId=" << std::get<LabelId>(instr.operand).value;
        }

        std::cout << "\n";
    }
    std::cout << "================================\n\n";
}

static IrProgram lowerFromSource(const std::string& source)
{
    Lexer              lexer(source);
    std::vector<Token> tokenList = lexer.scanTokens();

    Parser  parser(tokenList);
    Program program = parser.parseProgram();
    EXPECT_FALSE(program.hadError());

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    EXPECT_FALSE(sema.hadError());

    TypeChecker        tc(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = tc.typeCheck(program);
    EXPECT_FALSE(types.hadError());

    LoweringContext lowerer{nullptr, nullptr, {}, types.typeTable, sema.resolutionTable};

    return lowerer.lowerProgram(&program);
}

