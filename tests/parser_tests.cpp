#include "ast/expr.h"
#include "parser/parser.h"

#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <variant>

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

TEST(SingleTokenExpression, BoolLiteral)
{
    std::vector<Token> tokens = {Token("affirmative", BOOL, true, 1, 1),
                                 Token("", EOF_TOKEN, std::monostate{}, 1, 12)};

    std::unique_ptr<Expr> expected = std::make_unique<BoolLiteralExpr>(true, 1, 1);

    Parser parser(tokens);

    auto actual = parser.parseExpression();
    ASSERT_TRUE(isEqualExpression(actual, expected));
}

TEST(SingleTokenExpression, Identifer)
{
    std::vector<Token> tokens = {Token("x1", IDENTIFIER, std::monostate{}, 1, 1),
                                 Token("", EOF_TOKEN, std::monostate{}, 1, 3)};

    std::unique_ptr<Expr> expected = std::make_unique<IdentifierExpr>("x1", 1, 1);

    Parser parser(tokens);
    auto   actual = parser.parseExpression();
    ASSERT_TRUE(isEqualExpression(actual, expected));
}

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