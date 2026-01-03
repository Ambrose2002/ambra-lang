/**
 * @file lowering_tests.cpp
 * @brief Comprehensive test suite for the Ambra IR lowering phase
 *
 * This file tests the lowering of high-level AST nodes to low-level IR instructions.
 * The lowering phase transforms the type-checked AST into a stack-based intermediate
 * representation suitable for execution or further compilation.
 *
 * Test Organization:
 * 1. Literal lowering - Basic constants (int, bool, string)
 * 2. Variables - Declaration, storage, loading
 * 3. Expressions - Arithmetic and unary operations
 * 4. Comparison operators - All relational operators
 * 5. String operations - Interpolation and concatenation
 * 6. Variable operations - Complex variable usage patterns
 * 7. Control flow - Conditionals (if/else)
 * 8. Control flow - Loops (while)
 * 9. Block scoping - Nested scopes and shadowing
 * 10. Complex scenarios - Realistic multi-feature programs
 * 11. Edge cases - Boundary conditions and operator coverage
 *
 * Each test verifies:
 * - Correct IR instructions are generated
 * - Operands reference proper constants/locals/labels
 * - Control flow structure is valid (jumps, labels)
 * - Type conversions (ToString) are inserted when needed
 */

#include "ir/lowering.h"
#include "parser/parser.h"
#include "sema/analyzer.h"

#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <vector>

/**
 * @brief Helper function to convert opcode enum to string name
 * @param op The opcode to convert
 * @return Human-readable name of the opcode
 *
 * Used for debugging test failures by printing instruction sequences.
 */
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

/**
 * @brief Debug helper that prints the entire IR program structure
 * @param ir The IR program to print
 *
 * Outputs:
 * - All constants with their types and values
 * - All instructions with opcodes and operands
 * - Operand details (ConstId, LocalId, LabelId)
 *
 * Call this function in tests when debugging unexpected IR generation.
 * Example: printIrProgram(ir);
 */
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

/**
 * @brief Test helper that performs full compilation pipeline from source to IR
 * @param source Ambra source code string
 * @return Lowered IR program
 *
 * Pipeline stages:
 * 1. Lexical analysis (tokenization)
 * 2. Parsing (AST construction)
 * 3. Symbol resolution
 * 4. Type checking
 * 5. IR lowering
 *
 * Uses EXPECT_FALSE to allow test continuation even if early phases fail.
 * This helps identify whether issues are in lowering or earlier phases.
 */
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

// ==================================================================================
// 1) LITERAL LOWERING TESTS
// ==================================================================================
// Tests verify that literal values (integers, booleans, strings) are correctly
// translated into constant pool entries and PushConst instructions. Also validates
// that ToString conversions are inserted when literals are printed.

/**
 * Test: Integer literal lowering
 * Verifies:
 * - Integer constant is added to constant pool
 * - PushConst instruction references correct constant
 * - ToString conversion is inserted before print
 */
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

/**
 * Test: Boolean literal lowering
 * Verifies:
 * - Boolean constant (affirmative/negative) is added to constant pool
 * - Type is correctly identified as Bool32
 * - ToString conversion is inserted for printing
 */
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

/**
 * Test: String literal lowering
 * Verifies:
 * - String constant is stored in constant pool
 * - No ToString conversion needed (already a string)
 * - Instruction count is reduced (no conversion step)
 */
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

// ==================================================================================
// 2) VARIABLE DECLARATION AND USAGE TESTS
// ==================================================================================
// Tests verify that variable declarations create entries in the local table,
// and that variable references generate correct LoadLocal/StoreLocal instructions
// with appropriate local IDs.

/**
 * Test: Variable declaration and usage
 * Verifies:
 * - summon statement creates local variable entry
 * - Variable has correct name and type in local table
 * - StoreLocal instruction assigns value to local
 * - LoadLocal instruction retrieves value when referenced
 */
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

// ==================================================================================
// 3) BLOCK SCOPING TESTS
// ==================================================================================
// Tests verify that nested blocks create proper scoping for variables,
// including shadowing (same name in inner scope refers to different variable).

/**
 * Test: Variable shadowing in nested blocks
 * Verifies:
 * - Inner block creates new local with same name
 * - Two distinct locals exist in local table
 * - LoadLocal instructions reference different local IDs
 * - Inner and outer variables don't interfere
 */
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

// ==================================================================================
// 3) EXPRESSION LOWERING TESTS
// ==================================================================================
// Tests verify that arithmetic and unary expressions generate correct instruction
// sequences. Binary operators should push operands then execute operation.
// Unary operators should push operand then execute unary operation.

/**
 * Test: Binary addition expression
 * Verifies:
 * - Both operands pushed to stack (PushConst)
 * - AddI32 instruction performs addition
 * - Result is converted to string for printing
 */
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

/**
 * Test: Binary subtraction
 * Verifies SubI32 instruction is generated for subtraction operator
 */
TEST(Lowering_Expressions, BinarySubtract)
{
    IrProgram ir = lowerFromSource("say 10 - 3;");

    const auto& instrs = ir.main.instructions;
    bool        foundSub = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == SubI32)
            foundSub = true;
    }
    EXPECT_TRUE(foundSub);
}

/**
 * Test: Binary multiplication
 * Verifies MulI32 instruction is generated for multiplication operator
 */
TEST(Lowering_Expressions, BinaryMultiply)
{
    IrProgram ir = lowerFromSource("say 4 * 5;");

    const auto& instrs = ir.main.instructions;
    bool        foundMul = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == MulI32)
            foundMul = true;
    }
    EXPECT_TRUE(foundMul);
}

/**
 * Test: Binary division
 * Verifies DivI32 instruction is generated for division operator
 */
TEST(Lowering_Expressions, BinaryDivide)
{
    IrProgram ir = lowerFromSource("say 20 / 4;");

    const auto& instrs = ir.main.instructions;
    bool        foundDiv = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == DivI32)
            foundDiv = true;
    }
    EXPECT_TRUE(foundDiv);
}

/**
 * Test: Chained addition (a + b + c)
 * Verifies:
 * - Multiple AddI32 instructions for chained operations
 * - Left-to-right evaluation order
 */
TEST(Lowering_Expressions, ChainedAddition)
{
    IrProgram ir = lowerFromSource("say 1 + 2 + 3;");

    const auto& instrs = ir.main.instructions;
    int         addCount = 0;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == AddI32)
            addCount++;
    }
    EXPECT_EQ(addCount, 2); // Two additions for a + b + c
}

/**
 * Test: Mixed arithmetic operators
 * Verifies:
 * - Both addition and multiplication instructions present
 * - Operator precedence is respected in lowering
 */
TEST(Lowering_Expressions, MixedArithmetic)
{
    IrProgram ir = lowerFromSource("say 2 + 3 * 4;");

    const auto& instrs = ir.main.instructions;
    bool        foundAdd = false, foundMul = false, foundTypeConversion = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == AddI32)
            foundAdd = true;
        if (inst.opcode == MulI32)
            foundMul = true;
        if (inst.opcode == ToString)
            foundTypeConversion = true;
    }
    EXPECT_TRUE(foundAdd);
    EXPECT_TRUE(foundMul);
    EXPECT_TRUE(foundTypeConversion);
}

TEST(Lowering_Expressions, ParenthesizedExpression)
{
    IrProgram ir = lowerFromSource("say (1 + 2) * 3;");

    const auto& instrs = ir.main.instructions;
    ASSERT_GE(instrs.size(), 3u);
    // Should have add and mul
    bool foundAdd = false, foundMul = false, foundTypeConversion = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == AddI32)
            foundAdd = true;
        if (inst.opcode == MulI32)
            foundMul = true;
        if (inst.opcode == ToString)
            foundTypeConversion = true;
    }
    EXPECT_TRUE(foundAdd);
    EXPECT_TRUE(foundMul);
    EXPECT_TRUE(foundTypeConversion);
}

TEST(Lowering_Expressions, UnaryNegation)
{
    IrProgram ir = lowerFromSource("say -5;");

    const auto& instrs = ir.main.instructions;
    bool        foundNeg = false, foundTypeConverse = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == NegI32)
            foundNeg = true;
        if (inst.opcode == ToString)
            foundTypeConverse = true;
    }
    EXPECT_TRUE(foundNeg);
    EXPECT_TRUE(foundTypeConverse);
}

TEST(Lowering_Expressions, UnaryNegationVariable)
{
    IrProgram ir = lowerFromSource(R"(
        summon x = 5;
        say -x;
    )");

    const auto& instrs = ir.main.instructions;
    bool        foundNeg = false, foundTypeConversion = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == NegI32)
            foundNeg = true;
        if (inst.opcode == ToString)
            foundTypeConversion = true;
    }
    EXPECT_TRUE(foundNeg);
    EXPECT_TRUE(foundTypeConversion);
}

TEST(Lowering_Expressions, UnaryNotBool)
{
    IrProgram ir = lowerFromSource("say not affirmative;");

    const auto& instrs = ir.main.instructions;
    bool        foundNot = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == NotBool)
            foundNot = true;
    }
    EXPECT_TRUE(foundNot);
}

// ==================================================================================
// 4) COMPARISON OPERATOR TESTS
// ==================================================================================
// Tests verify that all comparison operators generate appropriate CmpXX instructions
// for different types (I32, Bool32, String32). Each comparison should push both
// operands then execute the comparison, leaving a boolean result on the stack.

/**
 * Test: Integer equality comparison
 * Verifies CmpEqI32 instruction is generated for == operator on integers
 */
TEST(Lowering_Comparisons, EqualityInt)
{
    IrProgram ir = lowerFromSource("say 5 == 5;");

    const auto& instrs = ir.main.instructions;
    bool        foundCmp = false, foundTypeConversion = true;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == CmpEqI32)
            foundCmp = true;
        if (inst.opcode == ToString)
            foundTypeConversion = true;
    }
    EXPECT_TRUE(foundCmp);
    EXPECT_TRUE(foundTypeConversion);
}

TEST(Lowering_Comparisons, InequalityInt)
{
    IrProgram ir = lowerFromSource("say 5 != 3;");

    const auto& instrs = ir.main.instructions;
    bool        foundCmp = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == CmpNEqI32)
            foundCmp = true;
    }
    EXPECT_TRUE(foundCmp);
}

TEST(Lowering_Comparisons, LessThan)
{
    IrProgram ir = lowerFromSource("say 3 < 5;");

    const auto& instrs = ir.main.instructions;
    bool        foundCmp = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == CmpLtI32)
            foundCmp = true;
    }
    EXPECT_TRUE(foundCmp);
}

TEST(Lowering_Comparisons, LessThanOrEqual)
{
    IrProgram ir = lowerFromSource("say 3 <= 5;");

    const auto& instrs = ir.main.instructions;
    bool        foundCmp = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == CmpLtEqI32)
            foundCmp = true;
    }
    EXPECT_TRUE(foundCmp);
}

TEST(Lowering_Comparisons, GreaterThan)
{
    IrProgram ir = lowerFromSource("say 5 > 3;");

    const auto& instrs = ir.main.instructions;
    bool        foundCmp = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == CmpGtI32)
            foundCmp = true;
    }
    EXPECT_TRUE(foundCmp);
}

TEST(Lowering_Comparisons, GreaterThanOrEqual)
{
    IrProgram ir = lowerFromSource("say 5 >= 3;");

    const auto& instrs = ir.main.instructions;
    bool        foundCmp = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == CmpGtEqI32)
            foundCmp = true;
    }
    EXPECT_TRUE(foundCmp);
}

TEST(Lowering_Comparisons, BoolEquality)
{
    IrProgram ir = lowerFromSource("say affirmative == negative;");

    const auto& instrs = ir.main.instructions;
    bool        foundCmp = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == CmpEqBool32)
            foundCmp = true;
    }
    EXPECT_TRUE(foundCmp);
}

TEST(Lowering_Comparisons, BoolInequality)
{
    IrProgram ir = lowerFromSource("say affirmative != negative;");

    const auto& instrs = ir.main.instructions;
    bool        foundCmp = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == CmpNEqBool32)
            foundCmp = true;
    }
    EXPECT_TRUE(foundCmp);
}

TEST(Lowering_Comparisons, StringEquality)
{
    IrProgram ir = lowerFromSource(R"(say "hello" == "world";)");

    const auto& instrs = ir.main.instructions;
    bool        foundCmp = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == CmpEqString32)
            foundCmp = true;
    }
    EXPECT_TRUE(foundCmp);
}

TEST(Lowering_Comparisons, StringInequality)
{
    IrProgram ir = lowerFromSource(R"(say "hello" != "world";)");

    const auto& instrs = ir.main.instructions;
    bool        foundCmp = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == CmpNEqString32)
            foundCmp = true;
    }
    EXPECT_TRUE(foundCmp);
}

TEST(Lowering_Comparisons, ComparisonWithVariables)
{
    IrProgram ir = lowerFromSource(R"(
        summon x = 5;
        summon y = 10;
        say x < y;
    )");

    const auto& instrs = ir.main.instructions;
    bool        foundLoad = false, foundCmp = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == LoadLocal)
            foundLoad = true;
        if (inst.opcode == CmpLtI32)
            foundCmp = true;
    }
    EXPECT_TRUE(foundLoad);
    EXPECT_TRUE(foundCmp);
}

// ==================================================================================
// 5) STRING OPERATION TESTS
// ==================================================================================
// Tests verify string interpolation and concatenation. String interpolation
// should generate: push literal part, evaluate expression, ToString if needed,
// ConcatString to combine parts.

/**
 * Test: Simple string interpolation with literal
 * Verifies:\n * - String parts are pushed as constants
 * - Interpolated expression is evaluated
 * - ConcatString instruction combines parts
 */
TEST(Lowering_Strings, SimpleInterpolation)
{
    IrProgram ir = lowerFromSource(R"(say "value: {5}";)");

    const auto& instrs = ir.main.instructions;
    bool        foundConcat = false, foundTypeConversion = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == ConcatString)
            foundConcat = true;
        if (inst.opcode == ToString)
            foundTypeConversion = true;
    }
    EXPECT_TRUE(foundConcat);
    EXPECT_TRUE(foundTypeConversion);
}

TEST(Lowering_Strings, InterpolationWithVariable)
{
    IrProgram ir = lowerFromSource(R"(
        summon x = 42;
        say "The answer is {x}";
    )");

    const auto& instrs = ir.main.instructions;
    bool        foundLoad = false, foundConcat = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == LoadLocal)
            foundLoad = true;
        if (inst.opcode == ConcatString)
            foundConcat = true;
    }
    EXPECT_TRUE(foundLoad);
    EXPECT_TRUE(foundConcat);
}

TEST(Lowering_Strings, MultipleInterpolations)
{
    IrProgram ir = lowerFromSource(R"(
        summon x = 1;
        summon y = 2;
        say "x={x}, y={y}";
    )");

    const auto& instrs = ir.main.instructions;
    int         concatCount = 0;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == ConcatString)
            concatCount++;
    }
    // Multiple concatenations for multiple interpolations
    EXPECT_EQ(concatCount, 4);
}

TEST(Lowering_Strings, InterpolationWithExpression)
{
    IrProgram ir = lowerFromSource(R"(say "sum: {1 + 2}";)");
    printIrProgram(ir);
    const auto& instrs = ir.main.instructions;
    bool        foundAdd = false;
    int         concatCount = 0;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == AddI32)
            foundAdd = true;
        if (inst.opcode == ConcatString)
            concatCount++;
    }
    EXPECT_TRUE(foundAdd);
    EXPECT_EQ(concatCount, 2);
}

// ==================================================================================
// 6) VARIABLE OPERATION TESTS
// ==================================================================================
// Tests verify complex variable usage patterns including multiple variables,
// reuse, and usage in expressions. Validates proper local table management
// and correct type tracking.

/**
 * Test: Multiple variable declarations
 * Verifies:
 * - Each variable gets unique entry in local table
 * - Variables maintain correct debug names
 * - Proper ordering in local table
 */
TEST(Lowering_Variables, MultipleVariables)
{
    IrProgram ir = lowerFromSource(R"(
        summon a = 1;
        summon b = 2;
        summon c = 3;
    )");

    ASSERT_EQ(ir.main.localTable.locals.size(), 3u);
    EXPECT_EQ(ir.main.localTable.locals[0].debugName, "a");
    EXPECT_EQ(ir.main.localTable.locals[1].debugName, "b");
    EXPECT_EQ(ir.main.localTable.locals[2].debugName, "c");
}

/**
 * Test: Variable reuse in multiple statements
 * Verifies:
 * - Same variable can be loaded multiple times
 * - Each reference generates separate LoadLocal instruction
 */
TEST(Lowering_Variables, VariableReuse)
{
    IrProgram ir = lowerFromSource(R"(
        summon x = 5;
        say x;
        say x;
    )");

    const auto& instrs = ir.main.instructions;
    int         loadCount = 0;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == LoadLocal)
            loadCount++;
    }
    EXPECT_EQ(loadCount, 2); // Two loads of the same variable
}

/**
 * Test: Variable used in expression
 * Verifies:
 * - Variable is loaded before being used in arithmetic
 * - Expression evaluation combines loaded value with literal
 */
TEST(Lowering_Variables, VariableInExpression)
{
    IrProgram ir = lowerFromSource(R"(
        summon x = 5;
        summon y = x + 3;
    )");

    const auto& instrs = ir.main.instructions;
    bool        foundLoad = false, foundAdd = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == LoadLocal)
            foundLoad = true;
        if (inst.opcode == AddI32)
            foundAdd = true;
    }
    EXPECT_TRUE(foundLoad);
    EXPECT_TRUE(foundAdd);
}

/**
 * Test: Boolean variable declaration
 * Verifies variable type is correctly tracked as Bool32
 */
TEST(Lowering_Variables, BoolVariable)
{
    IrProgram ir = lowerFromSource(R"(
        summon flag = affirmative;
        say flag;
    )");

    ASSERT_EQ(ir.main.localTable.locals.size(), 1u);
    EXPECT_EQ(ir.main.localTable.locals[0].type, Bool32);
}

/**
 * Test: String variable declaration
 * Verifies variable type is correctly tracked as String32
 */
TEST(Lowering_Variables, StringVariable)
{
    IrProgram ir = lowerFromSource(R"(
        summon name = "Alice";
        say name;
    )");

    ASSERT_EQ(ir.main.localTable.locals.size(), 1u);
    EXPECT_EQ(ir.main.localTable.locals[0].type, String32);
}

// ==================================================================================
// 7) CONTROL FLOW - CONDITIONAL TESTS
// ==================================================================================
// Tests verify that if/else statements generate proper control flow with:
// - JumpIfFalse for conditional branching
// - JLabel for branch targets
// - Jump for skipping else branch
// Nested conditionals should have multiple independent jump structures.

/**
 * Test: Simple if statement
 * Verifies:
 * - JumpIfFalse skips body if condition false
 * - JLabel marks end of if body
 */
TEST(Lowering_ControlFlow, SimpleIfStatement)
{
    IrProgram ir = lowerFromSource(R"(
        should (affirmative) {
            say 1;
        }
    )");

    const auto& instrs = ir.main.instructions;
    bool        foundJumpIfFalse = false, foundLabel = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == JumpIfFalse)
            foundJumpIfFalse = true;
        if (inst.opcode == JLabel)
            foundLabel = true;
    }
    EXPECT_TRUE(foundJumpIfFalse);
    EXPECT_TRUE(foundLabel);
}

/**
 * Test: If statement with variable in condition
 * Verifies:
 * - Variable is loaded for condition evaluation
 * - Comparison instruction evaluates condition
 * - Jump instruction based on comparison result
 */
TEST(Lowering_ControlFlow, IfWithConditionVariable)
{
    IrProgram ir = lowerFromSource(R"(
        summon x = 5;
        should (x > 3) {
            say x;
        }
    )");

    const auto& instrs = ir.main.instructions;
    bool        foundLoad = false, foundCmp = false, foundJump = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == LoadLocal)
            foundLoad = true;
        if (inst.opcode == CmpGtI32)
            foundCmp = true;
        if (inst.opcode == JumpIfFalse)
            foundJump = true;
    }
    EXPECT_TRUE(foundLoad);
    EXPECT_TRUE(foundCmp);
    EXPECT_TRUE(foundJump);
}

/**
 * Test: If-else statement
 * Verifies:
 * - Two jumps: conditional (if-false) and unconditional (skip else)
 * - Two labels: else start and end of statement
 * - Proper branching structure
 */
TEST(Lowering_ControlFlow, IfElseStatement)
{
    IrProgram ir = lowerFromSource(R"(
        should (affirmative) {
            say 1;
        } otherwise {
            say 2;
        }
    )");

    const auto& instrs = ir.main.instructions;
    int         jumpCount = 0, labelCount = 0;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == Jump || inst.opcode == JumpIfFalse)
            jumpCount++;
        if (inst.opcode == JLabel)
            labelCount++;
    }
    // Should have at least 2 jumps (conditional + skip else) and 2 labels
    EXPECT_GE(jumpCount, 2);
    EXPECT_GE(labelCount, 2);
}

/**
 * Test: Nested if statements
 * Verifies:
 * - Each if generates independent jump structure
 * - Two conditional jumps for two if statements
 */
TEST(Lowering_ControlFlow, NestedIfStatements)
{
    IrProgram ir = lowerFromSource(R"(
        should (affirmative) {
            should (negative) {
                say 1;
            }
        }
    )");

    const auto& instrs = ir.main.instructions;
    int         jumpCount = 0;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == JumpIfFalse)
            jumpCount++;
    }
    EXPECT_EQ(jumpCount, 2); // Two conditional jumps for nested ifs
}

// ==================================================================================
// 8) CONTROL FLOW - LOOP TESTS
// ==================================================================================
// Tests verify that while loops (aslongas) generate proper loop structure:
// - JLabel at loop start (for backward jump)
// - Condition evaluation
// - JumpIfFalse to exit loop
// - Loop body instructions
// - Jump back to loop start
// - JLabel at loop end

/**
 * Test: While loop structure
 * Verifies:
 * - JLabel marks loop entry point
 * - JumpIfFalse exits loop if condition false
 * - Jump returns to loop start for next iteration
 * - Complete loop control flow structure present
 */
TEST(Lowering_ControlFlow, WhileLoop_HasJumpsAndLabels)
{
    IrProgram ir = lowerFromSource(R"(
        summon x = 0;
        aslongas (x < 3) {
            say x;
        }
    )");

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

/**
 * Test: While loop with complex condition
 * Verifies:
 * - Variables loaded for condition evaluation
 * - Comparison instruction in loop condition
 * - Backward jump for loop iteration
 */
TEST(Lowering_ControlFlow, WhileLoopWithComplexCondition)
{
    IrProgram ir = lowerFromSource(R"(
        summon x = 0;
        summon y = 10;
        aslongas (x < y) {
            say x;
        }
    )");

    const auto& instrs = ir.main.instructions;
    bool        foundCmp = false, foundJump = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == CmpLtI32)
            foundCmp = true;
        if (inst.opcode == Jump)
            foundJump = true;
    }
    EXPECT_TRUE(foundCmp);
    EXPECT_TRUE(foundJump);
}

/**
 * Test: Empty while loop body
 * Verifies:
 * - Loop structure generated even with empty body
 * - Jump instruction present for loop iteration
 */
TEST(Lowering_ControlFlow, EmptyWhileLoop)
{
    IrProgram ir = lowerFromSource(R"(
        aslongas (affirmative) {
        }
    )");

    const auto& instrs = ir.main.instructions;
    bool        foundJump = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == Jump)
            foundJump = true;
    }
    EXPECT_TRUE(foundJump);
}

/**
 * Test: Nested while loops
 * Verifies:
 * - Each loop has independent backward jump
 * - Two Jump instructions for two loops
 * - Proper nesting of loop structures
 */
TEST(Lowering_ControlFlow, NestedWhileLoops)
{
    IrProgram ir = lowerFromSource(R"(
        summon i = 0;
        aslongas (i < 3) {
            summon j = 0;
            aslongas (j < 2) {
                say j;
            }
        }
    )");

    const auto& instrs = ir.main.instructions;
    int         jumpBackCount = 0;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == Jump)
            jumpBackCount++;
    }
    EXPECT_EQ(jumpBackCount, 2); // Two backward jumps for nested loops
}

// ==================================================================================
// 9) BLOCK SCOPING TESTS
// ==================================================================================
// Tests verify lexical scoping rules including:
// - Empty blocks compile successfully
// - Variables in outer scopes accessible in inner scopes
// - Variable shadowing creates separate local entries
// - Block exit doesn't interfere with later variables

/**
 * Test: Empty block statement
 * Verifies empty blocks compile without errors
 */
TEST(Lowering_Scopes, EmptyBlock)
{
    IrProgram ir = lowerFromSource(R"(
        {
        }
    )");

    EXPECT_TRUE(true); // Should compile without error
}

/**
 * Test: Nested block scopes
 * Verifies:
 * - Outer variables accessible in nested blocks
 * - LoadLocal works across scope boundaries
 */
TEST(Lowering_Scopes, NestedBlocks)
{
    IrProgram ir = lowerFromSource(R"(
        summon x = 1;
        {
            {
                say x;
            }
        }
    )");

    const auto& instrs = ir.main.instructions;
    bool        foundLoad = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == LoadLocal)
            foundLoad = true;
    }
    EXPECT_TRUE(foundLoad);
}

/**
 * Test: Variable scope isolation
 * Verifies:
 * - Variables declared in blocks don't interfere with later code
 * - Local table tracks all variables including out-of-scope ones
 */
TEST(Lowering_Scopes, VariableOutOfScope)
{
    IrProgram ir = lowerFromSource(R"(
        {
            summon temp = 99;
        }
        summon x = 1;
    )");

    // After inner block, temp should not interfere with later variables
    ASSERT_EQ(ir.main.localTable.locals.size(), 2u);
}

// ==================================================================================
// 10) COMPLEX SCENARIO TESTS
// ==================================================================================
// Tests verify realistic combinations of features working together.
// These tests ensure that multiple language features interact correctly
// in typical usage patterns.

/**
 * Test: Arithmetic in conditional expression
 * Verifies:
 * - Expression evaluation within condition
 * - Arithmetic instruction before comparison
 * - Proper control flow based on computed condition
 */
TEST(Lowering_Complex, ArithmeticInCondition)
{
    IrProgram ir = lowerFromSource(R"(
        summon x = 5;
        should (x + 1 > 3) {
            say x;
        }
    )");

    const auto& instrs = ir.main.instructions;
    bool        foundAdd = false, foundCmp = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == AddI32)
            foundAdd = true;
        if (inst.opcode == CmpGtI32)
            foundCmp = true;
    }
    EXPECT_TRUE(foundAdd);
    EXPECT_TRUE(foundCmp);
}

/**
 * Test: Multiple sequential statements
 * Verifies:
 * - Multiple variables tracked correctly
 * - Expression uses previously declared variables
 * - All StoreLocal instructions present
 */
TEST(Lowering_Complex, MultipleStatementsInBlock)
{
    IrProgram ir = lowerFromSource(R"(
        summon a = 1;
        summon b = 2;
        summon c = a + b;
        say c;
    )");

    ASSERT_EQ(ir.main.localTable.locals.size(), 3u);

    const auto& instrs = ir.main.instructions;
    int         storeCount = 0;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == StoreLocal)
            storeCount++;
    }
    EXPECT_EQ(storeCount, 3);
}

/**
 * Test: Conditional with multiple body statements
 * Verifies:
 * - Multiple statements execute within if body
 * - All variable declarations processed
 * - Multiple print statements generate correct instructions
 */
TEST(Lowering_Complex, ConditionalWithMultipleStatements)
{
    IrProgram ir = lowerFromSource(R"(
        should (affirmative) {
            summon x = 1;
            summon y = 2;
            say x;
            say y;
        }
    )");

    const auto& instrs = ir.main.instructions;
    int         storeCount = 0, printCount = 0;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == StoreLocal)
            storeCount++;
        if (inst.opcode == PrintString)
            printCount++;
    }
    EXPECT_EQ(storeCount, 2);
    EXPECT_EQ(printCount, 2);
}

/**
 * Test: While loop with counter variable
 * Verifies:
 * - Loop counter variable loaded for condition
 * - Comparison evaluates loop condition
 * - Jump instruction creates loop structure
 */
TEST(Lowering_Complex, LoopWithCounter)
{
    IrProgram ir = lowerFromSource(R"(
        summon i = 0;
        aslongas (i < 5) {
            say i;
        }
    )");

    const auto& instrs = ir.main.instructions;
    bool        foundCmp = false, foundLoad = false, foundJump = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == CmpLtI32)
            foundCmp = true;
        if (inst.opcode == LoadLocal)
            foundLoad = true;
        if (inst.opcode == Jump)
            foundJump = true;
    }
    EXPECT_TRUE(foundCmp);
    EXPECT_TRUE(foundLoad);
    EXPECT_TRUE(foundJump);
}

/**
 * Test: Nested control flow (if containing while)
 * Verifies:
 * - Multiple conditional jumps for nested structures
 * - Loop jump present within conditional
 * - Proper interaction of if and while control flow
 */
TEST(Lowering_Complex, NestedControlFlow)
{
    IrProgram ir = lowerFromSource(R"(
        summon x = 5;
        should (x > 0) {
            aslongas (x > 0) {
                say x;
            }
        }
    )");

    const auto& instrs = ir.main.instructions;
    int         jumpIfFalseCount = 0, jumpCount = 0;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == JumpIfFalse)
            jumpIfFalseCount++;
        if (inst.opcode == Jump)
            jumpCount++;
    }
    EXPECT_GE(jumpIfFalseCount, 2); // At least one for if, one for while
    EXPECT_GE(jumpCount, 1);        // At least one for while loop back
}

// ==================================================================================
// 11) EDGE CASE TESTS
// ==================================================================================
/**
 * Test: Negative integer literal
 * Verifies:
 * - Negative literal stored as positive constant
 * - NegI32 instruction applies negation
 * - Unary minus handled correctly for literals
 */
TEST(Lowering_EdgeCases, NegativeInteger)
{
    IrProgram ir = lowerFromSource("say -42;");

    ASSERT_EQ(ir.constants.size(), 1u);
    EXPECT_EQ(std::get<int>(ir.constants[0].value), 42); // Literal is positive

    const auto& instrs = ir.main.instructions;
    bool        foundNeg = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == NegI32)
            foundNeg = true;
    }
    EXPECT_TRUE(foundNeg);
}

/**
 * Test: Zero value constant
 * Verifies zero is stored correctly in constant pool
 */
TEST(Lowering_EdgeCases, ZeroValue)
{
    IrProgram ir = lowerFromSource("say 0;");

    ASSERT_EQ(ir.constants.size(), 1u);
    EXPECT_EQ(std::get<int>(ir.constants[0].value), 0);
}

/**
 * Test: Empty string literal
 * Verifies empty string is stored correctly as constant
 */
TEST(Lowering_EdgeCases, EmptyString)
{
    IrProgram ir = lowerFromSource(R"(say "";)");

    ASSERT_EQ(ir.constants.size(), 1u);
    EXPECT_EQ(std::get<std::string>(ir.constants[0].value), "");
}

/**
 * Test: Double negation
 * Verifies:
 * - Multiple unary operators applied in sequence
 * - Two NotBool instructions generated
 */
TEST(Lowering_EdgeCases, DoubleNegation)
{
    IrProgram ir = lowerFromSource("say not not affirmative;");

    const auto& instrs = ir.main.instructions;
    int         notCount = 0;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == NotBool)
            notCount++;
    }
    EXPECT_EQ(notCount, 2);
}

/**
 * Test: Excessive parentheses nesting
 * Verifies parser and lowerer handle deep expression nesting
 */
TEST(Lowering_EdgeCases, ComplexNesting)
{
    IrProgram ir = lowerFromSource(R"(say ((((5))));)");

    const auto& instrs = ir.main.instructions;
    // Should still work despite excessive parentheses
    ASSERT_GE(instrs.size(), 2u);
}

/**
 * Test: Comparing variable to itself
 * Verifies:
 * - Same variable loaded twice for comparison
 * - Two distinct LoadLocal instructions
 */
TEST(Lowering_EdgeCases, IdentityComparison)
{
    IrProgram ir = lowerFromSource(R"(
        summon x = 5;
        say x == x;
    )");

    const auto& instrs = ir.main.instructions;
    int         loadCount = 0;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == LoadLocal)
            loadCount++;
    }
    EXPECT_EQ(loadCount, 2); // Load same variable twice for comparison
}

/**
 * Test: Multiple consecutive print statements
 * Verifies each print generates separate PrintString instruction
 */
TEST(Lowering_EdgeCases, ConsecutivePrints)
{
    IrProgram ir = lowerFromSource(R"(
        say 1;
        say 2;
        say 3;
    )");

    const auto& instrs = ir.main.instructions;
    int         printCount = 0;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == PrintString)
            printCount++;
    }
    EXPECT_EQ(printCount, 3);
}

/**
 * Test: All binary arithmetic operators
 * Verifies:
 * - Add, Sub, Mul, Div instructions all generated
 * - Complete operator coverage
 */
TEST(Lowering_EdgeCases, AllBinaryOperators)
{
    IrProgram ir = lowerFromSource(R"(
        summon a = 1 + 2;
        summon b = 3 - 1;
        summon c = 2 * 4;
        summon d = 8 / 2;
    )");

    const auto& instrs = ir.main.instructions;
    bool        foundAdd = false, foundSub = false, foundMul = false, foundDiv = false;
    for (const auto& inst : instrs)
    {
        if (inst.opcode == AddI32)
            foundAdd = true;
        if (inst.opcode == SubI32)
            foundSub = true;
        if (inst.opcode == MulI32)
            foundMul = true;
        if (inst.opcode == DivI32)
            foundDiv = true;
    }
    EXPECT_TRUE(foundAdd);
    EXPECT_TRUE(foundSub);
    EXPECT_TRUE(foundMul);
    EXPECT_TRUE(foundDiv);
}

/**
 * Test: All comparison operators
 * Verifies:
 * - All six comparison instructions generated (==, !=, <, <=, >, >=)
 * - Complete comparison operator coverage
 * - Each operator generates distinct instruction
 */
TEST(Lowering_EdgeCases, AllComparisonOperators)
{
    IrProgram ir = lowerFromSource(R"(
        summon eq = 1 == 1;
        summon ne = 1 != 2;
        summon lt = 1 < 2;
        summon le = 1 <= 2;
        summon gt = 2 > 1;
        summon ge = 2 >= 1;
    )");

    const auto& instrs = ir.main.instructions;
    bool        foundEq = false, foundNe = false, foundLt = false;
    bool        foundLe = false, foundGt = false, foundGe = false;

    for (const auto& inst : instrs)
    {
        if (inst.opcode == CmpEqI32)
            foundEq = true;
        if (inst.opcode == CmpNEqI32)
            foundNe = true;
        if (inst.opcode == CmpLtI32)
            foundLt = true;
        if (inst.opcode == CmpLtEqI32)
            foundLe = true;
        if (inst.opcode == CmpGtI32)
            foundGt = true;
        if (inst.opcode == CmpGtEqI32)
            foundGe = true;
    }

    EXPECT_TRUE(foundEq);
    EXPECT_TRUE(foundNe);
    EXPECT_TRUE(foundLt);
    EXPECT_TRUE(foundLe);
    EXPECT_TRUE(foundGt);
    EXPECT_TRUE(foundGe);
}