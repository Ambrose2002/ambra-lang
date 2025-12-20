#include "ast/expr.h"
#include "parser/parser.h"

#include <gtest/gtest.h>
#include <memory>
#include <variant>

bool isEqualExpression(const std::unique_ptr<Expr>& e1, const std::unique_ptr<Expr>& e2)
{
    if (e1 == e2)
        return true;
    if (!e1 || !e2)
        return false;
    return *e1 == *e2;
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