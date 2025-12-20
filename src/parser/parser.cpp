#include "parser.h"

#include "ast/expr.h"

#include <cstddef>
#include <memory>
#include <utility>

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

Token Parser::consume(TokenType t, const std::string& msg)
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
    case IDENTIFIER:
    {
        advance();
        std::string name = token.getLexeme();
        return std::make_unique<IdentifierExpr>(name, loc.line, loc.column);
    }
    case LEFT_PAREN:
    {
        advance();
        auto expression = parseExpression();
        consume(RIGHT_PAREN, "Expected ')' after expression");
        return std::make_unique<GroupingExpr>(std::move(expression), loc.line, loc.column);
    }
    case STRING:
    case MULTILINE_STRING:
    {
        std::vector<StringPart> parts;

        // Consume initial string token
        Token          strToken = advance();
        SourceLocation loc = strToken.getLocation();

        StringPart textPart;
        textPart.kind = StringPart::TEXT;
        textPart.text = std::get<std::string>(strToken.getValue());
        parts.push_back(std::move(textPart));

        // Handle interpolations
        while (peek().getType() == INTERP_START)
        {
            Token interpStart = advance(); // consume '{'

            std::unique_ptr<Expr> expr = parseExpression();

            if (peek().getType() != INTERP_END)
            {
                reportError(interpStart, "Unterminated interpolation");
                return nullptr;
            }

            advance(); // consume '}'

            StringPart exprPart;
            exprPart.kind = StringPart::EXPR;
            exprPart.expr = std::move(expr);
            parts.push_back(std::move(exprPart));

            // Expect another string chunk
            if (peek().getType() != STRING && peek().getType() != MULTILINE_STRING)
            {
                reportError(peek(), "Expected string after interpolation");
                return nullptr;
            }

            Token nextStr = advance();

            StringPart nextText;
            nextText.kind = StringPart::TEXT;
            nextText.text = std::get<std::string>(nextStr.getValue());
            parts.push_back(std::move(nextText));
        }

        return std::make_unique<StringExpr>(std::move(parts), loc.line, loc.column);
    }
    default:
        reportError(token, "Expected expression");
        return nullptr;
    }
}

std::unique_ptr<Expr> Parser::parseUnary()
{
    Token token = peek();
    auto  loc = token.getLocation();
    if (match(NOT) || match(MINUS))
    {
        advance();
        auto operand = parseUnary();
        if (token.getType() == NOT)
        {
            return std::make_unique<UnaryExpr>(LogicalNot, std::move(operand), loc.line,
                                               loc.column);
        }
        return std::make_unique<UnaryExpr>(ArithmeticNegate, std::move(operand), loc.line,
                                           loc.column);
    }
    return parsePrimary();
}

std::unique_ptr<Expr> Parser::parseMultiplication()
{
    std::unique_ptr<Expr> initial = parseUnary();

    while (check(STAR) || check(SLASH))
    {
        Token                 op = advance();
        SourceLocation        loc = op.getLocation();
        std::unique_ptr<Expr> right = parseUnary();
        if (op.getType() == STAR)
        {
            initial = std::make_unique<BinaryExpr>(std::move(initial), Multiply, std::move(right),
                                                   loc.line, loc.column);
        }
        else
        {
            initial = std::make_unique<BinaryExpr>(std::move(initial), Divide, std::move(right),
                                                   loc.line, loc.column);
        }
    }
    return initial;
}

std::unique_ptr<Expr> Parser::parseAddition()
{
    std::unique_ptr<Expr> left = parseMultiplication();

    while (check(PLUS) || check(MINUS))
    {
        Token                 op = advance();
        SourceLocation        loc = op.getLocation();
        std::unique_ptr<Expr> right = parseMultiplication();
        if (op.getType() == MINUS)
        {
            left = std::make_unique<BinaryExpr>(std::move(left), Subtract, std::move(right),
                                                loc.line, loc.column);
        }
        else
        {
            left = std::make_unique<BinaryExpr>(std::move(left), Add, std::move(right), loc.line,
                                                loc.column);
        }
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseComparison()
{
    std::unique_ptr<Expr> left = parseAddition();

    while (check(LESS) || check(LESS_EQUAL) || check(GREATER) || check(GREATER_EQUAL))
    {
        Token                 op = advance();
        SourceLocation        loc = op.getLocation();
        std::unique_ptr<Expr> right = parseAddition();

        switch (op.getType())
        {
        case LESS:
        {
            left = std::make_unique<BinaryExpr>(std::move(left), Less, std::move(right), loc.line,
                                                loc.column);
            break;
        }
        case LESS_EQUAL:
        {
            left = std::make_unique<BinaryExpr>(std::move(left), LessEqual, std::move(right),
                                                loc.line, loc.column);
            break;
        }
        case GREATER:
        {
            left = std::make_unique<BinaryExpr>(std::move(left), Greater, std::move(right),
                                                loc.line, loc.column);
            break;
        }
        case GREATER_EQUAL:
        {
            left = std::make_unique<BinaryExpr>(std::move(left), GreaterEqual, std::move(right),
                                                loc.line, loc.column);
            break;
        }
        default:
        {
            reportError(op, "Invalid comparison operator");
            return left;
        }
        }
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseEquality()
{
    std::unique_ptr<Expr> left = parseComparison();
    while (check(EQUAL_EQUAL) || check(BANG_EQUAL))
    {
        Token          op = advance();
        SourceLocation loc = op.getLocation();

        std::unique_ptr<Expr> right = parseComparison();
        if (op.getType() == EQUAL_EQUAL)
        {
            left = std::make_unique<BinaryExpr>(std::move(left), EqualEqual, std::move(right),
                                                loc.line, loc.column);
        }
        else
        {
            left = std::make_unique<BinaryExpr>(std::move(left), NotEqual, std::move(right),
                                                loc.line, loc.column);
        }
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseExpression()
{
    return parseEquality();
}