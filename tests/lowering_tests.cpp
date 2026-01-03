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

TEST(Lowering_Basics, SayIntLiteral)
{
    IrProgram ir = lowerFromSource("say 5;");

    ASSERT_EQ(ir.constants.size(), 1u);
    EXPECT_EQ(ir.constants[0].type, I32);
    EXPECT_EQ(std::get<int>(ir.constants[0].value), 5);

    const auto& instrs = ir.main.instructions;
    ASSERT_EQ(instrs.size(), 3u);

    EXPECT_EQ(instrs[0].opcode, PushConst);
    EXPECT_EQ(std::get<ConstId>(instrs[0].operand), ConstId{0});

    EXPECT_EQ(instrs[1].opcode, ToString);
    EXPECT_EQ(instrs[2].opcode, PrintString);
}

TEST(Lowering_Basics, SayBoolLiteral)
{
    IrProgram ir = lowerFromSource("say affirmative;");

    ASSERT_EQ(ir.constants.size(), 1u);
    EXPECT_EQ(ir.constants[0].type, Bool32);
    EXPECT_EQ(std::get<bool>(ir.constants[0].value), true);

    const auto& instrs = ir.main.instructions;
    ASSERT_EQ(instrs.size(), 3u);

    EXPECT_EQ(instrs[0].opcode, PushConst);
    EXPECT_EQ(std::get<ConstId>(instrs[0].operand), ConstId{0});

    EXPECT_EQ(instrs[1].opcode, ToString);
    EXPECT_EQ(instrs[2].opcode, PrintString);
}

TEST(Lowering_Basics, SayStringLiteral)
{
    IrProgram ir = lowerFromSource(R"(say "hello";)");

    ASSERT_EQ(ir.constants.size(), 1u);
    EXPECT_EQ(ir.constants[0].type, String32);
    EXPECT_EQ(std::get<std::string>(ir.constants[0].value), "hello");

    const auto& instrs = ir.main.instructions;
    // String already on stack, so no ToString needed.
    ASSERT_EQ(instrs.size(), 2u);

    EXPECT_EQ(instrs[0].opcode, PushConst);
    EXPECT_EQ(std::get<ConstId>(instrs[0].operand), ConstId{0});

    EXPECT_EQ(instrs[1].opcode, PrintString);
}

TEST(Lowering_Variables, SummonThenUse)
{
    IrProgram ir = lowerFromSource(R"(
        summon x = 5;
        say x;
    )");

    ASSERT_EQ(ir.main.localTable.locals.size(), 1u);
    EXPECT_EQ(ir.main.localTable.locals[0].debugName, "x");
    EXPECT_EQ(ir.main.localTable.locals[0].type, I32);

    const auto& instrs = ir.main.instructions;
    ASSERT_EQ(instrs.size(), 5u);

    // summon x = 5;
    EXPECT_EQ(instrs[0].opcode, PushConst);
    EXPECT_EQ(std::get<ConstId>(instrs[0].operand), ConstId{0});

    EXPECT_EQ(instrs[1].opcode, StoreLocal);
    EXPECT_EQ(std::get<LocalId>(instrs[1].operand), LocalId{0});

    // say x;
    EXPECT_EQ(instrs[2].opcode, LoadLocal);
    EXPECT_EQ(std::get<LocalId>(instrs[2].operand), LocalId{0});

    EXPECT_EQ(instrs[3].opcode, ToString);
    EXPECT_EQ(instrs[4].opcode, PrintString);
}

TEST(Lowering_Scopes, BlockShadowing)
{
    IrProgram ir = lowerFromSource(R"(
        summon x = 1;
        {
            summon x = 2;
            say x;
        }
        say x;
    )");

    ASSERT_EQ(ir.main.localTable.locals.size(), 2u);
    EXPECT_EQ(ir.main.localTable.locals[0].debugName, "x");
    EXPECT_EQ(ir.main.localTable.locals[1].debugName, "x");

    const auto& instrs = ir.main.instructions;

    // Ensure we load two *different* locals (inner and outer).
    std::vector<LocalId> loaded;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == LoadLocal)
        {
            loaded.push_back(std::get<LocalId>(inst.operand));
        }
    }

    ASSERT_EQ(loaded.size(), 2u);
    EXPECT_FALSE(loaded[0] == loaded[1]);
}

TEST(Lowering_Expressions, BinaryAdd)
{
    IrProgram ir = lowerFromSource("say 1 + 2;");

    ASSERT_EQ(ir.constants.size(), 2u);

    const auto& instrs = ir.main.instructions;
    ASSERT_EQ(instrs.size(), 5u);

    EXPECT_EQ(instrs[0].opcode, PushConst);
    EXPECT_EQ(std::get<ConstId>(instrs[0].operand), ConstId{0});

    EXPECT_EQ(instrs[1].opcode, PushConst);
    EXPECT_EQ(std::get<ConstId>(instrs[1].operand), ConstId{1});

    EXPECT_EQ(instrs[2].opcode, AddI32);

    EXPECT_EQ(instrs[3].opcode, ToString);
    EXPECT_EQ(instrs[4].opcode, PrintString);
}

// ----------------------
// 4) Control flow shape tests
// ----------------------

TEST(Lowering_ControlFlow, WhileLoop_HasJumpsAndLabels)
{
    IrProgram ir = lowerFromSource(R"(
        summon x = 0;
        aslongas (x < 3) {
            say x;
        }
    )");

    printIrProgram(ir);

    const auto& instrs = ir.main.instructions;

    bool sawLabel = false;
    bool sawJumpIfFalse = false;
    bool sawJumpBack = false;

    for (const auto& inst : instrs)
    {
        if (inst.opcode == JLabel)
            sawLabel = true;
        if (inst.opcode == JumpIfFalse)
            sawJumpIfFalse = true;
        if (inst.opcode == Jump)
            sawJumpBack = true;
    }

    EXPECT_TRUE(sawLabel);
    EXPECT_TRUE(sawJumpIfFalse);
    EXPECT_TRUE(sawJumpBack);
}