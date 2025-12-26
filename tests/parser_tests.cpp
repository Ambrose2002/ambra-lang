// Tests for the parser component of the Ambra language.
// These tests cover expression parsing, including single-token expressions,
// operator precedence and associativity, grouping with parentheses, unary expressions,
// comparison and equality expressions, string interpolation, and error handling.

#include "ast/expr.h"
#include "ast/stmt.h"
#include "parser/parser.h"

#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

/**
 * Compares two expressions for equality.
 * Prints diagnostic messages to std::cerr if the expressions do not match.
 * Intended for use in test assertions only.
 */
bool isEqualExpression(const std::unique_ptr<Expr>& e1, const std::unique_ptr<Expr>& e2)
{
    if (e1 == e2)
        return true;
    if (!e1 || !e2)
    {
        std::cerr << "Expression mismatch: one side is null\n";
        std::cerr << " actual: " << (e1 ? e1->toString() : std::string("<null>")) << "\n";
        std::cerr << " expected: " << (e2 ? e2->toString() : std::string("<null>")) << "\n";
        return false;
    }
    if (!(*e1 == *e2))
    {
        std::cerr << "Expression mismatch:\n";
        std::cerr << " actual:   " << e1->toString() << "\n";
        std::cerr << " expected: " << e2->toString() << "\n";
        return false;
    }
    return true;
}

/**
 * Compares two statements for equality.
 * Prints diagnostic messages to std::cerr if the statements do not match.
 * Intended for use in test assertions only.
 */
bool isEqualStatements(const std::unique_ptr<Stmt>& s1, const std::unique_ptr<Stmt>& s2)
{
    if (s1 == s2)
    {
        return true;
    }

    if (!s1 || !s2)
    {
        std::cerr << "Statement mismatch: one side is null\n";
        std::cerr << " actual: " << (s1 ? s1->toString() : std::string("<null>")) << "\n";
        std::cerr << " expected: " << (s2 ? s2->toString() : std::string("<null>")) << "\n";
        return false;
    }
    if (!(*s1 == *s2))
    {
        std::cerr << "Statement mismatch:\n";
        std::cerr << " actual:   " << s1->toString() << "\n";
        std::cerr << " expected: " << s2->toString() << "\n";
        return false;
    }
    return true;
}

/**
 * @brief Helper function to compare two Program objects and report differences.
 *
 * Compares the actual and expected programs for equality. If they don't match,
 * prints detailed diagnostic information to stderr including the string representations
 * of both programs to aid in debugging test failures.
 *
 * @param actual The actual program result from parsing
 * @param expected The expected program for comparison
 * @return true if both programs are equal, false otherwise
 */
static bool isEqualProgram(const Program& actual, const Program& expected)
{
    if (!(actual == expected))
    {
        std::cerr << "Program mismatch:\n";
        std::cerr << " actual:\n" << actual.toString() << "\n";
        std::cerr << " expected:\n" << expected.toString() << "\n";
        return false;
    }
    return true;
}

// === Single-token expressions ===

// Tests parsing of integer literals.
TEST(SingleTokenExpression, IntLiteral)
{
    std::vector<Token> tokens = {
        Token("40", INTEGER, 42, 1, 1),
        Token("", EOF_TOKEN, std::monostate{}, 1, 3),
    };

    std::unique_ptr<Expr> expected = std::make_unique<IntLiteralExpr>(42, 1, 1);

    Parser parser(tokens);

    auto actual = parser.parseExpression();

    ASSERT_TRUE(isEqualExpression(actual, expected));
}

// Tests parsing of boolean literals.
TEST(SingleTokenExpression, BoolLiteral)
{
    std::vector<Token> tokens = {Token("affirmative", BOOL, true, 1, 1),
                                 Token("", EOF_TOKEN, std::monostate{}, 1, 12)};

    std::unique_ptr<Expr> expected = std::make_unique<BoolLiteralExpr>(true, 1, 1);

    Parser parser(tokens);

    auto actual = parser.parseExpression();
    ASSERT_TRUE(isEqualExpression(actual, expected));
}

// Tests parsing of identifiers.
TEST(SingleTokenExpression, Identifer)
{
    std::vector<Token> tokens = {Token("x1", IDENTIFIER, std::monostate{}, 1, 1),
                                 Token("", EOF_TOKEN, std::monostate{}, 1, 3)};

    std::unique_ptr<Expr> expected = std::make_unique<IdentifierExpr>("x1", 1, 1);

    Parser parser(tokens);
    auto   actual = parser.parseExpression();
    ASSERT_TRUE(isEqualExpression(actual, expected));
}

// Tests parsing of string literals.
TEST(SingleTokenExpression, String)
{
    std::vector<Token> tokens = {Token("\"hello\"", STRING, "hello", 1, 1),
                                 Token("", EOF_TOKEN, std::monostate{}, 1, 5)};

    StringPart part;

    Token strToken("\"hello\"", STRING, "hello", 1, 1);
    part.kind = StringPart::TEXT;
    part.text = std::get<std::string>(strToken.getValue());

    std::vector<StringPart> parts;
    parts.push_back(std::move(part));
    std::unique_ptr<Expr> expected = std::make_unique<StringExpr>(std::move(parts), 1, 1);

    Parser parser(tokens);
    auto   actual = parser.parseExpression();
    ASSERT_TRUE(isEqualExpression(actual, expected));
}

// === Operator precedence and associativity ===

// Tests correct precedence of addition and multiplication.
TEST(ExpressionPrecedence, AdditionAndMultiplication)
{
    std::vector<Token> tokens = {
        Token("1", INTEGER, 1, 1, 1), Token("+", PLUS, std::monostate{}, 1, 3),
        Token("2", INTEGER, 2, 1, 5), Token("*", STAR, std::monostate{}, 1, 7),
        Token("3", INTEGER, 3, 1, 9), Token("", EOF_TOKEN, std::monostate{}, 1, 10),
    };

    Parser parser(tokens);
    auto   actual = parser.parseExpression();

    std::unique_ptr<Expr> expected = std::make_unique<BinaryExpr>(
        std::make_unique<IntLiteralExpr>(1, 1, 1), Add,
        std::make_unique<BinaryExpr>(std::make_unique<IntLiteralExpr>(2, 1, 5), Multiply,
                                     std::make_unique<IntLiteralExpr>(3, 1, 9), 1, 7),
        1, 3);

    ASSERT_TRUE(isEqualExpression(actual, expected));
}

// Tests left-associativity of subtraction.
TEST(ExpressionAssociativity, LeftAssociativeSubtraction)
{
    std::vector<Token> tokens = {
        Token("10", INTEGER, 10, 1, 1), Token("-", MINUS, std::monostate{}, 1, 4),
        Token("5", INTEGER, 5, 1, 6),   Token("-", MINUS, std::monostate{}, 1, 8),
        Token("2", INTEGER, 2, 1, 10),  Token("", EOF_TOKEN, std::monostate{}, 1, 11),
    };

    Parser parser(tokens);
    auto   actual = parser.parseExpression();

    std::unique_ptr<Expr> expected = std::make_unique<BinaryExpr>(
        std::make_unique<BinaryExpr>(std::make_unique<IntLiteralExpr>(10, 1, 1), Subtract,
                                     std::make_unique<IntLiteralExpr>(5, 1, 6), 1, 4),
        Subtract, std::make_unique<IntLiteralExpr>(2, 1, 10), 1, 8);

    ASSERT_TRUE(isEqualExpression(actual, expected));
}

// === Grouping / parentheses ===

// Tests that parentheses override operator precedence.
TEST(ExpressionGrouping, ParenthesesOverridePrecedence)
{
    std::vector<Token> tokens = {
        Token("(", LEFT_PAREN, std::monostate{}, 1, 1),
        Token("1", INTEGER, 1, 1, 2),
        Token("+", PLUS, std::monostate{}, 1, 4),
        Token("2", INTEGER, 2, 1, 6),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 7),
        Token("*", STAR, std::monostate{}, 1, 9),
        Token("3", INTEGER, 3, 1, 11),
        Token("", EOF_TOKEN, std::monostate{}, 1, 12),
    };

    Parser parser(tokens);
    auto   actual = parser.parseExpression();

    std::unique_ptr<Expr> expected = std::make_unique<BinaryExpr>(
        std::make_unique<GroupingExpr>(
            std::make_unique<BinaryExpr>(std::make_unique<IntLiteralExpr>(1, 1, 2), Add,
                                         std::make_unique<IntLiteralExpr>(2, 1, 6), 1, 4),
            1, 1),
        Multiply, std::make_unique<IntLiteralExpr>(3, 1, 11), 1, 9);

    ASSERT_TRUE(isEqualExpression(actual, expected));
}

// === Unary expressions ===

// Tests parsing of logical NOT unary operator.
TEST(ExpressionUnary, LogicalNot)
{
    std::vector<Token> tokens = {
        Token("not", NOT, std::monostate{}, 1, 1),
        Token("affirmative", BOOL, true, 1, 5),
        Token("", EOF_TOKEN, std::monostate{}, 1, 17),
    };

    Parser parser(tokens);
    auto   actual = parser.parseExpression();

    std::unique_ptr<Expr> expected = std::make_unique<UnaryExpr>(
        LogicalNot, std::make_unique<BoolLiteralExpr>(true, 1, 5), 1, 1);

    ASSERT_TRUE(isEqualExpression(actual, expected));
}

// Tests that unary operators bind tighter than multiplication.
TEST(ExpressionUnaryBinary, UnaryBindsTighterThanMultiplication)
{
    std::vector<Token> tokens = {
        Token("-", MINUS, std::monostate{}, 1, 1),    Token("1", INTEGER, 1, 1, 2),
        Token("*", STAR, std::monostate{}, 1, 4),     Token("2", INTEGER, 2, 1, 6),
        Token("", EOF_TOKEN, std::monostate{}, 1, 7),
    };

    Parser parser(tokens);
    auto   actual = parser.parseExpression();

    std::unique_ptr<Expr> expected = std::make_unique<BinaryExpr>(
        std::make_unique<UnaryExpr>(ArithmeticNegate, std::make_unique<IntLiteralExpr>(1, 1, 2), 1,
                                    1),
        Multiply, std::make_unique<IntLiteralExpr>(2, 1, 6), 1, 4);

    ASSERT_TRUE(isEqualExpression(actual, expected));
}

// Tests parsing of double logical NOT unary operators.
TEST(ExpressionUnary, DoubleLogicalNot)
{
    std::vector<Token> tokens = {
        Token("not", NOT, std::monostate{}, 1, 1),
        Token("not", NOT, std::monostate{}, 1, 5),
        Token("affirmative", BOOL, true, 1, 9),
        Token("", EOF_TOKEN, std::monostate{}, 1, 21),
    };

    Parser parser(tokens);
    auto   actual = parser.parseExpression();

    std::unique_ptr<Expr> expected = std::make_unique<UnaryExpr>(
        LogicalNot,
        std::make_unique<UnaryExpr>(LogicalNot, std::make_unique<BoolLiteralExpr>(true, 1, 9), 1,
                                    5),
        1, 1);

    ASSERT_TRUE(isEqualExpression(actual, expected));
}

// === Comparison and equality expressions ===

// Tests parsing of greater-than comparison.
TEST(ExpressionComparison, GreaterThan)
{
    std::vector<Token> tokens = {
        Token("5", INTEGER, 5, 1, 1),
        Token(">", GREATER, std::monostate{}, 1, 3),
        Token("3", INTEGER, 3, 1, 5),
        Token("", EOF_TOKEN, std::monostate{}, 1, 6),
    };

    Parser parser(tokens);
    auto   actual = parser.parseExpression();

    std::unique_ptr<Expr> expected =
        std::make_unique<BinaryExpr>(std::make_unique<IntLiteralExpr>(5, 1, 1), Greater,
                                     std::make_unique<IntLiteralExpr>(3, 1, 5), 1, 3);

    ASSERT_TRUE(isEqualExpression(actual, expected));
}

// Tests chaining of less-than comparisons.
TEST(ExpressionComparison, ChainedLessThan)
{
    std::vector<Token> tokens = {
        Token("1", INTEGER, 1, 1, 1), Token("<", LESS, std::monostate{}, 1, 3),
        Token("2", INTEGER, 2, 1, 5), Token("<", LESS, std::monostate{}, 1, 7),
        Token("3", INTEGER, 3, 1, 9), Token("", EOF_TOKEN, std::monostate{}, 1, 10),
    };

    Parser parser(tokens);
    auto   actual = parser.parseExpression();

    std::unique_ptr<Expr> expected = std::make_unique<BinaryExpr>(
        std::make_unique<BinaryExpr>(std::make_unique<IntLiteralExpr>(1, 1, 1), Less,
                                     std::make_unique<IntLiteralExpr>(2, 1, 5), 1, 3),
        Less, std::make_unique<IntLiteralExpr>(3, 1, 9), 1, 7);

    ASSERT_TRUE(isEqualExpression(actual, expected));
}

// === String and interpolation expressions ===

// Tests parsing of single interpolation in a string.
TEST(ExpressionString, InterpolatedSingle)
{
    std::vector<Token> tokens = {
        Token("\"hello \"", STRING, "hello ", 1, 1),
        Token("{", INTERP_START, std::monostate{}, 1, 9),
        Token("x", IDENTIFIER, std::monostate{}, 1, 10),
        Token("}", INTERP_END, std::monostate{}, 1, 11),
        Token("\"\"", STRING, "", 1, 12),
        Token("", EOF_TOKEN, std::monostate{}, 1, 14),
    };

    Parser parser(tokens);
    auto   actual = parser.parseExpression();

    std::vector<StringPart> parts;
    StringPart              textPart;
    textPart.kind = StringPart::TEXT;
    textPart.text = "hello ";
    parts.push_back(std::move(textPart));

    StringPart exprPart;
    exprPart.kind = StringPart::EXPR;
    exprPart.expr = std::make_unique<IdentifierExpr>("x", 1, 10);
    parts.push_back(std::move(exprPart));

    // Parser appends the trailing string chunk (empty string literal here).
    StringPart trailingText;
    trailingText.kind = StringPart::TEXT;
    trailingText.text = "";
    parts.push_back(std::move(trailingText));

    std::unique_ptr<Expr> expected = std::make_unique<StringExpr>(std::move(parts), 1, 1);

    ASSERT_TRUE(isEqualExpression(actual, expected));
}

// === Error-handling tests ===

// Tests error detection for missing right parenthesis.
TEST(ExpressionErrors, MissingRightParen)
{
    std::vector<Token> tokens = {
        Token("(", LEFT_PAREN, std::monostate{}, 1, 1), Token("1", INTEGER, 1, 1, 2),
        Token("+", PLUS, std::monostate{}, 1, 4),       Token("2", INTEGER, 2, 1, 6),
        Token("", EOF_TOKEN, std::monostate{}, 1, 7),
    };

    Parser parser(tokens);
    auto   result = parser.parseExpression();

    ASSERT_TRUE(parser.hadError());
    ASSERT_EQ(result, nullptr);
}

// Tests error detection for missing right-hand operand.
TEST(ExpressionErrors, MissingRightHandOperand)
{
    std::vector<Token> tokens = {
        Token("1", INTEGER, 1, 1, 1),
        Token("+", PLUS, std::monostate{}, 1, 3),
        Token("", EOF_TOKEN, std::monostate{}, 1, 4),
    };

    Parser parser(tokens);
    auto   result = parser.parseExpression();

    ASSERT_TRUE(parser.hadError());
    ASSERT_EQ(result, nullptr);
}

// Tests error detection for unterminated string interpolation.
TEST(ExpressionErrors, UnterminatedInterpolation)
{
    std::vector<Token> tokens = {
        Token("\"hello \"", STRING, "hello ", 1, 1),
        Token("{", INTERP_START, std::monostate{}, 1, 9),
        Token("x", IDENTIFIER, std::monostate{}, 1, 10),
        // Missing INTERP_END
        Token("", EOF_TOKEN, std::monostate{}, 1, 11),
    };

    Parser parser(tokens);
    auto   result = parser.parseExpression();

    ASSERT_TRUE(parser.hadError());
    ASSERT_EQ(result, nullptr);
}

// Tests unary operator applied to a grouped expression.
TEST(ExpressionUnary, UnaryOnGroupedExpression)
{
    std::vector<Token> tokens = {
        Token("-", MINUS, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 2),
        Token("1", INTEGER, 1, 1, 3),
        Token("+", PLUS, std::monostate{}, 1, 5),
        Token("2", INTEGER, 2, 1, 7),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 8),
        Token("", EOF_TOKEN, std::monostate{}, 1, 9),
    };

    Parser parser(tokens);
    auto   actual = parser.parseExpression();

    std::unique_ptr<Expr> expected = std::make_unique<UnaryExpr>(
        ArithmeticNegate,
        std::make_unique<GroupingExpr>(
            std::make_unique<BinaryExpr>(std::make_unique<IntLiteralExpr>(1, 1, 3), Add,
                                         std::make_unique<IntLiteralExpr>(2, 1, 7), 1, 5),
            1, 2),
        1, 1);

    ASSERT_TRUE(isEqualExpression(actual, expected));
}

// Tests parsing of equality operator (==).
TEST(ExpressionEquality, EqualEqual)
{
    std::vector<Token> tokens = {
        Token("1", INTEGER, 1, 1, 1),
        Token("==", EQUAL_EQUAL, std::monostate{}, 1, 3),
        Token("2", INTEGER, 2, 1, 6),
        Token("", EOF_TOKEN, std::monostate{}, 1, 7),
    };

    Parser parser(tokens);
    auto   actual = parser.parseExpression();

    std::unique_ptr<Expr> expected =
        std::make_unique<BinaryExpr>(std::make_unique<IntLiteralExpr>(1, 1, 1), EqualEqual,
                                     std::make_unique<IntLiteralExpr>(2, 1, 6), 1, 3);

    ASSERT_TRUE(isEqualExpression(actual, expected));
}

// Tests that comparison binds tighter than equality.
TEST(ExpressionPrecedence, ComparisonBeforeEquality)
{
    std::vector<Token> tokens = {
        Token("1", INTEGER, 1, 1, 1),
        Token("<", LESS, std::monostate{}, 1, 3),
        Token("2", INTEGER, 2, 1, 5),
        Token("==", EQUAL_EQUAL, std::monostate{}, 1, 7),
        Token("affirmative", BOOL, true, 1, 10),
        Token("", EOF_TOKEN, std::monostate{}, 1, 22),
    };

    Parser parser(tokens);
    auto   actual = parser.parseExpression();

    std::unique_ptr<Expr> expected = std::make_unique<BinaryExpr>(
        std::make_unique<BinaryExpr>(std::make_unique<IntLiteralExpr>(1, 1, 1), Less,
                                     std::make_unique<IntLiteralExpr>(2, 1, 5), 1, 3),
        EqualEqual, std::make_unique<BoolLiteralExpr>(true, 1, 10), 1, 7);

    ASSERT_TRUE(isEqualExpression(actual, expected));
}

// Tests parsing of a string with multiple interpolations.
TEST(ExpressionString, MultipleInterpolations)
{
    std::vector<Token> tokens = {
        Token("\"a \"", STRING, "a ", 1, 1),
        Token("{", INTERP_START, std::monostate{}, 1, 5),
        Token("x", IDENTIFIER, std::monostate{}, 1, 6),
        Token("}", INTERP_END, std::monostate{}, 1, 7),
        Token("\" b \"", STRING, " b ", 1, 8),
        Token("{", INTERP_START, std::monostate{}, 1, 13),
        Token("y", IDENTIFIER, std::monostate{}, 1, 14),
        Token("}", INTERP_END, std::monostate{}, 1, 15),
        Token("\" c\"", STRING, " c", 1, 16),
        Token("", EOF_TOKEN, std::monostate{}, 1, 20),
    };

    Parser parser(tokens);
    auto   actual = parser.parseExpression();

    std::vector<StringPart> parts;

    StringPart t1{StringPart::TEXT, "a ", nullptr};
    parts.push_back(std::move(t1));

    StringPart e1;
    e1.kind = StringPart::EXPR;
    e1.expr = std::make_unique<IdentifierExpr>("x", 1, 6);
    parts.push_back(std::move(e1));

    StringPart t2{StringPart::TEXT, " b ", nullptr};
    parts.push_back(std::move(t2));

    StringPart e2;
    e2.kind = StringPart::EXPR;
    e2.expr = std::make_unique<IdentifierExpr>("y", 1, 14);
    parts.push_back(std::move(e2));

    StringPart t3{StringPart::TEXT, " c", nullptr};
    parts.push_back(std::move(t3));

    std::unique_ptr<Expr> expected = std::make_unique<StringExpr>(std::move(parts), 1, 1);

    ASSERT_TRUE(isEqualExpression(actual, expected));
}

// Tests interpolation containing a full expression.
TEST(ExpressionString, InterpolationWithExpression)
{
    std::vector<Token> tokens = {
        Token("\"sum \"", STRING, "sum ", 1, 1),
        Token("{", INTERP_START, std::monostate{}, 1, 7),
        Token("1", INTEGER, 1, 1, 8),
        Token("+", PLUS, std::monostate{}, 1, 10),
        Token("2", INTEGER, 2, 1, 12),
        Token("}", INTERP_END, std::monostate{}, 1, 13),
        Token("\"\"", STRING, "", 1, 14),
        Token("", EOF_TOKEN, std::monostate{}, 1, 16),
    };

    Parser parser(tokens);
    auto   actual = parser.parseExpression();

    std::vector<StringPart> parts;

    StringPart text;
    text.kind = StringPart::TEXT;
    text.text = "sum ";
    parts.push_back(std::move(text));

    StringPart expr;
    expr.kind = StringPart::EXPR;
    expr.expr = std::make_unique<BinaryExpr>(std::make_unique<IntLiteralExpr>(1, 1, 8), Add,
                                             std::make_unique<IntLiteralExpr>(2, 1, 12), 1, 10);
    parts.push_back(std::move(expr));

    StringPart trailing;
    trailing.kind = StringPart::TEXT;
    trailing.text = "";
    parts.push_back(std::move(trailing));

    std::unique_ptr<Expr> expected = std::make_unique<StringExpr>(std::move(parts), 1, 1);

    ASSERT_TRUE(isEqualExpression(actual, expected));
}

// Tests error detection for unary operator without operand.
TEST(ExpressionErrors, UnaryMissingOperand)
{
    std::vector<Token> tokens = {
        Token("not", NOT, std::monostate{}, 1, 1),
        Token("", EOF_TOKEN, std::monostate{}, 1, 4),
    };

    Parser parser(tokens);
    auto   result = parser.parseExpression();

    ASSERT_TRUE(parser.hadError());
    ASSERT_EQ(result, nullptr);
}

// Tests parsing of inequality operator (!=).
TEST(ExpressionEquality, NotEqual)
{
    std::vector<Token> tokens = {
        Token("true", BOOL, true, 1, 1),
        Token("!=", BANG_EQUAL, std::monostate{}, 1, 6),
        Token("false", BOOL, false, 1, 9),
        Token("", EOF_TOKEN, std::monostate{}, 1, 14),
    };

    Parser parser(tokens);
    auto   actual = parser.parseExpression();

    std::unique_ptr<Expr> expected =
        std::make_unique<BinaryExpr>(std::make_unique<BoolLiteralExpr>(true, 1, 1), NotEqual,
                                     std::make_unique<BoolLiteralExpr>(false, 1, 9), 1, 6);

    ASSERT_TRUE(isEqualExpression(actual, expected));
}

// Tests parsing of deeply nested grouped expressions.
TEST(ExpressionGrouping, DeeplyNestedGrouping)
{
    std::vector<Token> tokens = {
        Token("(", LEFT_PAREN, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 2),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 3),
        Token("1", INTEGER, 1, 1, 4),
        Token("+", PLUS, std::monostate{}, 1, 6),
        Token("2", INTEGER, 2, 1, 8),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 9),
        Token("*", STAR, std::monostate{}, 1, 11),
        Token("3", INTEGER, 3, 1, 13),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 14),
        Token("-", MINUS, std::monostate{}, 1, 16),
        Token("4", INTEGER, 4, 1, 18),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 19),
        Token("", EOF_TOKEN, std::monostate{}, 1, 20),
    };

    Parser parser(tokens);
    auto   actual = parser.parseExpression();

    std::unique_ptr<Expr> expected = std::make_unique<GroupingExpr>(
        std::make_unique<BinaryExpr>(
            std::make_unique<GroupingExpr>( // ← THIS was missing
                std::make_unique<BinaryExpr>(
                    std::make_unique<GroupingExpr>(
                        std::make_unique<BinaryExpr>(std::make_unique<IntLiteralExpr>(1, 1, 4), Add,
                                                     std::make_unique<IntLiteralExpr>(2, 1, 8), 1,
                                                     6),
                        1, 3),
                    Multiply, std::make_unique<IntLiteralExpr>(3, 1, 13), 1, 11),
                1, 2),
            Subtract, std::make_unique<IntLiteralExpr>(4, 1, 18), 1, 16),
        1, 1);

    ASSERT_TRUE(isEqualExpression(actual, expected));
}

// Tests behavior when extra tokens appear after a valid expression.
TEST(ExpressionErrors, TrailingTokensAfterExpression)
{
    std::vector<Token> tokens = {
        Token("1", INTEGER, 1, 1, 1),
        Token("2", INTEGER, 2, 1, 3),
        Token("", EOF_TOKEN, std::monostate{}, 1, 4),
    };

    Parser parser(tokens);
    auto   result = parser.parseExpression();

    // Current parser accepts the first expression and ignores trailing tokens.
    // This test documents existing behavior.
    ASSERT_FALSE(parser.hadError());
    ASSERT_NE(result, nullptr);
}

// Tests parsing of less-than-or-equal operator.
TEST(ExpressionComparison, LessEqual)
{
    std::vector<Token> tokens = {
        Token("1", INTEGER, 1, 1, 1),
        Token("<=", LESS_EQUAL, std::monostate{}, 1, 3),
        Token("2", INTEGER, 2, 1, 6),
        Token("", EOF_TOKEN, std::monostate{}, 1, 7),
    };

    Parser parser(tokens);
    auto   actual = parser.parseExpression();

    std::unique_ptr<Expr> expected =
        std::make_unique<BinaryExpr>(std::make_unique<IntLiteralExpr>(1, 1, 1), LessEqual,
                                     std::make_unique<IntLiteralExpr>(2, 1, 6), 1, 3);

    ASSERT_TRUE(isEqualExpression(actual, expected));
}

TEST(SayStatement, SayStatement)
{
    std::vector<Token> tokens = {Token("say", SAY, std::monostate{}, 1, 1),
                                 Token("\"Hello \"", STRING, "Hello", 1, 4),
                                 Token(",", SEMI_COLON, std::monostate{}, 1, 9),
                                 Token("", EOF_TOKEN, std::monostate{}, 1, 10)};

    Parser     parser(tokens);
    const auto actual = parser.parseStatement();

    std::vector<StringPart> parts;
    StringPart              s1;
    s1.kind = StringPart::TEXT;
    s1.text = "Hello";
    parts.push_back(std::move(s1));

    std::unique_ptr<Stmt> expected =
        std::make_unique<SayStmt>(std::make_unique<StringExpr>(std::move(parts), 1, 4), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

TEST(SayStatement, SayIntLiteral)
{
    std::vector<Token> tokens = {
        Token("say", SAY, std::monostate{}, 1, 1),
        Token("42", INTEGER, 42, 1, 5),
        Token(";", SEMI_COLON, std::monostate{}, 1, 7),
        Token("", EOF_TOKEN, std::monostate{}, 1, 8),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    std::unique_ptr<Stmt> expected =
        std::make_unique<SayStmt>(std::make_unique<IntLiteralExpr>(42, 1, 5), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

TEST(SayStatement, SayIdentifier)
{
    std::vector<Token> tokens = {
        Token("say", SAY, std::monostate{}, 1, 1),
        Token("x", IDENTIFIER, std::monostate{}, 1, 5),
        Token(";", SEMI_COLON, std::monostate{}, 1, 6),
        Token("", EOF_TOKEN, std::monostate{}, 1, 7),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    std::unique_ptr<Stmt> expected =
        std::make_unique<SayStmt>(std::make_unique<IdentifierExpr>("x", 1, 5), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

TEST(SayStatement, SayBinaryExpression)
{
    std::vector<Token> tokens = {
        Token("say", SAY, std::monostate{}, 1, 1),
        Token("1", INTEGER, 1, 1, 5),
        Token("+", PLUS, std::monostate{}, 1, 7),
        Token("2", INTEGER, 2, 1, 9),
        Token(";", SEMI_COLON, std::monostate{}, 1, 10),
        Token("", EOF_TOKEN, std::monostate{}, 1, 11),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    std::unique_ptr<Stmt> expected = std::make_unique<SayStmt>(
        std::make_unique<BinaryExpr>(std::make_unique<IntLiteralExpr>(1, 1, 5), Add,
                                     std::make_unique<IntLiteralExpr>(2, 1, 9), 1, 7),
        1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

TEST(SayStatementErrors, SayMissingSemicolon)
{
    std::vector<Token> tokens = {
        Token("say", SAY, std::monostate{}, 1, 1),
        Token("1", INTEGER, 1, 1, 5),
        Token("", EOF_TOKEN, std::monostate{}, 1, 6),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    ASSERT_TRUE(parser.hadError());
    ASSERT_EQ(actual, nullptr);
}

TEST(SayStatementErrors, SayMissingExpression)
{
    std::vector<Token> tokens = {
        Token("say", SAY, std::monostate{}, 1, 1),
        Token(";", SEMI_COLON, std::monostate{}, 1, 5),
        Token("", EOF_TOKEN, std::monostate{}, 1, 6),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    ASSERT_TRUE(parser.hadError());
    ASSERT_EQ(actual, nullptr);
}

TEST(SayStatementErrors, SayInvalidExpression)
{
    std::vector<Token> tokens = {
        Token("say", SAY, std::monostate{}, 1, 1),
        Token("+", PLUS, std::monostate{}, 1, 5),
        Token(";", SEMI_COLON, std::monostate{}, 1, 6),
        Token("", EOF_TOKEN, std::monostate{}, 1, 7),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    ASSERT_TRUE(parser.hadError());
    ASSERT_EQ(actual, nullptr);
}

TEST(SummonStatement, SummonIntLiteral)
{
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("x", IDENTIFIER, std::monostate{}, 1, 8),
        Token("=", EQUAL, std::monostate{}, 1, 10),
        Token("10", INTEGER, 10, 1, 12),
        Token(";", SEMI_COLON, std::monostate{}, 1, 13),
        Token("", EOF_TOKEN, std::monostate{}, 1, 14),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    std::unique_ptr<Stmt> expected =
        std::make_unique<SummonStmt>("x", std::make_unique<IntLiteralExpr>(10, 1, 12), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

// === Summon statement tests ===

// Parses a summon statement with a boolean initializer.
TEST(SummonStatement, SummonBoolLiteral)
{
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("flag", IDENTIFIER, std::monostate{}, 1, 8),
        Token("=", EQUAL, std::monostate{}, 1, 13),
        Token("affirmative", BOOL, true, 1, 15),
        Token(";", SEMI_COLON, std::monostate{}, 1, 26),
        Token("", EOF_TOKEN, std::monostate{}, 1, 27),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    std::unique_ptr<Stmt> expected =
        std::make_unique<SummonStmt>("flag", std::make_unique<BoolLiteralExpr>(true, 1, 15), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

// Parses a summon statement with an identifier initializer.
TEST(SummonStatement, SummonIdentifierInitializer)
{
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("y", IDENTIFIER, std::monostate{}, 1, 8),
        Token("=", EQUAL, std::monostate{}, 1, 10),
        Token("x", IDENTIFIER, std::monostate{}, 1, 12),
        Token(";", SEMI_COLON, std::monostate{}, 1, 13),
        Token("", EOF_TOKEN, std::monostate{}, 1, 14),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    std::unique_ptr<Stmt> expected =
        std::make_unique<SummonStmt>("y", std::make_unique<IdentifierExpr>("x", 1, 12), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

// Parses a summon statement with a binary expression initializer to test precedence/AST wiring.
TEST(SummonStatement, SummonBinaryInitializerPrecedence)
{
    // summon x = 1 + 2 * 3;
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("x", IDENTIFIER, std::monostate{}, 1, 8),
        Token("=", EQUAL, std::monostate{}, 1, 10),
        Token("1", INTEGER, 1, 1, 12),
        Token("+", PLUS, std::monostate{}, 1, 14),
        Token("2", INTEGER, 2, 1, 16),
        Token("*", STAR, std::monostate{}, 1, 18),
        Token("3", INTEGER, 3, 1, 20),
        Token(";", SEMI_COLON, std::monostate{}, 1, 21),
        Token("", EOF_TOKEN, std::monostate{}, 1, 22),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    auto expectedInitializer = std::make_unique<BinaryExpr>(
        std::make_unique<IntLiteralExpr>(1, 1, 12), Add,
        std::make_unique<BinaryExpr>(std::make_unique<IntLiteralExpr>(2, 1, 16), Multiply,
                                     std::make_unique<IntLiteralExpr>(3, 1, 20), 1, 18),
        1, 14);

    std::unique_ptr<Stmt> expected =
        std::make_unique<SummonStmt>("x", std::move(expectedInitializer), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

// Parses a summon statement whose initializer uses grouping to override precedence.
TEST(SummonStatement, SummonGroupedInitializer)
{
    // summon x = (1 + 2) * 3;
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("x", IDENTIFIER, std::monostate{}, 1, 8),
        Token("=", EQUAL, std::monostate{}, 1, 10),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 12),
        Token("1", INTEGER, 1, 1, 13),
        Token("+", PLUS, std::monostate{}, 1, 15),
        Token("2", INTEGER, 2, 1, 17),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 18),
        Token("*", STAR, std::monostate{}, 1, 20),
        Token("3", INTEGER, 3, 1, 22),
        Token(";", SEMI_COLON, std::monostate{}, 1, 23),
        Token("", EOF_TOKEN, std::monostate{}, 1, 24),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    auto grouped = std::make_unique<GroupingExpr>(
        std::make_unique<BinaryExpr>(std::make_unique<IntLiteralExpr>(1, 1, 13), Add,
                                     std::make_unique<IntLiteralExpr>(2, 1, 17), 1, 15),
        1, 12);

    auto expectedInitializer = std::make_unique<BinaryExpr>(
        std::move(grouped), Multiply, std::make_unique<IntLiteralExpr>(3, 1, 22), 1, 20);

    std::unique_ptr<Stmt> expected =
        std::make_unique<SummonStmt>("x", std::move(expectedInitializer), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

// Parses a summon statement with a unary initializer.
TEST(SummonStatement, SummonUnaryInitializer)
{
    // summon x = -1;
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("x", IDENTIFIER, std::monostate{}, 1, 8),
        Token("=", EQUAL, std::monostate{}, 1, 10),
        Token("-", MINUS, std::monostate{}, 1, 12),
        Token("1", INTEGER, 1, 1, 13),
        Token(";", SEMI_COLON, std::monostate{}, 1, 14),
        Token("", EOF_TOKEN, std::monostate{}, 1, 15),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    std::unique_ptr<Stmt> expected = std::make_unique<SummonStmt>(
        "x",
        std::make_unique<UnaryExpr>(ArithmeticNegate, std::make_unique<IntLiteralExpr>(1, 1, 13), 1,
                                    12),
        1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

// Parses a summon statement with a comparison initializer.
TEST(SummonStatement, SummonComparisonInitializer)
{
    // summon ok = 2 < 3;
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("ok", IDENTIFIER, std::monostate{}, 1, 8),
        Token("=", EQUAL, std::monostate{}, 1, 11),
        Token("2", INTEGER, 2, 1, 13),
        Token("<", LESS, std::monostate{}, 1, 15),
        Token("3", INTEGER, 3, 1, 17),
        Token(";", SEMI_COLON, std::monostate{}, 1, 18),
        Token("", EOF_TOKEN, std::monostate{}, 1, 19),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    std::unique_ptr<Stmt> expected = std::make_unique<SummonStmt>(
        "ok",
        std::make_unique<BinaryExpr>(std::make_unique<IntLiteralExpr>(2, 1, 13), Less,
                                     std::make_unique<IntLiteralExpr>(3, 1, 17), 1, 15),
        1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

// Parses a summon statement with equality initializer to test precedence relative to comparisons.
TEST(SummonStatement, SummonEqualityInitializer)
{
    // summon eq = 1 < 2 == affirmative;
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("eq", IDENTIFIER, std::monostate{}, 1, 8),
        Token("=", EQUAL, std::monostate{}, 1, 11),
        Token("1", INTEGER, 1, 1, 13),
        Token("<", LESS, std::monostate{}, 1, 15),
        Token("2", INTEGER, 2, 1, 17),
        Token("==", EQUAL_EQUAL, std::monostate{}, 1, 19),
        Token("affirmative", BOOL, true, 1, 22),
        Token(";", SEMI_COLON, std::monostate{}, 1, 33),
        Token("", EOF_TOKEN, std::monostate{}, 1, 34),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    auto lhs = std::make_unique<BinaryExpr>(std::make_unique<IntLiteralExpr>(1, 1, 13), Less,
                                            std::make_unique<IntLiteralExpr>(2, 1, 17), 1, 15);

    auto expectedInitializer = std::make_unique<BinaryExpr>(
        std::move(lhs), EqualEqual, std::make_unique<BoolLiteralExpr>(true, 1, 22), 1, 19);

    std::unique_ptr<Stmt> expected =
        std::make_unique<SummonStmt>("eq", std::move(expectedInitializer), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

// Parses a summon statement whose initializer is a string interpolation expression.
TEST(SummonStatement, SummonInterpolatedStringInitializer)
{
    // summon msg = "hi {name}";
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("msg", IDENTIFIER, std::monostate{}, 1, 8),
        Token("=", EQUAL, std::monostate{}, 1, 12),
        Token("\"hi \"", STRING, "hi ", 1, 14),
        Token("{", INTERP_START, std::monostate{}, 1, 19),
        Token("name", IDENTIFIER, std::monostate{}, 1, 20),
        Token("}", INTERP_END, std::monostate{}, 1, 24),
        Token("\"\"", STRING, "", 1, 25),
        Token(";", SEMI_COLON, std::monostate{}, 1, 27),
        Token("", EOF_TOKEN, std::monostate{}, 1, 28),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    std::vector<StringPart> parts;

    StringPart t1;
    t1.kind = StringPart::TEXT;
    t1.text = "hi ";
    parts.push_back(std::move(t1));

    StringPart e1;
    e1.kind = StringPart::EXPR;
    e1.expr = std::make_unique<IdentifierExpr>("name", 1, 20);
    parts.push_back(std::move(e1));

    StringPart t2;
    t2.kind = StringPart::TEXT;
    t2.text = "";
    parts.push_back(std::move(t2));

    auto expectedInitializer = std::make_unique<StringExpr>(std::move(parts), 1, 14);

    std::unique_ptr<Stmt> expected =
        std::make_unique<SummonStmt>("msg", std::move(expectedInitializer), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

// --- Error cases ---

// Fails when the identifier after 'summon' is missing.
TEST(StatementErrors, SummonMissingIdentifier)
{
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("=", EQUAL, std::monostate{}, 1, 8),
        Token("10", INTEGER, 10, 1, 10),
        Token(";", SEMI_COLON, std::monostate{}, 1, 12),
        Token("", EOF_TOKEN, std::monostate{}, 1, 13),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    ASSERT_TRUE(parser.hadError());
    ASSERT_EQ(actual, nullptr);
}

// Fails when '=' is missing after the identifier.
TEST(StatementErrors, SummonMissingEqual)
{
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("x", IDENTIFIER, std::monostate{}, 1, 8),
        Token("10", INTEGER, 10, 1, 10),
        Token(";", SEMI_COLON, std::monostate{}, 1, 12),
        Token("", EOF_TOKEN, std::monostate{}, 1, 13),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    ASSERT_TRUE(parser.hadError());
    ASSERT_EQ(actual, nullptr);
}

// Fails when initializer expression is missing (e.g. 'summon x = ;').
TEST(StatementErrors, SummonMissingInitializer)
{
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("x", IDENTIFIER, std::monostate{}, 1, 8),
        Token("=", EQUAL, std::monostate{}, 1, 10),
        Token(";", SEMI_COLON, std::monostate{}, 1, 12),
        Token("", EOF_TOKEN, std::monostate{}, 1, 13),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    ASSERT_TRUE(parser.hadError());
    ASSERT_EQ(actual, nullptr);
}

// Fails when semicolon is missing at end of summon statement.
TEST(StatementErrors, SummonMissingSemicolon)
{
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("x", IDENTIFIER, std::monostate{}, 1, 8),
        Token("=", EQUAL, std::monostate{}, 1, 10),
        Token("10", INTEGER, 10, 1, 12),
        Token("", EOF_TOKEN, std::monostate{}, 1, 14),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    ASSERT_TRUE(parser.hadError());
    ASSERT_EQ(actual, nullptr);
}

// Fails if the initializer expression is malformed (e.g. 'summon x = + 1;').
TEST(StatementErrors, SummonInvalidInitializerExpression)
{
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("x", IDENTIFIER, std::monostate{}, 1, 8),
        Token("=", EQUAL, std::monostate{}, 1, 10),
        Token("+", PLUS, std::monostate{}, 1, 12),
        Token("1", INTEGER, 1, 1, 14),
        Token(";", SEMI_COLON, std::monostate{}, 1, 15),
        Token("", EOF_TOKEN, std::monostate{}, 1, 16),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    ASSERT_TRUE(parser.hadError());
    ASSERT_EQ(actual, nullptr);
}

TEST(BlockStatement, BlockStatementSimple)
{
    std::vector<Token> tokens = {
        Token("{", LEFT_BRACE, std::monostate{}, 1, 1),
        Token("summon", SUMMON, std::monostate{}, 1, 2),
        Token("flag", IDENTIFIER, std::monostate{}, 1, 9),
        Token("=", EQUAL, std::monostate{}, 1, 14),
        Token("affirmative", BOOL, true, 1, 16),
        Token(";", SEMI_COLON, std::monostate{}, 1, 17),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 18),
        Token("", EOF_TOKEN, std::monostate{}, 1, 19),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    std::vector<std::unique_ptr<Stmt>> summonExpression;
    summonExpression.push_back(
        std::make_unique<SummonStmt>("flag", std::make_unique<BoolLiteralExpr>(true, 1, 16), 1, 2));

    std::unique_ptr<Stmt> expected = std::make_unique<BlockStmt>(std::move(summonExpression), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

// Parses an empty block: { }
TEST(BlockStatement, BlockStatementEmpty)
{
    std::vector<Token> tokens = {
        Token("{", LEFT_BRACE, std::monostate{}, 1, 1),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 2),
        Token("", EOF_TOKEN, std::monostate{}, 1, 3),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    std::vector<std::unique_ptr<Stmt>> statements;

    std::unique_ptr<Stmt> expected = std::make_unique<BlockStmt>(std::move(statements), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

// Parses a block with multiple statements (summon + say).
TEST(BlockStatement, BlockStatementTwoStatements)
{
    std::vector<Token> tokens = {
        Token("{", LEFT_BRACE, std::monostate{}, 1, 1),

        Token("summon", SUMMON, std::monostate{}, 1, 3),
        Token("x", IDENTIFIER, std::monostate{}, 1, 10),
        Token("=", EQUAL, std::monostate{}, 1, 12),
        Token("10", INTEGER, 10, 1, 14),
        Token(";", SEMI_COLON, std::monostate{}, 1, 16),

        Token("say", SAY, std::monostate{}, 1, 18),
        Token("\"hi\"", STRING, std::string("hi"), 1, 22),
        Token(";", SEMI_COLON, std::monostate{}, 1, 26),

        Token("}", RIGHT_BRACE, std::monostate{}, 1, 28),
        Token("", EOF_TOKEN, std::monostate{}, 1, 29),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    std::vector<std::unique_ptr<Stmt>> statements;

    statements.push_back(
        std::make_unique<SummonStmt>("x", std::make_unique<IntLiteralExpr>(10, 1, 14), 1, 3));

    std::vector<StringPart> parts;
    StringPart              p;
    p.kind = StringPart::TEXT;
    p.text = "hi";
    parts.push_back(std::move(p));

    statements.push_back(
        std::make_unique<SayStmt>(std::make_unique<StringExpr>(std::move(parts), 1, 22), 1, 18));

    std::unique_ptr<Stmt> expected = std::make_unique<BlockStmt>(std::move(statements), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

// Parses nested blocks: { { } }
TEST(BlockStatement, BlockStatementNestedEmptyBlock)
{
    std::vector<Token> tokens = {
        Token("{", LEFT_BRACE, std::monostate{}, 1, 1),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 3),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 5),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 7),
        Token("", EOF_TOKEN, std::monostate{}, 1, 8),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    // inner block
    std::vector<std::unique_ptr<Stmt>> innerStatements;
    auto inner = std::make_unique<BlockStmt>(std::move(innerStatements), 1, 3);

    // outer block contains inner block stmt
    std::vector<std::unique_ptr<Stmt>> outerStatements;
    outerStatements.push_back(std::move(inner));

    std::unique_ptr<Stmt> expected = std::make_unique<BlockStmt>(std::move(outerStatements), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

// Parses nested block with content: { { summon x = 1; } }
TEST(BlockStatement, BlockStatementNestedWithSummon)
{
    std::vector<Token> tokens = {
        Token("{", LEFT_BRACE, std::monostate{}, 1, 1),

        Token("{", LEFT_BRACE, std::monostate{}, 1, 3),
        Token("summon", SUMMON, std::monostate{}, 1, 5),
        Token("x", IDENTIFIER, std::monostate{}, 1, 12),
        Token("=", EQUAL, std::monostate{}, 1, 14),
        Token("1", INTEGER, 1, 1, 16),
        Token(";", SEMI_COLON, std::monostate{}, 1, 17),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 19),

        Token("}", RIGHT_BRACE, std::monostate{}, 1, 21),
        Token("", EOF_TOKEN, std::monostate{}, 1, 22),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    std::vector<std::unique_ptr<Stmt>> innerStatements;
    innerStatements.push_back(
        std::make_unique<SummonStmt>("x", std::make_unique<IntLiteralExpr>(1, 1, 16), 1, 5));

    auto innerBlock = std::make_unique<BlockStmt>(std::move(innerStatements), 1, 3);

    std::vector<std::unique_ptr<Stmt>> outerStatements;
    outerStatements.push_back(std::move(innerBlock));

    std::unique_ptr<Stmt> expected = std::make_unique<BlockStmt>(std::move(outerStatements), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

// Parses a block with a say statement using string interpolation parts.
TEST(BlockStatement, BlockStatementSayInterpolatedString)
{
    std::vector<Token> tokens = {
        Token("{", LEFT_BRACE, std::monostate{}, 1, 1),

        Token("say", SAY, std::monostate{}, 1, 3),
        Token("\"hi \"", STRING, std::string("hi "), 1, 7),
        Token("{", INTERP_START, std::monostate{}, 1, 13),
        Token("name", IDENTIFIER, std::monostate{}, 1, 14),
        Token("}", INTERP_END, std::monostate{}, 1, 18),
        Token("\"\"", STRING, std::string(""), 1, 19),
        Token(";", SEMI_COLON, std::monostate{}, 1, 21),

        Token("}", RIGHT_BRACE, std::monostate{}, 1, 23),
        Token("", EOF_TOKEN, std::monostate{}, 1, 24),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    std::vector<std::unique_ptr<Stmt>> statements;

    std::vector<StringPart> parts;

    StringPart t1;
    t1.kind = StringPart::TEXT;
    t1.text = "hi ";
    parts.push_back(std::move(t1));

    StringPart e1;
    e1.kind = StringPart::EXPR;
    e1.expr = std::make_unique<IdentifierExpr>("name", 1, 14);
    parts.push_back(std::move(e1));

    StringPart t2;
    t2.kind = StringPart::TEXT;
    t2.text = "";
    parts.push_back(std::move(t2));

    statements.push_back(
        std::make_unique<SayStmt>(std::make_unique<StringExpr>(std::move(parts), 1, 7), 1, 3));

    std::unique_ptr<Stmt> expected = std::make_unique<BlockStmt>(std::move(statements), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

// Reports an error if EOF is reached before encountering a closing '}'.
TEST(StatementErrors, BlockStatementMissingRightBraceEOF)
{
    std::vector<Token> tokens = {
        Token("{", LEFT_BRACE, std::monostate{}, 1, 1),
        Token("say", SAY, std::monostate{}, 1, 3),
        Token("\"hi\"", STRING, std::string("hi"), 1, 7),
        Token(";", SEMI_COLON, std::monostate{}, 1, 11),
        Token("", EOF_TOKEN, std::monostate{}, 1, 12),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    ASSERT_EQ(actual, nullptr);
    ASSERT_TRUE(parser.hadError());
}

// If a statement inside the block fails (e.g., missing semicolon), block parsing fails fast.
TEST(BlockStatementErrors, BlockStatementInnerStatementError)
{
    std::vector<Token> tokens = {
        Token("{", LEFT_BRACE, std::monostate{}, 1, 1),

        Token("summon", SUMMON, std::monostate{}, 1, 3),
        Token("x", IDENTIFIER, std::monostate{}, 1, 10),
        Token("=", EQUAL, std::monostate{}, 1, 12),
        Token("10", INTEGER, 10, 1, 14),
        // Missing ';' here

        Token("}", RIGHT_BRACE, std::monostate{}, 1, 16),
        Token("", EOF_TOKEN, std::monostate{}, 1, 17),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    ASSERT_EQ(actual, nullptr);
    ASSERT_TRUE(parser.hadError());
}

// Parses a block and leaves remaining tokens for later parsing (program-level tests later).
TEST(BlockStatement, BlockStatementStopsAtRightBrace)
{
    std::vector<Token> tokens = {
        Token("{", LEFT_BRACE, std::monostate{}, 1, 1),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 2),

        // Trailing statement tokens (not parsed by this single parseStatement() call)
        Token("say", SAY, std::monostate{}, 1, 4),
        Token("\"hi\"", STRING, std::string("hi"), 1, 8),
        Token(";", SEMI_COLON, std::monostate{}, 1, 12),

        Token("", EOF_TOKEN, std::monostate{}, 1, 13),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    std::vector<std::unique_ptr<Stmt>> statements;
    std::unique_ptr<Stmt> expected = std::make_unique<BlockStmt>(std::move(statements), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

TEST(IfStatement, IfChainSimple)
{
    std::vector<Token> tokens = {
        Token("should", SHOULD, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 8),
        Token("x", IDENTIFIER, std::monostate{}, 1, 9),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 10),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 12),
        Token("say", SAY, std::monostate{}, 1, 14),
        Token("\"yes\"", STRING, "yes", 1, 18),
        Token(";", SEMI_COLON, std::monostate{}, 1, 23),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 25),
        Token("", EOF_TOKEN, std::monostate{}, 1, 26),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    // Build expected AST
    std::vector<std::tuple<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> branches;

    // Condition: x
    auto condition = std::make_unique<IdentifierExpr>("x", 1, 9);

    // Body: { say "yes"; }
    std::vector<std::unique_ptr<Stmt>> bodyStmts;
    {
        std::vector<StringPart> parts;
        StringPart              p;
        p.kind = StringPart::TEXT;
        p.text = "yes";
        parts.push_back(std::move(p));

        bodyStmts.push_back(std::make_unique<SayStmt>(
            std::make_unique<StringExpr>(std::move(parts), 1, 18), 1, 14));
    }

    auto body = std::make_unique<BlockStmt>(std::move(bodyStmts), 1, 12);

    branches.emplace_back(std::move(condition), std::move(body));

    std::unique_ptr<Stmt> expected =
        std::make_unique<IfChainStmt>(std::move(branches), nullptr, 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

/**
 * should (x) { say "yes"; }
 * Baseline: single-branch if-chain with a single statement in the block.
 */
TEST(IfStatement, SingleBranch_SayString)
{
    std::vector<Token> tokens = {
        Token("should", SHOULD, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 8),
        Token("x", IDENTIFIER, std::monostate{}, 1, 9),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 10),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 12),
        Token("say", SAY, std::monostate{}, 1, 14),
        Token("\"yes\"", STRING, std::string("yes"), 1, 18),
        Token(";", SEMI_COLON, std::monostate{}, 1, 23),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 25),
        Token("", EOF_TOKEN, std::monostate{}, 1, 26),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    // Condition: Identifier(x)
    auto cond = std::make_unique<IdentifierExpr>("x", 1, 9);

    // Body: { say "yes"; }
    std::vector<std::unique_ptr<Stmt>> bodyStmts;
    {
        std::vector<StringPart> parts;
        StringPart              p;
        p.kind = StringPart::TEXT;
        p.text = "yes";
        parts.push_back(std::move(p));

        bodyStmts.push_back(std::make_unique<SayStmt>(
            std::make_unique<StringExpr>(std::move(parts), 1, 18), 1, 14));
    }
    auto body = std::make_unique<BlockStmt>(std::move(bodyStmts), 1, 12);

    std::vector<std::tuple<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> branches;
    branches.emplace_back(std::move(cond), std::move(body));

    std::unique_ptr<Stmt> expected =
        std::make_unique<IfChainStmt>(std::move(branches), nullptr, 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

/**
 * should (affirmative) { }
 * Edge: empty block should parse and produce a BlockStmt with 0 statements.
 */
TEST(IfStatement, SingleBranch_EmptyBlock)
{
    std::vector<Token> tokens = {
        Token("should", SHOULD, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 8),
        Token("affirmative", BOOL, true, 1, 9),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 20),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 22),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 24),
        Token("", EOF_TOKEN, std::monostate{}, 1, 25),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    auto cond = std::make_unique<BoolLiteralExpr>(true, 1, 9);

    std::vector<std::unique_ptr<Stmt>> bodyStmts;
    auto body = std::make_unique<BlockStmt>(std::move(bodyStmts), 1, 22);

    std::vector<std::tuple<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> branches;
    branches.emplace_back(std::move(cond), std::move(body));

    std::unique_ptr<Stmt> expected =
        std::make_unique<IfChainStmt>(std::move(branches), nullptr, 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

/**
 * should (x) { say "a"; } otherwise { say "b"; }
 * Standard: else-branch exists, no else-if branches.
 */
TEST(IfStatement, WithElseBranch)
{
    std::vector<Token> tokens = {
        Token("should", SHOULD, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 8),
        Token("x", IDENTIFIER, std::monostate{}, 1, 9),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 10),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 12),
        Token("say", SAY, std::monostate{}, 1, 14),
        Token("\"a\"", STRING, std::string("a"), 1, 18),
        Token(";", SEMI_COLON, std::monostate{}, 1, 21),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 23),
        Token("otherwise", OTHERWISE, std::monostate{}, 1, 25),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 35),
        Token("say", SAY, std::monostate{}, 1, 37),
        Token("\"b\"", STRING, std::string("b"), 1, 41),
        Token(";", SEMI_COLON, std::monostate{}, 1, 44),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 46),
        Token("", EOF_TOKEN, std::monostate{}, 1, 47),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    // First branch
    auto cond = std::make_unique<IdentifierExpr>("x", 1, 9);

    std::vector<std::unique_ptr<Stmt>> thenStmts;
    {
        std::vector<StringPart> parts;
        StringPart              p{};
        p.kind = StringPart::TEXT;
        p.text = "a";
        parts.push_back(std::move(p));
        thenStmts.push_back(std::make_unique<SayStmt>(
            std::make_unique<StringExpr>(std::move(parts), 1, 18), 1, 14));
    }
    auto thenBlock = std::make_unique<BlockStmt>(std::move(thenStmts), 1, 12);

    std::vector<std::tuple<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> branches;
    branches.emplace_back(std::move(cond), std::move(thenBlock));

    // Else branch
    std::vector<std::unique_ptr<Stmt>> elseStmts;
    {
        std::vector<StringPart> parts;
        StringPart              p{};
        p.kind = StringPart::TEXT;
        p.text = "b";
        parts.push_back(std::move(p));
        elseStmts.push_back(std::make_unique<SayStmt>(
            std::make_unique<StringExpr>(std::move(parts), 1, 41), 1, 37));
    }
    auto elseBlock = std::make_unique<BlockStmt>(std::move(elseStmts), 1, 35);

    std::unique_ptr<Stmt> expected =
        std::make_unique<IfChainStmt>(std::move(branches), std::move(elseBlock), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

/**
 * should (x) { say "a"; } otherwise should (y) { say "b"; }
 * Standard: two branches, no trailing else.
 */
TEST(IfStatement, OneElseIf_NoElse)
{
    std::vector<Token> tokens = {
        Token("should", SHOULD, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 8),
        Token("x", IDENTIFIER, std::monostate{}, 1, 9),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 10),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 12),
        Token("say", SAY, std::monostate{}, 1, 14),
        Token("\"a\"", STRING, std::string("a"), 1, 18),
        Token(";", SEMI_COLON, std::monostate{}, 1, 21),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 23),

        Token("otherwise", OTHERWISE, std::monostate{}, 1, 25),
        Token("should", SHOULD, std::monostate{}, 1, 35),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 42),
        Token("y", IDENTIFIER, std::monostate{}, 1, 43),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 44),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 46),
        Token("say", SAY, std::monostate{}, 1, 48),
        Token("\"b\"", STRING, std::string("b"), 1, 52),
        Token(";", SEMI_COLON, std::monostate{}, 1, 55),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 57),

        Token("", EOF_TOKEN, std::monostate{}, 1, 58),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    std::vector<std::tuple<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> branches;

    // Branch 1: (x) { say "a"; }
    {
        auto cond = std::make_unique<IdentifierExpr>("x", 1, 9);

        std::vector<std::unique_ptr<Stmt>> stmts;
        std::vector<StringPart>            parts;
        StringPart                         p;
        p.kind = StringPart::TEXT;
        p.text = "a";
        parts.push_back(std::move(p));
        stmts.push_back(std::make_unique<SayStmt>(
            std::make_unique<StringExpr>(std::move(parts), 1, 18), 1, 14));

        auto block = std::make_unique<BlockStmt>(std::move(stmts), 1, 12);
        branches.emplace_back(std::move(cond), std::move(block));
    }

    // Branch 2: (y) { say "b"; }
    {
        auto cond = std::make_unique<IdentifierExpr>("y", 1, 43);

        std::vector<std::unique_ptr<Stmt>> stmts;
        std::vector<StringPart>            parts;
        StringPart                         p;
        p.kind = StringPart::TEXT;
        p.text = "b";
        parts.push_back(std::move(p));
        stmts.push_back(std::make_unique<SayStmt>(
            std::make_unique<StringExpr>(std::move(parts), 1, 52), 1, 48));

        auto block = std::make_unique<BlockStmt>(std::move(stmts), 1, 46);
        branches.emplace_back(std::move(cond), std::move(block));
    }

    std::unique_ptr<Stmt> expected =
        std::make_unique<IfChainStmt>(std::move(branches), nullptr, 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

/**
 * should (x) {...} otherwise should (y) {...} otherwise should (z) {...} otherwise {...}
 * Full chain: multiple else-if branches and a trailing else.
 */
TEST(IfStatement, MultipleElseIf_WithElse)
{
    std::vector<Token> tokens = {
        // should (x) { say "a"; }
        Token("should", SHOULD, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 8),
        Token("x", IDENTIFIER, std::monostate{}, 1, 9),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 10),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 12),
        Token("say", SAY, std::monostate{}, 1, 14),
        Token("\"a\"", STRING, std::string("a"), 1, 18),
        Token(";", SEMI_COLON, std::monostate{}, 1, 21),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 23),

        // otherwise should (y) { say "b"; }
        Token("otherwise", OTHERWISE, std::monostate{}, 1, 25),
        Token("should", SHOULD, std::monostate{}, 1, 35),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 42),
        Token("y", IDENTIFIER, std::monostate{}, 1, 43),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 44),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 46),
        Token("say", SAY, std::monostate{}, 1, 48),
        Token("\"b\"", STRING, std::string("b"), 1, 52),
        Token(";", SEMI_COLON, std::monostate{}, 1, 55),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 57),

        // otherwise should (z) { say "c"; }
        Token("otherwise", OTHERWISE, std::monostate{}, 1, 59),
        Token("should", SHOULD, std::monostate{}, 1, 69),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 76),
        Token("z", IDENTIFIER, std::monostate{}, 1, 77),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 78),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 80),
        Token("say", SAY, std::monostate{}, 1, 82),
        Token("\"c\"", STRING, std::string("c"), 1, 86),
        Token(";", SEMI_COLON, std::monostate{}, 1, 89),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 91),

        // otherwise { say "d"; }
        Token("otherwise", OTHERWISE, std::monostate{}, 1, 93),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 103),
        Token("say", SAY, std::monostate{}, 1, 105),
        Token("\"d\"", STRING, std::string("d"), 1, 109),
        Token(";", SEMI_COLON, std::monostate{}, 1, 112),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 114),

        Token("", EOF_TOKEN, std::monostate{}, 1, 115),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    std::vector<std::tuple<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> branches;

    auto mkSayBlock = [&](const std::string& text, int braceLine, int braceCol, int sayLine,
                          int sayCol, int strLine, int strCol)
    {
        std::vector<std::unique_ptr<Stmt>> stmts;
        std::vector<StringPart>            parts;
        StringPart                         p;
        p.kind = StringPart::TEXT;
        p.text = text;
        parts.push_back(std::move(p));
        stmts.push_back(std::make_unique<SayStmt>(
            std::make_unique<StringExpr>(std::move(parts), strLine, strCol), sayLine, sayCol));
        return std::make_unique<BlockStmt>(std::move(stmts), braceLine, braceCol);
    };

    // x -> "a"
    branches.emplace_back(std::make_unique<IdentifierExpr>("x", 1, 9),
                          mkSayBlock("a", 1, 12, 1, 14, 1, 18));

    // y -> "b"
    branches.emplace_back(std::make_unique<IdentifierExpr>("y", 1, 43),
                          mkSayBlock("b", 1, 46, 1, 48, 1, 52));

    // z -> "c"
    branches.emplace_back(std::make_unique<IdentifierExpr>("z", 1, 77),
                          mkSayBlock("c", 1, 80, 1, 82, 1, 86));

    auto elseBranch = mkSayBlock("d", 1, 103, 1, 105, 1, 109);

    std::unique_ptr<Stmt> expected =
        std::make_unique<IfChainStmt>(std::move(branches), std::move(elseBranch), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

/**
 * should x) { ... }
 * Error: missing '(' immediately after should.
 */
TEST(IfChainErrors, MissingLeftParenAfterShould)
{
    std::vector<Token> tokens = {
        Token("should", SHOULD, std::monostate{}, 1, 1),
        Token("x", IDENTIFIER, std::monostate{}, 1, 8),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 9),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 11),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 12),
        Token("", EOF_TOKEN, std::monostate{}, 1, 13),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    ASSERT_TRUE(parser.hadError());
    ASSERT_EQ(actual, nullptr);
}

/**
 * should (x { ... }
 * Error: missing ')' after condition expression.
 */
TEST(IfChainErrors, MissingRightParenAfterCondition)
{
    std::vector<Token> tokens = {
        Token("should", SHOULD, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 8),
        Token("x", IDENTIFIER, std::monostate{}, 1, 9),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 11),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 12),
        Token("", EOF_TOKEN, std::monostate{}, 1, 13),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    ASSERT_TRUE(parser.hadError());
    ASSERT_EQ(actual, nullptr);
}

/**
 * should (x) say "hi";
 * Error: expected '{' to start block after condition.
 */
TEST(IfChainErrors, MissingLeftBraceAfterCondition)
{
    std::vector<Token> tokens = {
        Token("should", SHOULD, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 8),
        Token("x", IDENTIFIER, std::monostate{}, 1, 9),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 10),
        Token("say", SAY, std::monostate{}, 1, 12),
        Token("\"hi\"", STRING, std::string("hi"), 1, 16),
        Token(";", SEMI_COLON, std::monostate{}, 1, 20),
        Token("", EOF_TOKEN, std::monostate{}, 1, 21),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    ASSERT_TRUE(parser.hadError());
    ASSERT_EQ(actual, nullptr);
}

/**
 * should (x) { } otherwise say "no";
 * Error: 'otherwise' must be followed by '{' (else block required).
 */
TEST(IfChainErrors, OtherwiseMissingLeftBrace)
{
    std::vector<Token> tokens = {
        Token("should", SHOULD, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 8),
        Token("x", IDENTIFIER, std::monostate{}, 1, 9),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 10),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 12),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 13),

        Token("otherwise", OTHERWISE, std::monostate{}, 1, 15),
        Token("say", SAY, std::monostate{}, 1, 25),
        Token("\"no\"", STRING, std::string("no"), 1, 29),
        Token(";", SEMI_COLON, std::monostate{}, 1, 33),
        Token("", EOF_TOKEN, std::monostate{}, 1, 34),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    ASSERT_TRUE(parser.hadError());
    ASSERT_EQ(actual, nullptr);
}

/**
 * should (x) { } otherwise (y) { }
 * Error: 'otherwise' followed by '(' is invalid; must be "otherwise should (...)" or "otherwise
 * {...}".
 */
TEST(IfChainErrors, OtherwiseThenParenIsInvalid)
{
    std::vector<Token> tokens = {
        Token("should", SHOULD, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 8),
        Token("x", IDENTIFIER, std::monostate{}, 1, 9),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 10),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 12),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 13),

        Token("otherwise", OTHERWISE, std::monostate{}, 1, 15),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 25),
        Token("y", IDENTIFIER, std::monostate{}, 1, 26),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 27),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 29),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 30),
        Token("", EOF_TOKEN, std::monostate{}, 1, 31),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    ASSERT_TRUE(parser.hadError());
    ASSERT_EQ(actual, nullptr);
}

/**
 * should (x) {
 *   should (y) {
 *     say "inner";
 *   }
 * }
 *
 * Tests nested if-chain parsing inside a block.
 */
TEST(IfStatement, NestedIfInsideBlock)
{
    std::vector<Token> tokens = {
        Token("should", SHOULD, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 8),
        Token("x", IDENTIFIER, std::monostate{}, 1, 9),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 10),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 12),

        Token("should", SHOULD, std::monostate{}, 2, 3),
        Token("(", LEFT_PAREN, std::monostate{}, 2, 10),
        Token("y", IDENTIFIER, std::monostate{}, 2, 11),
        Token(")", RIGHT_PAREN, std::monostate{}, 2, 12),
        Token("{", LEFT_BRACE, std::monostate{}, 2, 14),
        Token("say", SAY, std::monostate{}, 3, 5),
        Token("\"inner\"", STRING, std::string("inner"), 3, 9),
        Token(";", SEMI_COLON, std::monostate{}, 3, 16),
        Token("}", RIGHT_BRACE, std::monostate{}, 4, 3),

        Token("}", RIGHT_BRACE, std::monostate{}, 5, 1),
        Token("", EOF_TOKEN, std::monostate{}, 5, 2),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    // Inner if
    std::vector<std::unique_ptr<Stmt>> innerStmts;
    {
        std::vector<StringPart> parts;
        StringPart              p;
        p.kind = StringPart::TEXT;
        p.text = "inner";
        parts.push_back(std::move(p));

        innerStmts.push_back(
            std::make_unique<SayStmt>(std::make_unique<StringExpr>(std::move(parts), 3, 9), 3, 5));
    }

    auto innerBlock = std::make_unique<BlockStmt>(std::move(innerStmts), 2, 14);

    std::vector<std::tuple<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> innerBranches;
    innerBranches.emplace_back(std::make_unique<IdentifierExpr>("y", 2, 11), std::move(innerBlock));

    auto innerIf = std::make_unique<IfChainStmt>(std::move(innerBranches), nullptr, 2, 3);

    // Outer block
    std::vector<std::unique_ptr<Stmt>> outerStmts;
    outerStmts.push_back(std::move(innerIf));

    auto outerBlock = std::make_unique<BlockStmt>(std::move(outerStmts), 1, 12);

    std::vector<std::tuple<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> outerBranches;
    outerBranches.emplace_back(std::make_unique<IdentifierExpr>("x", 1, 9), std::move(outerBlock));

    std::unique_ptr<Stmt> expected =
        std::make_unique<IfChainStmt>(std::move(outerBranches), nullptr, 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

/**
 * should ((x + 1) * 2 > 3 == affirmative) { }
 *
 * Tests full precedence stack inside a condition expression.
 */
TEST(IfStatement, ConditionWithFullExpressionPrecedence)
{
    std::vector<Token> tokens = {
        Token("should", SHOULD, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 8),

        Token("(", LEFT_PAREN, std::monostate{}, 1, 9),
        Token("x", IDENTIFIER, std::monostate{}, 1, 10),
        Token("+", PLUS, std::monostate{}, 1, 12),
        Token("1", INTEGER, 1, 1, 14),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 15),
        Token("*", STAR, std::monostate{}, 1, 17),
        Token("2", INTEGER, 2, 1, 19),
        Token(">", GREATER, std::monostate{}, 1, 21),
        Token("3", INTEGER, 3, 1, 23),
        Token("==", EQUAL_EQUAL, std::monostate{}, 1, 25),
        Token("affirmative", BOOL, true, 1, 28),

        Token(")", RIGHT_PAREN, std::monostate{}, 1, 40),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 42),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 43),
        Token("", EOF_TOKEN, std::monostate{}, 1, 44),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    // (((x + 1) * 2) > 3) == true
    auto expr = std::make_unique<BinaryExpr>(
        std::make_unique<BinaryExpr>(
            std::make_unique<BinaryExpr>(
                std::make_unique<GroupingExpr>(
                    std::make_unique<BinaryExpr>(std::make_unique<IdentifierExpr>("x", 1, 10), Add,
                                                 std::make_unique<IntLiteralExpr>(1, 1, 14), 1, 12),
                    1, 9),
                Multiply, std::make_unique<IntLiteralExpr>(2, 1, 19), 1, 17),
            Greater, std::make_unique<IntLiteralExpr>(3, 1, 23), 1, 21),
        EqualEqual, std::make_unique<BoolLiteralExpr>(true, 1, 28), 1, 25);

    std::vector<std::unique_ptr<Stmt>> stmts;
    auto                               block = std::make_unique<BlockStmt>(std::move(stmts), 1, 42);

    std::vector<std::tuple<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> branches;
    branches.emplace_back(std::move(expr), std::move(block));

    std::unique_ptr<Stmt> expected =
        std::make_unique<IfChainStmt>(std::move(branches), nullptr, 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

/**
 * should (x) {
 *   say "value = {x}";
 * }
 *
 * Tests interpolation inside say statement inside if-branch.
 */
TEST(IfStatement, InterpolatedStringInsideBranch)
{
    std::vector<Token> tokens = {
        Token("should", SHOULD, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 8),
        Token("x", IDENTIFIER, std::monostate{}, 1, 9),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 10),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 12),

        Token("say", SAY, std::monostate{}, 2, 3),
        Token("\"value = \"", STRING, std::string("value = "), 2, 7),
        Token("{", INTERP_START, std::monostate{}, 2, 17),
        Token("x", IDENTIFIER, std::monostate{}, 2, 18),
        Token("}", INTERP_END, std::monostate{}, 2, 19),
        Token("\"\"", STRING, std::string(""), 2, 20),
        Token(";", SEMI_COLON, std::monostate{}, 2, 22),

        Token("}", RIGHT_BRACE, std::monostate{}, 3, 1),
        Token("", EOF_TOKEN, std::monostate{}, 3, 2),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    std::vector<StringPart> parts;
    parts.push_back({StringPart::TEXT, "value = ", nullptr});
    parts.push_back({StringPart::EXPR, "", std::make_unique<IdentifierExpr>("x", 2, 18)});
    parts.push_back({StringPart::TEXT, "", nullptr});

    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(
        std::make_unique<SayStmt>(std::make_unique<StringExpr>(std::move(parts), 2, 7), 2, 3));

    auto block = std::make_unique<BlockStmt>(std::move(stmts), 1, 12);

    std::vector<std::tuple<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> branches;
    branches.emplace_back(std::make_unique<IdentifierExpr>("x", 1, 9), std::move(block));

    std::unique_ptr<Stmt> expected =
        std::make_unique<IfChainStmt>(std::move(branches), nullptr, 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

/**
 * should (x) {
 *   should (y) { say "a"; }
 * } otherwise { say "b"; }
 *
 * Ensures else binds to the outer should, not the inner one.
 */
TEST(IfStatement, DanglingElseAttachesToOuter)
{
    std::vector<Token> tokens = {
        Token("should", SHOULD, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 8),
        Token("x", IDENTIFIER, std::monostate{}, 1, 9),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 10),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 12),

        Token("should", SHOULD, std::monostate{}, 2, 3),
        Token("(", LEFT_PAREN, std::monostate{}, 2, 10),
        Token("y", IDENTIFIER, std::monostate{}, 2, 11),
        Token(")", RIGHT_PAREN, std::monostate{}, 2, 12),
        Token("{", LEFT_BRACE, std::monostate{}, 2, 14),
        Token("say", SAY, std::monostate{}, 3, 5),
        Token("\"a\"", STRING, std::string("a"), 3, 9),
        Token(";", SEMI_COLON, std::monostate{}, 3, 12),
        Token("}", RIGHT_BRACE, std::monostate{}, 4, 3),

        Token("}", RIGHT_BRACE, std::monostate{}, 5, 1),

        Token("otherwise", OTHERWISE, std::monostate{}, 5, 3),
        Token("{", LEFT_BRACE, std::monostate{}, 5, 13),
        Token("say", SAY, std::monostate{}, 6, 5),
        Token("\"b\"", STRING, std::string("b"), 6, 9),
        Token(";", SEMI_COLON, std::monostate{}, 6, 12),
        Token("}", RIGHT_BRACE, std::monostate{}, 7, 1),

        Token("", EOF_TOKEN, std::monostate{}, 7, 2),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    // Inner if
    std::vector<std::unique_ptr<Stmt>> innerStmts;
    {
        std::vector<StringPart> parts;
        parts.push_back({StringPart::TEXT, "a", nullptr});
        innerStmts.push_back(
            std::make_unique<SayStmt>(std::make_unique<StringExpr>(std::move(parts), 3, 9), 3, 5));
    }

    auto innerBlock = std::make_unique<BlockStmt>(std::move(innerStmts), 2, 14);

    std::vector<std::tuple<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> innerBranches;
    innerBranches.emplace_back(std::make_unique<IdentifierExpr>("y", 2, 11), std::move(innerBlock));

    auto innerIf = std::make_unique<IfChainStmt>(std::move(innerBranches), nullptr, 2, 3);

    // Outer then block
    std::vector<std::unique_ptr<Stmt>> outerThen;
    outerThen.push_back(std::move(innerIf));

    auto outerThenBlock = std::make_unique<BlockStmt>(std::move(outerThen), 1, 12);

    // Else block
    std::vector<std::unique_ptr<Stmt>> elseStmts;
    {
        std::vector<StringPart> parts;
        parts.push_back({StringPart::TEXT, "b", nullptr});
        elseStmts.push_back(
            std::make_unique<SayStmt>(std::make_unique<StringExpr>(std::move(parts), 6, 9), 6, 5));
    }
    auto elseBlock = std::make_unique<BlockStmt>(std::move(elseStmts), 5, 13);

    std::vector<std::tuple<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> branches;
    branches.emplace_back(std::make_unique<IdentifierExpr>("x", 1, 9), std::move(outerThenBlock));

    std::unique_ptr<Stmt> expected =
        std::make_unique<IfChainStmt>(std::move(branches), std::move(elseBlock), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

// === While / aslongas statement tests ===

/**
 * aslongas (affirmative) { }
 * Tests simplest while-loop with a boolean literal condition and empty block.
 */
TEST(WhileStatement, EmptyBlockBoolCondition)
{
    std::vector<Token> tokens = {
        Token("aslongas", ASLONGAS, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 10),
        Token("affirmative", BOOL, true, 1, 11),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 23),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 25),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 26),
        Token("", EOF_TOKEN, std::monostate{}, 1, 27),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    auto                               condition = std::make_unique<BoolLiteralExpr>(true, 1, 11);
    std::vector<std::unique_ptr<Stmt>> stmts;
    auto                               block = std::make_unique<BlockStmt>(std::move(stmts), 1, 25);

    std::unique_ptr<Stmt> expected =
        std::make_unique<WhileStmt>(std::move(condition), std::move(block), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

/**
 * aslongas (x) { say "hi"; }
 * Tests identifier condition and a single statement in the body.
 */
TEST(WhileStatement, IdentifierConditionSingleSay)
{
    std::vector<Token> tokens = {
        Token("aslongas", ASLONGAS, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 10),
        Token("x", IDENTIFIER, std::monostate{}, 1, 11),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 12),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 14),

        Token("say", SAY, std::monostate{}, 1, 16),
        Token("\"hi\"", STRING, std::string("hi"), 1, 20),
        Token(";", SEMI_COLON, std::monostate{}, 1, 24),

        Token("}", RIGHT_BRACE, std::monostate{}, 1, 26),
        Token("", EOF_TOKEN, std::monostate{}, 1, 27),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    auto condition = std::make_unique<IdentifierExpr>("x", 1, 11);

    std::vector<StringPart> parts;
    StringPart              p;
    p.kind = StringPart::TEXT;
    p.text = "hi";
    parts.push_back(std::move(p));

    std::vector<std::unique_ptr<Stmt>> body;
    body.push_back(
        std::make_unique<SayStmt>(std::make_unique<StringExpr>(std::move(parts), 1, 20), 1, 16));

    auto block = std::make_unique<BlockStmt>(std::move(body), 1, 14);

    std::unique_ptr<Stmt> expected =
        std::make_unique<WhileStmt>(std::move(condition), std::move(block), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

/**
 * aslongas ((x + 1) * 2 > 3 == affirmative) { }
 * Tests full precedence stack inside while condition.
 */
TEST(WhileStatement, ComplexConditionFullPrecedence)
{
    std::vector<Token> tokens = {
        Token("aslongas", ASLONGAS, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 10),

        Token("(", LEFT_PAREN, std::monostate{}, 1, 11),
        Token("x", IDENTIFIER, std::monostate{}, 1, 12),
        Token("+", PLUS, std::monostate{}, 1, 14),
        Token("1", INTEGER, 1, 1, 16),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 17),
        Token("*", STAR, std::monostate{}, 1, 19),
        Token("2", INTEGER, 2, 1, 21),
        Token(">", GREATER, std::monostate{}, 1, 23),
        Token("3", INTEGER, 3, 1, 25),
        Token("==", EQUAL_EQUAL, std::monostate{}, 1, 27),
        Token("affirmative", BOOL, true, 1, 30),

        Token(")", RIGHT_PAREN, std::monostate{}, 1, 42),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 44),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 45),
        Token("", EOF_TOKEN, std::monostate{}, 1, 46),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    auto cond = std::make_unique<BinaryExpr>(
        std::make_unique<BinaryExpr>(
            std::make_unique<BinaryExpr>(
                std::make_unique<GroupingExpr>(
                    std::make_unique<BinaryExpr>(std::make_unique<IdentifierExpr>("x", 1, 12), Add,
                                                 std::make_unique<IntLiteralExpr>(1, 1, 16), 1, 14),
                    1, 11),
                Multiply, std::make_unique<IntLiteralExpr>(2, 1, 21), 1, 19),
            Greater, std::make_unique<IntLiteralExpr>(3, 1, 25), 1, 23),
        EqualEqual, std::make_unique<BoolLiteralExpr>(true, 1, 30), 1, 27);

    std::vector<std::unique_ptr<Stmt>> stmts;
    auto                               block = std::make_unique<BlockStmt>(std::move(stmts), 1, 44);

    std::unique_ptr<Stmt> expected =
        std::make_unique<WhileStmt>(std::move(cond), std::move(block), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

/**
 * aslongas (x) { summon y = 10; say "ok"; }
 * Tests multiple statements inside while body (summon + say).
 */
TEST(WhileStatement, MultipleStatementsInBody)
{
    std::vector<Token> tokens = {
        Token("aslongas", ASLONGAS, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 10),
        Token("x", IDENTIFIER, std::monostate{}, 1, 11),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 12),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 14),

        Token("summon", SUMMON, std::monostate{}, 2, 3),
        Token("y", IDENTIFIER, std::monostate{}, 2, 10),
        Token("=", EQUAL, std::monostate{}, 2, 12),
        Token("10", INTEGER, 10, 2, 14),
        Token(";", SEMI_COLON, std::monostate{}, 2, 16),

        Token("say", SAY, std::monostate{}, 3, 3),
        Token("\"ok\"", STRING, std::string("ok"), 3, 7),
        Token(";", SEMI_COLON, std::monostate{}, 3, 11),

        Token("}", RIGHT_BRACE, std::monostate{}, 4, 1),
        Token("", EOF_TOKEN, std::monostate{}, 4, 2),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    auto condition = std::make_unique<IdentifierExpr>("x", 1, 11);

    std::vector<std::unique_ptr<Stmt>> body;
    body.push_back(
        std::make_unique<SummonStmt>("y", std::make_unique<IntLiteralExpr>(10, 2, 14), 2, 3));

    std::vector<StringPart> parts;
    parts.push_back({StringPart::TEXT, "ok", nullptr});
    body.push_back(
        std::make_unique<SayStmt>(std::make_unique<StringExpr>(std::move(parts), 3, 7), 3, 3));

    auto block = std::make_unique<BlockStmt>(std::move(body), 1, 14);

    std::unique_ptr<Stmt> expected =
        std::make_unique<WhileStmt>(std::move(condition), std::move(block), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

/**
 * aslongas (x) { aslongas (y) { } }
 * Tests nested while loops inside the body.
 */
TEST(WhileStatement, NestedWhileInBody)
{
    std::vector<Token> tokens = {
        Token("aslongas", ASLONGAS, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 10),
        Token("x", IDENTIFIER, std::monostate{}, 1, 11),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 12),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 14),

        Token("aslongas", ASLONGAS, std::monostate{}, 2, 3),
        Token("(", LEFT_PAREN, std::monostate{}, 2, 12),
        Token("y", IDENTIFIER, std::monostate{}, 2, 13),
        Token(")", RIGHT_PAREN, std::monostate{}, 2, 14),
        Token("{", LEFT_BRACE, std::monostate{}, 2, 16),
        Token("}", RIGHT_BRACE, std::monostate{}, 2, 17),

        Token("}", RIGHT_BRACE, std::monostate{}, 3, 1),
        Token("", EOF_TOKEN, std::monostate{}, 3, 2),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    // inner while
    auto                               innerCond = std::make_unique<IdentifierExpr>("y", 2, 13);
    std::vector<std::unique_ptr<Stmt>> innerBody;
    auto innerBlock = std::make_unique<BlockStmt>(std::move(innerBody), 2, 16);
    auto innerWhile =
        std::make_unique<WhileStmt>(std::move(innerCond), std::move(innerBlock), 2, 3);

    // outer while
    auto                               outerCond = std::make_unique<IdentifierExpr>("x", 1, 11);
    std::vector<std::unique_ptr<Stmt>> outerBody;
    outerBody.push_back(std::move(innerWhile));
    auto outerBlock = std::make_unique<BlockStmt>(std::move(outerBody), 1, 14);

    std::unique_ptr<Stmt> expected =
        std::make_unique<WhileStmt>(std::move(outerCond), std::move(outerBlock), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

/**
 * aslongas (x) { should (y) { say "a"; } otherwise { say "b"; } }
 * Tests if-chain inside a while body.
 */
TEST(WhileStatement, IfChainInsideBody)
{
    std::vector<Token> tokens = {
        Token("aslongas", ASLONGAS, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 10),
        Token("x", IDENTIFIER, std::monostate{}, 1, 11),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 12),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 14),

        Token("should", SHOULD, std::monostate{}, 2, 3),
        Token("(", LEFT_PAREN, std::monostate{}, 2, 10),
        Token("y", IDENTIFIER, std::monostate{}, 2, 11),
        Token(")", RIGHT_PAREN, std::monostate{}, 2, 12),
        Token("{", LEFT_BRACE, std::monostate{}, 2, 14),

        Token("say", SAY, std::monostate{}, 3, 5),
        Token("\"a\"", STRING, std::string("a"), 3, 9),
        Token(";", SEMI_COLON, std::monostate{}, 3, 12),
        Token("}", RIGHT_BRACE, std::monostate{}, 4, 3),

        Token("otherwise", OTHERWISE, std::monostate{}, 4, 5),
        Token("{", LEFT_BRACE, std::monostate{}, 4, 15),
        Token("say", SAY, std::monostate{}, 5, 5),
        Token("\"b\"", STRING, std::string("b"), 5, 9),
        Token(";", SEMI_COLON, std::monostate{}, 5, 12),
        Token("}", RIGHT_BRACE, std::monostate{}, 6, 3),

        Token("}", RIGHT_BRACE, std::monostate{}, 7, 1),
        Token("", EOF_TOKEN, std::monostate{}, 7, 2),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    // If then block: say "a";
    std::vector<std::unique_ptr<Stmt>> thenStmts;
    {
        std::vector<StringPart> parts;
        parts.push_back({StringPart::TEXT, "a", nullptr});
        thenStmts.push_back(
            std::make_unique<SayStmt>(std::make_unique<StringExpr>(std::move(parts), 3, 9), 3, 5));
    }
    auto thenBlock = std::make_unique<BlockStmt>(std::move(thenStmts), 2, 14);

    // Else block: say "b";
    std::vector<std::unique_ptr<Stmt>> elseStmts;
    {
        std::vector<StringPart> parts;
        parts.push_back({StringPart::TEXT, "b", nullptr});
        elseStmts.push_back(
            std::make_unique<SayStmt>(std::make_unique<StringExpr>(std::move(parts), 5, 9), 5, 5));
    }
    auto elseBlock = std::make_unique<BlockStmt>(std::move(elseStmts), 4, 15);

    std::vector<std::tuple<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> branches;
    branches.emplace_back(std::make_unique<IdentifierExpr>("y", 2, 11), std::move(thenBlock));

    auto ifStmt = std::make_unique<IfChainStmt>(std::move(branches), std::move(elseBlock), 2, 3);

    // While body contains the if
    auto                               cond = std::make_unique<IdentifierExpr>("x", 1, 11);
    std::vector<std::unique_ptr<Stmt>> whileBody;
    whileBody.push_back(std::move(ifStmt));
    auto block = std::make_unique<BlockStmt>(std::move(whileBody), 1, 14);

    std::unique_ptr<Stmt> expected =
        std::make_unique<WhileStmt>(std::move(cond), std::move(block), 1, 1);

    ASSERT_TRUE(isEqualStatements(actual, expected));
}

// === While error tests ===

/**
 * aslongas x) { }
 * Missing '(' after aslongas -> should error and return nullptr.
 */
TEST(WhileErrors, MissingLeftParen)
{
    std::vector<Token> tokens = {
        Token("aslongas", ASLONGAS, std::monostate{}, 1, 1),
        Token("x", IDENTIFIER, std::monostate{}, 1, 10),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 11),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 13),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 14),
        Token("", EOF_TOKEN, std::monostate{}, 1, 15),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    ASSERT_TRUE(parser.hadError());
    ASSERT_EQ(actual, nullptr);
}

/**
 * aslongas (x { }
 * Missing ')' after condition -> should error and return nullptr.
 */
TEST(WhileErrors, MissingRightParen)
{
    std::vector<Token> tokens = {
        Token("aslongas", ASLONGAS, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 10),
        Token("x", IDENTIFIER, std::monostate{}, 1, 11),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 13),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 14),
        Token("", EOF_TOKEN, std::monostate{}, 1, 15),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    ASSERT_TRUE(parser.hadError());
    ASSERT_EQ(actual, nullptr);
}

/**
 * aslongas (x) say "hi";
 * Missing '{' after condition -> should error and return nullptr.
 */
TEST(WhileErrors, MissingLeftBrace)
{
    std::vector<Token> tokens = {
        Token("aslongas", ASLONGAS, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 10),
        Token("x", IDENTIFIER, std::monostate{}, 1, 11),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 12),

        Token("say", SAY, std::monostate{}, 1, 14),
        Token("\"hi\"", STRING, std::string("hi"), 1, 18),
        Token(";", SEMI_COLON, std::monostate{}, 1, 22),

        Token("", EOF_TOKEN, std::monostate{}, 1, 23),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    ASSERT_TRUE(parser.hadError());
    ASSERT_EQ(actual, nullptr);
}

/**
 * aslongas () { }
 * Empty condition -> parseExpression should fail -> error + nullptr.
 */
TEST(WhileErrors, EmptyCondition)
{
    std::vector<Token> tokens = {
        Token("aslongas", ASLONGAS, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 10),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 11),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 13),
        Token("}", RIGHT_BRACE, std::monostate{}, 1, 14),
        Token("", EOF_TOKEN, std::monostate{}, 1, 15),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    ASSERT_TRUE(parser.hadError());
    ASSERT_EQ(actual, nullptr);
}

/**
 * aslongas (x) {
 *   say "hi"
 * }
 * Missing semicolon inside body -> should error and return nullptr.
 */
TEST(WhileErrors, BodyStatementMissingSemicolon)
{
    std::vector<Token> tokens = {
        Token("aslongas", ASLONGAS, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 10),
        Token("x", IDENTIFIER, std::monostate{}, 1, 11),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 12),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 14),

        Token("say", SAY, std::monostate{}, 2, 3),
        Token("\"hi\"", STRING, std::string("hi"), 2, 7),
        // Missing SEMI_COLON
        Token("}", RIGHT_BRACE, std::monostate{}, 3, 1),

        Token("", EOF_TOKEN, std::monostate{}, 3, 2),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    ASSERT_TRUE(parser.hadError());
    ASSERT_EQ(actual, nullptr);
}

/**
 * aslongas (x) {  EOF
 * Unterminated block -> should error and return nullptr.
 */
TEST(WhileErrors, UnterminatedBlockEOF)
{
    std::vector<Token> tokens = {
        Token("aslongas", ASLONGAS, std::monostate{}, 1, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 1, 10),
        Token("x", IDENTIFIER, std::monostate{}, 1, 11),
        Token(")", RIGHT_PAREN, std::monostate{}, 1, 12),
        Token("{", LEFT_BRACE, std::monostate{}, 1, 14),
        Token("", EOF_TOKEN, std::monostate{}, 1, 15),
    };

    Parser parser(tokens);
    auto   actual = parser.parseStatement();

    ASSERT_TRUE(parser.hadError());
    ASSERT_EQ(actual, nullptr);
}

// Parses an empty file: should produce an empty Program with hadError=false.
TEST(ParseProgram_Basics, EmptyInput)
{
    std::vector<Token> tokens = {
        Token("", EOF_TOKEN, std::monostate{}, 1, 1),
    };

    Parser parser(tokens);
    Program actual = parser.parseProgram();

    std::vector<std::unique_ptr<Stmt>> expectedStmts;
    SourceLoc start{1, 1};
    SourceLoc end{1, 1};
    Program expected(std::move(expectedStmts), false, start, end);

    ASSERT_TRUE(isEqualProgram(actual, expected));
}

// Parses a one-statement program: say "hello";
TEST(ParseProgram_Basics, SingleSayStatement)
{
    std::vector<Token> tokens = {
        Token("say", SAY, std::monostate{}, 1, 1),
        Token("\"hello\"", STRING, std::string("hello"), 1, 5),
        Token(";", SEMI_COLON, std::monostate{}, 1, 12),
        Token("", EOF_TOKEN, std::monostate{}, 1, 13),
    };

    Parser parser(tokens);
    Program actual = parser.parseProgram();

    std::vector<StringPart> parts;
    StringPart t;
    t.kind = StringPart::TEXT;
    t.text = "hello";
    parts.push_back(std::move(t));

    std::vector<std::unique_ptr<Stmt>> expectedStmts;
    expectedStmts.push_back(
        std::make_unique<SayStmt>(
            std::make_unique<StringExpr>(std::move(parts), 1, 5),
            1, 1));

    SourceLoc start{1, 1};
    SourceLoc end{1, 13};
    Program expected(std::move(expectedStmts), false, start, end);

    ASSERT_TRUE(isEqualProgram(actual, expected));
}

// Parses a multi-statement program: summon x = 10; say x;
TEST(ParseProgram_Basics, MultipleStatementsMixed)
{
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("x", IDENTIFIER, std::monostate{}, 1, 8),
        Token("=", EQUAL, std::monostate{}, 1, 10),
        Token("10", INTEGER, 10, 1, 12),
        Token(";", SEMI_COLON, std::monostate{}, 1, 14),

        Token("say", SAY, std::monostate{}, 2, 1),
        Token("x", IDENTIFIER, std::monostate{}, 2, 5),
        Token(";", SEMI_COLON, std::monostate{}, 2, 6),

        Token("", EOF_TOKEN, std::monostate{}, 2, 7),
    };

    Parser parser(tokens);
    Program actual = parser.parseProgram();

    std::vector<std::unique_ptr<Stmt>> expectedStmts;
    expectedStmts.push_back(
        std::make_unique<SummonStmt>(
            "x",
            std::make_unique<IntLiteralExpr>(10, 1, 12),
            1, 1));

    expectedStmts.push_back(
        std::make_unique<SayStmt>(
            std::make_unique<IdentifierExpr>("x", 2, 5),
            2, 1));

    SourceLoc start{1, 1};
    SourceLoc end{2, 7};
    Program expected(std::move(expectedStmts), false, start, end);

    ASSERT_TRUE(isEqualProgram(actual, expected));
}

