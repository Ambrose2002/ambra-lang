#include "parser.h"

#include "ast/expr.h"

#include <cstddef>
#include <memory>

Token Parser::peek()
{
    return tokens[current];
}

Token Parser::previous()
{
    return tokens[current - 1];
}

bool Parser::isAtEnd()
{
    return peek().getType() == EOF_TOKEN;
}

Token Parser::advance()
{
    auto token = tokens[current];
    current += 1;
    return token;
}

bool Parser::check(TokenType t)
{
    if (isAtEnd())
    {
        return false;
    }
    return peek().getType() == t;
}

bool Parser::match(TokenType t)
{
    if (check(t))
    {
        advance();
        return true;
    }
    return false;
}

Token Parser::consume(TokenType t, std::string& msg)
{
    if (check(t))
    {
        auto token = advance();
        return token;
    }
    reportError(peek(), msg);
    return peek();
}

void Parser::reportError(const Token& where, const std::string& msg)
{
    hasError = true;
}

std::unique_ptr<Expr> Parser::parsePrimary()
{

    Token          token = peek();
    SourceLocation loc = token.getLocation();

    switch (token.getType())
    {
    case INTEGER:
    {
        advance();
        int value = std::get<int>(token.getValue());
        return std::make_unique<IntLiteralExpr>(value, loc.line, loc.column);
    }
    case BOOL:
    {
        advance();
        bool value = std::get<bool>(token.getValue());
        return std::make_unique<BoolLiteralExpr>(value, loc.line, loc.column);
    }
    default:
        reportError(token, "Expcted expression");
        return nullptr;
    }
}