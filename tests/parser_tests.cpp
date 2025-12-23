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

TEST(Statement, SayStatement)
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

TEST(Statement, SayIntLiteral)
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

TEST(Statement, SayIdentifier)
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

TEST(Statement, SayBinaryExpression)
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

TEST(StatementErrors, SayMissingSemicolon)
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

TEST(StatementErrors, SayMissingExpression)
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

TEST(StatementErrors, SayInvalidExpression)
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

TEST(Statement, SummonIntLiteral)
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
TEST(Statement, SummonBoolLiteral)
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
TEST(Statement, SummonIdentifierInitializer)
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
TEST(Statement, SummonBinaryInitializerPrecedence)
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
TEST(Statement, SummonGroupedInitializer)
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
TEST(Statement, SummonUnaryInitializer)
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
TEST(Statement, SummonComparisonInitializer)
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
TEST(Statement, SummonEqualityInitializer)
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
TEST(Statement, SummonInterpolatedStringInitializer)
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
