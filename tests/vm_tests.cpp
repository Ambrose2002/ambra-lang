#include "parser/parser.h"
#include "sema/analyzer.h"
#include "vm/vm.h"

#include <gtest/gtest.h>

/*
 * Helper: full pipeline + VM execution with captured stdout
 */
static std::string runProgram(const std::string& source)
{
    // Parse
    Lexer              lexer(source);
    std::vector<Token> tokens = lexer.scanTokens();
    Parser             parser(tokens);
    Program            program = parser.parseProgram();
    EXPECT_FALSE(program.hadError());

    // Resolve
    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    EXPECT_FALSE(sema.hadError());

    // Typecheck
    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);
    EXPECT_FALSE(types.hadError());

    // Lower
    LoweringContext lowerer{nullptr, nullptr, {}, types.typeTable, sema.resolutionTable};

    IrProgram ir = lowerer.lowerProgram(&program);
    EXPECT_FALSE(lowerer.hadError);

    // Validate IR
    IrValidator        validator{ir, ir.main};
    IrValidatorResults vres = validator.validate();
    EXPECT_FALSE(vres.hadError());

    // Capture stdout
    std::stringstream buffer;
    std::streambuf*   old = std::cout.rdbuf(buffer.rdbuf());

    // Execute
    VM vm{ir, ir.main};
    vm.execute();

    // Restore stdout
    std::cout.rdbuf(old);

    return buffer.str();
}

/**
 * Verifies that the VM can print integer literals correctly.
 * Tests the basic print functionality with integer values.
 */
TEST(VM_Basics, PrintInt)
{
    std::string out = runProgram("say 5;");
    EXPECT_EQ(out, "5");
}

/**
 * Verifies that boolean literals are printed correctly.
 * Tests printing of 'affirmative' boolean value.
 */
TEST(VM_Basics, PrintBool)
{
    std::string out = runProgram("say affirmative;");
    EXPECT_EQ(out, "affirmative");
}

/**
 * Verifies that string literals are printed correctly.
 * Tests basic string output functionality.
 */
TEST(VM_Basics, PrintString)
{
    std::string out = runProgram(R"(say "hello";)");
    EXPECT_EQ(out, "hello");
}

/**
 * Verifies that negative boolean literal is printed correctly.
 * Tests printing of 'negative' boolean value.
 */
TEST(VM_Basics, PrintNegativeBool)
{
    std::string out = runProgram("say negative;");
    EXPECT_EQ(out, "negative");
}

/**
 * Verifies that zero is printed correctly.
 * Edge case for integer output.
 */
TEST(VM_Basics, PrintZero)
{
    std::string out = runProgram("say 0;");
    EXPECT_EQ(out, "0");
}

/**
 * Verifies that large integers are printed correctly.
 * Tests VM handling of larger numeric values.
 */
TEST(VM_Basics, PrintLargeInt)
{
    std::string out = runProgram("say 999999;");
    EXPECT_EQ(out, "999999");
}

/**
 * Verifies that negative integers are printed correctly.
 * Tests handling of negative literal values.
 */
TEST(VM_Basics, PrintNegativeInt)
{
    std::string out = runProgram("say -42;");
    EXPECT_EQ(out, "-42");
}

/**
 * Verifies that empty strings are printed correctly.
 * Edge case for string output.
 */
TEST(VM_Basics, PrintEmptyString)
{
    std::string out = runProgram(R"(say "";)");
    EXPECT_EQ(out, "");
}

/**
 * Verifies that integer addition works correctly.
 * Tests basic addition of two positive integers.
 */
TEST(VM_Arithmetic, Addition)
{
    std::string out = runProgram("say 1 + 2;");
    EXPECT_EQ(out, "3");
}

/**
 * Verifies that integer subtraction works correctly.
 * Tests basic subtraction operation.
 */
TEST(VM_Arithmetic, Subtraction)
{
    std::string out = runProgram("say 10 - 3;");
    EXPECT_EQ(out, "7");
}

/**
 * Verifies that integer multiplication works correctly.
 * Tests basic multiplication operation.
 */
TEST(VM_Arithmetic, Multiplication)
{
    std::string out = runProgram("say 6 * 7;");
    EXPECT_EQ(out, "42");
}

/**
 * Verifies that integer division works correctly.
 * Tests basic division operation.
 */
TEST(VM_Arithmetic, Division)
{
    std::string out = runProgram("say 20 / 4;");
    EXPECT_EQ(out, "5");
}

/**
 * Verifies that mixed arithmetic expressions evaluate correctly.
 * Tests operator precedence with parentheses.
 */
TEST(VM_Arithmetic, MixedExpression)
{
    std::string out = runProgram("say (2 + 3) * 4;");
    EXPECT_EQ(out, "20");
}

/**
 * Verifies that unary negation works correctly.
 * Tests the unary minus operator on integer literals.
 */
TEST(VM_Arithmetic, Negation)
{
    std::string out = runProgram("say -5;");
    EXPECT_EQ(out, "-5");
}

/**
 * Verifies correct operator precedence for multiplication and addition.
 * Multiplication should be evaluated before addition.
 */
TEST(VM_Arithmetic, PrecedenceMultiplyAdd)
{
    std::string out = runProgram("say 2 + 3 * 4;");
    EXPECT_EQ(out, "14");
}

/**
 * Verifies correct operator precedence for division and subtraction.
 * Division should be evaluated before subtraction.
 */
TEST(VM_Arithmetic, PrecedenceDivideSubtract)
{
    std::string out = runProgram("say 20 - 10 / 2;");
    EXPECT_EQ(out, "15");
}

/**
 * Verifies that chained addition operations work correctly.
 * Tests left-to-right associativity of addition.
 */
TEST(VM_Arithmetic, ChainedAddition)
{
    std::string out = runProgram("say 1 + 2 + 3 + 4;");
    EXPECT_EQ(out, "10");
}

/**
 * Verifies that chained multiplication operations work correctly.
 * Tests left-to-right associativity of multiplication.
 */
TEST(VM_Arithmetic, ChainedMultiplication)
{
    std::string out = runProgram("say 2 * 3 * 4;");
    EXPECT_EQ(out, "24");
}

/**
 * Verifies that subtraction with negative results works correctly.
 * Tests handling of negative results from arithmetic operations.
 */
TEST(VM_Arithmetic, SubtractionNegativeResult)
{
    std::string out = runProgram("say 5 - 10;");
    EXPECT_EQ(out, "-5");
}

/**
 * Verifies that negation of expressions works correctly.
 * Tests unary minus applied to a parenthesized expression.
 */
TEST(VM_Arithmetic, NegationOfExpression)
{
    std::string out = runProgram("say -(3 + 4);");
    EXPECT_EQ(out, "-7");
}

/**
 * Verifies that double negation works correctly.
 * Tests unary minus applied twice.
 */
TEST(VM_Arithmetic, DoubleNegation)
{
    std::string out = runProgram("say - -5;");
    EXPECT_EQ(out, "5");
}

/**
 * Verifies complex nested arithmetic expressions.
 * Tests deeply nested parentheses and mixed operators.
 */
TEST(VM_Arithmetic, ComplexNested)
{
    std::string out = runProgram("say ((10 + 5) * 2 - 6) / 3;");
    EXPECT_EQ(out, "8");
}

/**
 * Verifies arithmetic operations with zero.
 * Tests addition with zero identity element.
 */
TEST(VM_Arithmetic, AddZero)
{
    std::string out = runProgram("say 42 + 0;");
    EXPECT_EQ(out, "42");
}

/**
 * Verifies multiplication by zero.
 * Tests zero property of multiplication.
 */
TEST(VM_Arithmetic, MultiplyByZero)
{
    std::string out = runProgram("say 42 * 0;");
    EXPECT_EQ(out, "0");
}

/**
 * Verifies multiplication by one.
 * Tests identity property of multiplication.
 */
TEST(VM_Arithmetic, MultiplyByOne)
{
    std::string out = runProgram("say 42 * 1;");
    EXPECT_EQ(out, "42");
}
/**
 * Verifies integer equality comparison.
 * Tests == operator with equal integer values.
 */
TEST(VM_Comparison, IntegerEquality)
{
    std::string out = runProgram("say 3 == 3;");
    EXPECT_EQ(out, "affirmative");
}

/**
 * Verifies integer inequality comparison.
 * Tests == operator with different integer values.
 */
TEST(VM_Comparison, IntegerInequality)
{
    std::string out = runProgram("say 3 == 5;");
    EXPECT_EQ(out, "negative");
}

/**
 * Verifies integer not-equal comparison.
 * Tests != operator with different values.
 */
TEST(VM_Comparison, IntegerNotEqual)
{
    std::string out = runProgram("say 3 != 5;");
    EXPECT_EQ(out, "affirmative");
}

/**
 * Verifies integer not-equal comparison with equal values.
 * Tests != operator with same values.
 */
TEST(VM_Comparison, IntegerNotEqualSame)
{
    std::string out = runProgram("say 3 != 3;");
    EXPECT_EQ(out, "negative");
}

/**
 * Verifies less-than comparison.
 * Tests < operator with true condition.
 */
TEST(VM_Comparison, LessThanTrue)
{
    std::string out = runProgram("say 3 < 5;");
    EXPECT_EQ(out, "affirmative");
}

/**
 * Verifies less-than comparison false case.
 * Tests < operator with false condition.
 */
TEST(VM_Comparison, LessThanFalse)
{
    std::string out = runProgram("say 5 < 3;");
    EXPECT_EQ(out, "negative");
}

/**
 * Verifies less-than-or-equal comparison.
 * Tests <= operator with less-than case.
 */
TEST(VM_Comparison, LessThanOrEqual)
{
    std::string out = runProgram("say 3 <= 5;");
    EXPECT_EQ(out, "affirmative");
}

/**
 * Verifies less-than-or-equal comparison with equal values.
 * Tests <= operator with equal case.
 */
TEST(VM_Comparison, LessThanOrEqualSame)
{
    std::string out = runProgram("say 5 <= 5;");
    EXPECT_EQ(out, "affirmative");
}

/**
 * Verifies greater-than comparison.
 * Tests > operator with true condition.
 */
TEST(VM_Comparison, GreaterThanTrue)
{
    std::string out = runProgram("say 5 > 3;");
    EXPECT_EQ(out, "affirmative");
}

/**
 * Verifies greater-than comparison false case.
 * Tests > operator with false condition.
 */
TEST(VM_Comparison, GreaterThanFalse)
{
    std::string out = runProgram("say 3 > 5;");
    EXPECT_EQ(out, "negative");
}

/**
 * Verifies greater-than-or-equal comparison.
 * Tests >= operator with greater-than case.
 */
TEST(VM_Comparison, GreaterThanOrEqual)
{
    std::string out = runProgram("say 5 >= 3;");
    EXPECT_EQ(out, "affirmative");
}

/**
 * Verifies greater-than-or-equal comparison with equal values.
 * Tests >= operator with equal case.
 */
TEST(VM_Comparison, GreaterThanOrEqualSame)
{
    std::string out = runProgram("say 5 >= 5;");
    EXPECT_EQ(out, "affirmative");
}

/**
 * Verifies string equality comparison with equal strings.
 * Tests == operator on matching string literals.
 */
TEST(VM_Comparison, StringEquality)
{
    std::string out = runProgram(R"(say "hi" == "hi";)");
    EXPECT_EQ(out, "affirmative");
}

/**
 * Verifies string inequality comparison with different strings.
 * Tests != operator on non-matching string literals.
 */
TEST(VM_Comparison, StringInequality)
{
    std::string out = runProgram(R"(say "hi" != "bye";)");
    EXPECT_EQ(out, "affirmative");
}

/**
 * Verifies string inequality comparison with same strings.
 * Tests != operator on matching string literals.
 */
TEST(VM_Comparison, StringInequalitySame)
{
    std::string out = runProgram(R"(say "hello" != "hello";)");
    EXPECT_EQ(out, "negative");
}

/**
 * Verifies boolean equality comparison.
 * Tests == operator with boolean values.
 */
TEST(VM_Comparison, BooleanEquality)
{
    std::string out = runProgram("say affirmative == affirmative;");
    EXPECT_EQ(out, "affirmative");
}

/**
 * Verifies boolean inequality comparison.
 * Tests != operator with different boolean values.
 */
TEST(VM_Comparison, BooleanInequality)
{
    std::string out = runProgram("say affirmative != negative;");
    EXPECT_EQ(out, "affirmative");
}

/**
 * Verifies comparison with negative numbers.
 * Tests < operator with negative integers.
 */
TEST(VM_Comparison, NegativeComparison)
{
    std::string out = runProgram("say -5 < -3;");
    EXPECT_EQ(out, "affirmative");
}

/**
 * Verifies comparison with zero.
 * Tests comparison operations involving zero.
 */
TEST(VM_Comparison, ComparisonWithZero)
{
    std::string out = runProgram("say 0 > -1;");
    EXPECT_EQ(out, "affirmative");
}

/**
 * Verifies chained comparisons are evaluated correctly.
 * Tests multiple comparison operations in sequence.
 */
TEST(VM_Comparison, ChainedComparison)
{
    std::string out = runProgram("say (3 < 5) == (2 < 4);");
    EXPECT_EQ(out, "affirmative");
}

/**
 * Verifies logical NOT operator on affirmative.
 * Tests negation of true boolean value.
 */
TEST(VM_Logical, NotAffirmative)
{
    std::string out = runProgram("say not affirmative;");
    EXPECT_EQ(out, "negative");
}

/**
 * Verifies logical NOT operator on negative.
 * Tests negation of false boolean value.
 */
TEST(VM_Logical, NotNegative)
{
    std::string out = runProgram("say not negative;");
    EXPECT_EQ(out, "affirmative");
}

/**
 * Verifies double negation returns original value.
 * Tests applying NOT operator twice.
 */
TEST(VM_Logical, DoubleNot)
{
    std::string out = runProgram("say not not affirmative;");
    EXPECT_EQ(out, "affirmative");
}

/**
 * Verifies NOT operator on comparison result.
 * Tests negation of a comparison expression.
 */
TEST(VM_Logical, NotComparison)
{
    std::string out = runProgram("say not (3 < 5);");
    EXPECT_EQ(out, "negative");
}

/**
 * Verifies NOT operator on equality result.
 * Tests negation of equality comparison.
 */
TEST(VM_Logical, NotEquality)
{
    std::string out = runProgram("say not (3 == 5);");
    EXPECT_EQ(out, "affirmative");
}

/**
 * Verifies complex logical expression with NOT and comparisons.
 * Tests combination of logical and comparison operators.
 */
TEST(VM_Logical, ComplexNotExpression)
{
    std::string out = runProgram("say not (5 > 3) == (2 < 1);");
    EXPECT_EQ(out, "affirmative");
}

/**
 * Verifies basic variable declaration and usage.
 * Tests summon keyword and variable reference.
 */
TEST(VM_Variables, SummonAndUse)
{
    std::string out = runProgram(R"(
        summon x = 10;
        say x;
    )");
    EXPECT_EQ(out, "10");
}

/**
 * Verifies variable shadowing in nested scope.
 * Tests that inner scope can shadow outer variable.
 */
TEST(VM_Variables, Shadowing)
{
    std::string out = runProgram(R"(
        summon x = 1;
        {
            summon x = 2;
            say x;
        }
        say x;
    )");
    EXPECT_EQ(out, "21");
}

/**
 * Verifies variable reassignment works correctly.
 * Tests updating a variable's value.
 */
TEST(VM_Variables, Reassignment)
{
    std::string out = runProgram(R"(
        summon x = 5;
        x = 10;
        say x;
    )");
    EXPECT_EQ(out, "10");
}

/**
 * Verifies multiple variable declarations.
 * Tests declaring and using multiple variables.
 */
TEST(VM_Variables, MultipleVariables)
{
    std::string out = runProgram(R"(
        summon x = 1;
        summon y = 2;
        summon z = 3;
        say x + y + z;
    )");
    EXPECT_EQ(out, "6");
}

/**
 * Verifies variable assignment from expression.
 * Tests assigning result of expression to variable.
 */
TEST(VM_Variables, AssignmentFromExpression)
{
    std::string out = runProgram(R"(
        summon x = 5;
        summon y = x * 2 + 3;
        say y;
    )");
    EXPECT_EQ(out, "13");
}

/**
 * Verifies variable assignment using other variables.
 * Tests cross-variable assignment.
 */
TEST(VM_Variables, AssignmentFromVariable)
{
    std::string out = runProgram(R"(
        summon x = 10;
        summon y = x;
        say y;
    )");
    EXPECT_EQ(out, "10");
}

/**
 * Verifies string variable declaration and usage.
 * Tests variables holding string values.
 */
TEST(VM_Variables, StringVariable)
{
    std::string out = runProgram(R"(
        summon name = "Ambrose";
        say name;
    )");
    EXPECT_EQ(out, "Ambrose");
}

/**
 * Verifies boolean variable declaration and usage.
 * Tests variables holding boolean values.
 */
TEST(VM_Variables, BooleanVariable)
{
    std::string out = runProgram(R"(
        summon flag = affirmative;
        say flag;
    )");
    EXPECT_EQ(out, "affirmative");
}

/**
 * Verifies variable used in arithmetic expression.
 * Tests variables as operands in calculations.
 */
TEST(VM_Variables, InArithmetic)
{
    std::string out = runProgram(R"(
        summon a = 5;
        summon b = 3;
        say a * b + a - b;
    )");
    EXPECT_EQ(out, "17");
}

/**
 * Verifies variable can be updated multiple times.
 * Tests sequential reassignment of same variable.
 */
TEST(VM_Variables, MultipleReassignments)
{
    std::string out = runProgram(R"(
        summon x = 1;
        x = 2;
        x = 3;
        x = 4;
        say x;
    )");
    EXPECT_EQ(out, "4");
}

/**
 * Verifies deeply nested shadowing works correctly.
 * Tests multiple levels of variable shadowing.
 */
TEST(VM_Variables, DeeplyShadowing)
{
    std::string out = runProgram(R"(
        summon x = 1;
        {
            summon x = 2;
            {
                summon x = 3;
                say x;
            }
            say x;
        }
        say x;
    )");
    EXPECT_EQ(out, "321");
}

/**
 * Verifies block scope isolation.
 * Tests that variables in sibling blocks don't interfere.
 */
TEST(VM_Variables, BlockScopeIsolation)
{
    std::string out = runProgram(R"(
        summon x = 1;
        {
            summon y = 2;
            say x;
        }
        say x;
    )");
    EXPECT_EQ(out, "11");
}

/**
 * Verifies self-referential assignment works correctly.
 * Tests variable updating itself using its own value.
 */
TEST(VM_Variables, SelfReferentialAssignment)
{
    std::string out = runProgram(R"(
        summon x = 10;
        x = x + 5;
        say x;
    )");
    EXPECT_EQ(out, "15");
}

/**
 * Verifies if-else chain with multiple branches.
 * Tests otherwise should syntax with multiple conditions.
 */
TEST(VM_ControlFlow, IfChain)
{
    std::string out = runProgram(R"(
        summon x = 3;
        should (x < 2) {
            say "low";
        } otherwise should (x < 4) {
            say "mid";
        } otherwise {
            say "high";
        }
    )");
    EXPECT_EQ(out, "mid");
}

/**
 * Verifies simple if statement without else.
 * Tests single conditional branch execution.
 */
TEST(VM_ControlFlow, SimpleIf)
{
    std::string out = runProgram(R"(
        summon x = 5;
        should (x > 3) {
            say "yes";
        }
    )");
    EXPECT_EQ(out, "yes");
}

/**
 * Verifies if statement skips body when condition is false.
 * Tests that false condition doesn't execute if body.
 */
TEST(VM_ControlFlow, IfNotExecuted)
{
    std::string out = runProgram(R"(
        summon x = 1;
        should (x > 3) {
            say "yes";
        }
        say "done";
    )");
    EXPECT_EQ(out, "done");
}

/**
 * Verifies if-else statement executes else branch.
 * Tests else branch when if condition is false.
 */
TEST(VM_ControlFlow, IfElse)
{
    std::string out = runProgram(R"(
        summon x = 1;
        should (x > 3) {
            say "big";
        } otherwise {
            say "small";
        }
    )");
    EXPECT_EQ(out, "small");
}

/**
 * Verifies nested if statements work correctly.
 * Tests conditional statements within conditional bodies.
 */
TEST(VM_ControlFlow, NestedIf)
{
    std::string out = runProgram(R"(
        summon x = 5;
        should (x > 3) {
            should (x > 4) {
                say "very big";
            }
        }
    )");
    EXPECT_EQ(out, "very big");
}

/**
 * Verifies if condition with boolean variable.
 * Tests using boolean variable directly in condition.
 */
TEST(VM_ControlFlow, IfWithBooleanVariable)
{
    std::string out = runProgram(R"(
        summon flag = affirmative;
        should (flag) {
            say "flag is set";
        }
    )");
    EXPECT_EQ(out, "flag is set");
}

/**
 * Verifies if condition with NOT operator.
 * Tests negated boolean in conditional.
 */
TEST(VM_ControlFlow, IfWithNot)
{
    std::string out = runProgram(R"(
        summon flag = negative;
        should (not flag) {
            say "flag is not set";
        }
    )");
    EXPECT_EQ(out, "flag is not set");
}

/**
 * Verifies if statement with complex condition.
 * Tests multiple comparison operators in condition.
 */
TEST(VM_ControlFlow, IfWithComplexCondition)
{
    std::string out = runProgram(R"(
        summon x = 5;
        should ((x > 3) == (x < 10)) {
            say "in range";
        }
    )");
    EXPECT_EQ(out, "in range");
}

/**
 * Verifies long if-else chain reaches final else.
 * Tests multiple otherwise should branches.
 */
TEST(VM_ControlFlow, LongIfChain)
{
    std::string out = runProgram(R"(
        summon x = 100;
        should (x < 10) {
            say "A";
        } otherwise should (x < 20) {
            say "B";
        } otherwise should (x < 30) {
            say "C";
        } otherwise should (x < 40) {
            say "D";
        } otherwise {
            say "E";
        }
    )");
    EXPECT_EQ(out, "E");
}

/**
 * Verifies basic while loop functionality.
 * Tests aslongas loop with simple counter.
 */
TEST(VM_ControlFlow, WhileLoop)
{
    std::string out = runProgram(R"(
        summon x = 0;
        aslongas (x < 3) {
            say x;
            x = x + 1;
        }
    )");
    EXPECT_EQ(out, "012");
}

/**
 * Verifies while loop that never executes.
 * Tests loop with initially false condition.
 */
TEST(VM_ControlFlow, WhileLoopNeverExecutes)
{
    std::string out = runProgram(R"(
        summon x = 10;
        aslongas (x < 5) {
            say "never";
        }
        say "done";
    )");
    EXPECT_EQ(out, "done");
}

/**
 * Verifies while loop with countdown.
 * Tests loop decrementing a counter.
 */
TEST(VM_ControlFlow, WhileLoopCountdown)
{
    std::string out = runProgram(R"(
        summon x = 3;
        aslongas (x > 0) {
            say x;
            x = x - 1;
        }
    )");
    EXPECT_EQ(out, "321");
}

/**
 * Verifies nested while loops work correctly.
 * Tests loop within loop execution.
 */
TEST(VM_ControlFlow, NestedWhileLoops)
{
    std::string out = runProgram(R"(
        summon i = 0;
        aslongas (i < 2) {
            summon j = 0;
            aslongas (j < 2) {
                say i * 2 + j;
                j = j + 1;
            }
            i = i + 1;
        }
    )");
    EXPECT_EQ(out, "0123");
}

/**
 * Verifies while loop with conditional inside.
 * Tests combining loops and conditionals.
 */
TEST(VM_ControlFlow, WhileWithIf)
{
    std::string out = runProgram(R"(
        summon x = 0;
        aslongas (x < 5) {
            should (x == 2) {
                say "two";
            }
            x = x + 1;
        }
    )");
    EXPECT_EQ(out, "two");
}

/**
 * Verifies loop with multiple variable updates.
 * Tests managing multiple loop variables.
 */
TEST(VM_ControlFlow, WhileMultipleUpdates)
{
    std::string out = runProgram(R"(
        summon x = 0;
        summon y = 10;
        aslongas (x < 3) {
            say x + y;
            x = x + 1;
            y = y - 2;
        }
    )");
    EXPECT_EQ(out, "1098");
}

/**
 * Verifies while loop can iterate many times.
 * Tests loop with higher iteration count.
 */
TEST(VM_ControlFlow, WhileManyIterations)
{
    std::string out = runProgram(R"(
        summon x = 0;
        summon sum = 0;
        aslongas (x < 5) {
            sum = sum + x;
            x = x + 1;
        }
        say sum;
    )");
    EXPECT_EQ(out, "10");
}

/**
 * Verifies printing of string with spaces.
 * Tests string literals containing whitespace.
 */
TEST(VM_Strings, StringWithSpaces)
{
    std::string out = runProgram(R"(say "hello world";)");
    EXPECT_EQ(out, "hello world");
}

/**
 * Verifies string comparison with empty string.
 * Tests equality check with empty string literal.
 */
TEST(VM_Strings, EmptyStringComparison)
{
    std::string out = runProgram(R"(say "" == "";)");
    EXPECT_EQ(out, "affirmative");
}

/**
 * Verifies string variable reassignment.
 * Tests updating string variable value.
 */
TEST(VM_Strings, StringVariableReassignment)
{
    std::string out = runProgram(R"(
        summon name = "Alice";
        name = "Bob";
        say name;
    )");
    EXPECT_EQ(out, "Bob");
}

/**
 * Verifies string comparison in conditional.
 * Tests using string equality in if statement.
 */
TEST(VM_Strings, StringInConditional)
{
    std::string out = runProgram(R"(
        summon name = "Alice";
        should (name == "Alice") {
            say "correct";
        }
    )");
    EXPECT_EQ(out, "correct");
}

/**
 * Verifies strings with numeric characters.
 * Tests string literals containing digits.
 */
TEST(VM_Strings, StringWithNumbers)
{
    std::string out = runProgram(R"(say "Test123";)");
    EXPECT_EQ(out, "Test123");
}

/**
 * Verifies complex program with variables, loops, and conditionals.
 * Integration test combining multiple language features.
 */
TEST(VM_Integration, FizzBuzzStyle)
{
    std::string out = runProgram(R"(
        summon i = 1;
        aslongas (i <= 5) {
            should (i == 3) {
                say "Fizz";
            } otherwise should (i == 5) {
                say "Buzz";
            } otherwise {
                say i;
            }
            i = i + 1;
        }
    )");
    EXPECT_EQ(out, "12Fizz4Buzz");
}

/**
 * Verifies factorial calculation using loop.
 * Tests iterative algorithm implementation.
 */
TEST(VM_Integration, Factorial)
{
    std::string out = runProgram(R"(
        summon n = 5;
        summon result = 1;
        summon i = 1;
        aslongas (i <= n) {
            result = result * i;
            i = i + 1;
        }
        say result;
    )");
    EXPECT_EQ(out, "120");
}

/**
 * Verifies sum of evens algorithm.
 * Tests conditional accumulation in loop.
 */
TEST(VM_Integration, SumOfEvens)
{
    std::string out = runProgram(R"(
        summon sum = 0;
        summon i = 0;
        aslongas (i <= 10) {
            should ((i / 2) * 2 == i) {
                sum = sum + i;
            }
            i = i + 1;
        }
        say sum;
    )");
    EXPECT_EQ(out, "30");
}

/**
 * Verifies nested scopes with variable shadowing in loops.
 * Tests complex scope interactions.
 */
TEST(VM_Integration, ComplexScopingInLoop)
{
    std::string out = runProgram(R"(
        summon x = 0;
        aslongas (x < 2) {
            summon y = x;
            {
                summon y = 10;
                say y;
            }
            say y;
            x = x + 1;
        }
    )");
    EXPECT_EQ(out, "100101");
}

/**
 * Verifies max finding algorithm.
 * Tests comparison-based logic in loop.
 */
TEST(VM_Integration, FindMaximum)
{
    std::string out = runProgram(R"(
        summon max = 0;
        summon val = 5;
        should (val > max) {
            max = val;
        }
        val = 3;
        should (val > max) {
            max = val;
        }
        val = 8;
        should (val > max) {
            max = val;
        }
        say max;
    )");
    EXPECT_EQ(out, "8");
}

/**
 * Verifies computation with multiple temporary variables.
 * Tests variable management in complex expression.
 */
TEST(VM_Integration, MultipleTemporaries)
{
    std::string out = runProgram(R"(
        summon a = 2;
        summon b = 3;
        summon c = 4;
        summon d = 5;
        summon result = (a + b) * (c - d);
        say result;
    )");
    EXPECT_EQ(out, "-5");
}

/**
 * Verifies deeply nested blocks with many variables.
 * Tests VM stack management with deep nesting.
 */
TEST(VM_Integration, DeeplyNestedBlocks)
{
    std::string out = runProgram(R"(
        summon a = 1;
        {
            summon b = 2;
            {
                summon c = 3;
                {
                    summon d = 4;
                    say a + b + c + d;
                }
            }
        }
    )");
    EXPECT_EQ(out, "10");
}

/**
 * Verifies triangle number calculation.
 * Tests accumulation pattern in loop.
 */
TEST(VM_Integration, TriangleNumbers)
{
    std::string out = runProgram(R"(
        summon n = 4;
        summon sum = 0;
        summon i = 1;
        aslongas (i <= n) {
            sum = sum + i;
            i = i + 1;
        }
        say sum;
    )");
    EXPECT_EQ(out, "10");
}

/**
 * Verifies power calculation using loop.
 * Tests repeated multiplication pattern.
 */
TEST(VM_Integration, PowerCalculation)
{
    std::string out = runProgram(R"(
        summon base = 2;
        summon exp = 4;
        summon result = 1;
        summon i = 0;
        aslongas (i < exp) {
            result = result * base;
            i = i + 1;
        }
        say result;
    )");
    EXPECT_EQ(out, "16");
}

/**
 * Verifies boolean flag toggling in loop.
 * Tests boolean variable manipulation.
 */
TEST(VM_Integration, BooleanToggling)
{
    std::string out = runProgram(R"(
        summon flag = affirmative;
        summon count = 0;
        aslongas (count < 3) {
            should (flag) {
                say "T";
                flag = negative;
            } otherwise {
                say "F";
                flag = affirmative;
            }
            count = count + 1;
        }
    )");
    EXPECT_EQ(out, "TFT");
}

/**
 * Verifies guard pattern with early loop termination logic.
 * Tests conditional logic to simulate break-like behavior.
 */
TEST(VM_Integration, GuardPattern)
{
    std::string out = runProgram(R"(
        summon i = 0;
        summon found = negative;
        aslongas ((i < 10) == (not found)) {
            should (i == 5) {
                found = affirmative;
            }
            say i;
            i = i + 1;
        }
    )");
    EXPECT_EQ(out, "012345");
}

/**
 * Verifies collatz sequence calculation (partial).
 * Tests complex arithmetic and conditional logic in loop.
 */
TEST(VM_Integration, CollatzSequence)
{
    std::string out = runProgram(R"(
        summon n = 10;
        aslongas (n > 1) {
            say n;
            should ((n / 2) * 2 == n) {
                n = n / 2;
            } otherwise {
                n = n * 3 + 1;
            }
        }
        say n;
    )");
    EXPECT_EQ(out, "105168421");
}