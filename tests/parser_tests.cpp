// Tests for the parser component of the Ambra language.
// These tests cover expression parsing, including single-token expressions,
// operator precedence and associativity, grouping with parentheses, unary expressions,
// comparison and equality expressions, string interpolation, and error handling.

#include "ast/expr.h"
#include "parser/parser.h"

#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <variant>

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
        Token("-", MINUS, std::monostate{}, 1, 1),
        Token("1", INTEGER, 1, 1, 2),
        Token("*", STAR, std::monostate{}, 1, 4),
        Token("2", INTEGER, 2, 1, 6),
        Token("", EOF_TOKEN, std::monostate{}, 1, 7),
    };

    Parser parser(tokens);
    auto actual = parser.parseExpression();

    std::unique_ptr<Expr> expected = std::make_unique<BinaryExpr>(
        std::make_unique<UnaryExpr>(
            ArithmeticNegate,
            std::make_unique<IntLiteralExpr>(1, 1, 2),
            1, 1),
        Multiply,
        std::make_unique<IntLiteralExpr>(2, 1, 6),
        1, 4);

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
    auto actual = parser.parseExpression();

    std::unique_ptr<Expr> expected = std::make_unique<UnaryExpr>(
        LogicalNot,
        std::make_unique<UnaryExpr>(
            LogicalNot,
            std::make_unique<BoolLiteralExpr>(true, 1, 9),
            1, 5),
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
        Token("1", INTEGER, 1, 1, 1),
        Token("<", LESS, std::monostate{}, 1, 3),
        Token("2", INTEGER, 2, 1, 5),
        Token("<", LESS, std::monostate{}, 1, 7),
        Token("3", INTEGER, 3, 1, 9),
        Token("", EOF_TOKEN, std::monostate{}, 1, 10),
    };

    Parser parser(tokens);
    auto actual = parser.parseExpression();

    std::unique_ptr<Expr> expected = std::make_unique<BinaryExpr>(
        std::make_unique<BinaryExpr>(
            std::make_unique<IntLiteralExpr>(1, 1, 1),
            Less,
            std::make_unique<IntLiteralExpr>(2, 1, 5),
            1, 3),
        Less,
        std::make_unique<IntLiteralExpr>(3, 1, 9),
        1, 7);

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
        Token("(", LEFT_PAREN, std::monostate{}, 1, 1),
        Token("1", INTEGER, 1, 1, 2),
        Token("+", PLUS, std::monostate{}, 1, 4),
        Token("2", INTEGER, 2, 1, 6),
        Token("", EOF_TOKEN, std::monostate{}, 1, 7),
    };

    Parser parser(tokens);
    auto result = parser.parseExpression();

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
    auto result = parser.parseExpression();

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
    auto result = parser.parseExpression();

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
    auto actual = parser.parseExpression();

    std::unique_ptr<Expr> expected = std::make_unique<UnaryExpr>(
        ArithmeticNegate,
        std::make_unique<GroupingExpr>(
            std::make_unique<BinaryExpr>(
                std::make_unique<IntLiteralExpr>(1, 1, 3),
                Add,
                std::make_unique<IntLiteralExpr>(2, 1, 7),
                1, 5),
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
    auto actual = parser.parseExpression();

    std::unique_ptr<Expr> expected =
        std::make_unique<BinaryExpr>(
            std::make_unique<IntLiteralExpr>(1, 1, 1),
            EqualEqual,
            std::make_unique<IntLiteralExpr>(2, 1, 6),
            1, 3);

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
    auto actual = parser.parseExpression();

    std::unique_ptr<Expr> expected =
        std::make_unique<BinaryExpr>(
            std::make_unique<BinaryExpr>(
                std::make_unique<IntLiteralExpr>(1, 1, 1),
                Less,
                std::make_unique<IntLiteralExpr>(2, 1, 5),
                1, 3),
            EqualEqual,
            std::make_unique<BoolLiteralExpr>(true, 1, 10),
            1, 7);

    ASSERT_TRUE(isEqualExpression(actual, expected));
}