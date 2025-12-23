#include "parser.h"

#include "ast/expr.h"
#include "ast/stmt.h"

#include <cstddef>
#include <memory>
#include <unordered_set>
#include <utility>

Token Parser::peek()
{
    // Returns the current token without consuming it.
    // Undefined behavior if current is out of bounds, but tokens always end with EOF_TOKEN.
    return tokens[current];
}

Token Parser::previous()
{
    // Returns the previously consumed token.
    // Assumes at least one token has been consumed (current > 0).
    return tokens[current - 1];
}

bool Parser::isAtEnd()
{
    // Returns true if the current token is EOF_TOKEN.
    return peek().getType() == EOF_TOKEN;
}

Token Parser::advance()
{
    // Consumes the current token and advances the cursor.
    auto token = tokens[current];
    current += 1;
    return token;
}

bool Parser::check(TokenType t)
{
    // Returns true if the current token is of type t.
    // Returns false if at EOF_TOKEN.
    // Does not consume the token.
    if (isAtEnd())
    {
        return false;
    }
    return peek().getType() == t;
}

bool Parser::match(TokenType t)
{
    // If the current token matches t, consumes it and returns true.
    // Otherwise returns false without consuming.
    if (check(t))
    {
        advance();
        return true;
    }
    return false;
}

// Error reporting responsibility: called when a token of expected type is required.
// On failure, reports an error and does not consume the token.
// The returned token should not be used if hadError() is true.
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

// Records a parse error at the given token location.
// Intended to be called exactly once per detected error.
void Parser::reportError(const Token& where, const std::string& msg)
{
    hasError = true;
}

bool Parser::hadError()
{
    return hasError;
}

// Parses a primary expression, the base level of the grammar.
// Returns nullptr on failure.
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
        // Parse grouping: '(' expression ')'.
        // If the closing ')' is missing, report error and return nullptr.
        advance();

        TokenType type = peek().getType();

        std::unordered_set<TokenType> allowedTokens{
            INTEGER, BOOL, STRING, MULTILINE_STRING, IDENTIFIER, LEFT_PAREN, NOT, MINUS};

        if (!allowedTokens.count(type))
        {
            reportError(peek(), "This token is not allowed here");
            return nullptr;
        }

        auto expression = parseExpression();
        if (!expression)
        {
            return nullptr;
        }
        // If we cannot find the closing ')', report and bail out.
        if (!match(RIGHT_PAREN))
        {
            reportError(peek(), "Expected ')' after expression");
            return nullptr;
        }

        if (!expression)
        {
            return nullptr;
        }

        return std::make_unique<GroupingExpr>(std::move(expression), loc.line, loc.column);
    }
    case STRING:
    case MULTILINE_STRING:
    {
        // Parse string literals with possible interpolations.
        // String parts are collected in order: text and embedded expressions.
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

            TokenType type = peek().getType();

            std::unordered_set<TokenType> allowedTokens{
                INTEGER, BOOL, STRING, MULTILINE_STRING, IDENTIFIER, LEFT_PAREN, NOT, MINUS};

            if (!allowedTokens.count(type))
            {
                reportError(peek(), "This token is not allowed here");
                return nullptr;
            }

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

// Parses unary expressions, handling chained unary operators recursively.
// Returns nullptr on failure.
std::unique_ptr<Expr> Parser::parseUnary()
{
    if (match(NOT) || match(MINUS))
    {
        Token op = previous();
        auto  loc = op.getLocation();

        auto operand = parseUnary();
        if (!operand)
        {
            return nullptr;
        }

        if (op.getType() == NOT)
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
        if (!right)
        {
            return nullptr;
        }
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

// Parses addition and subtraction expressions.
// Returns nullptr on failure.
std::unique_ptr<Expr> Parser::parseAddition()
{
    std::unique_ptr<Expr> left = parseMultiplication();

    while (check(PLUS) || check(MINUS))
    {
        Token                 op = advance();
        SourceLocation        loc = op.getLocation();
        std::unique_ptr<Expr> right = parseMultiplication();
        if (!right)
        {
            return nullptr;
        }
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

// Parses comparison expressions: <, <=, >, >=
// Returns nullptr on failure.
std::unique_ptr<Expr> Parser::parseComparison()
{
    std::unique_ptr<Expr> left = parseAddition();

    while (check(LESS) || check(LESS_EQUAL) || check(GREATER) || check(GREATER_EQUAL))
    {
        Token                 op = advance();
        SourceLocation        loc = op.getLocation();
        std::unique_ptr<Expr> right = parseAddition();
        if (!right)
        {
            return nullptr;
        }
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
            return nullptr;
        }
        }
    }
    return left;
}

// Parses equality expressions: ==, !=
// Returns nullptr on failure.
std::unique_ptr<Expr> Parser::parseEquality()
{
    std::unique_ptr<Expr> left = parseComparison();
    while (check(EQUAL_EQUAL) || check(BANG_EQUAL))
    {
        Token          op = advance();
        SourceLocation loc = op.getLocation();

        std::unique_ptr<Expr> right = parseComparison();

        if (!right)
        {
            return nullptr;
        }

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

// Top-level expression parsing entry point.
// Returns nullptr on failure.
std::unique_ptr<Expr> Parser::parseExpression()
{
    return parseEquality();
}

std::unique_ptr<Stmt> Parser::parseSayStatement()
{
    Token          sayToken = advance();
    SourceLocation loc = sayToken.getLocation();

    TokenType type = peek().getType();

    std::unordered_set<TokenType> allowedTokens{INTEGER,    BOOL,       STRING, MULTILINE_STRING,
                                                IDENTIFIER, LEFT_PAREN, NOT,    MINUS};

    if (!allowedTokens.count(type))
    {
        reportError(peek(), "This token is not allowed here");
        return nullptr;
    }

    std::unique_ptr<Expr> expression = parseExpression();

    if (!expression)
    {
        reportError(sayToken, "Expression expected.");
        return nullptr;
    }

    if (!match(SEMI_COLON))
    {
        reportError(sayToken, "Missing terminating ;");
        return nullptr;
    }

    return std::make_unique<SayStmt>(std::move(expression), loc.line, loc.column);
}

std::unique_ptr<Stmt> Parser::parseSummonStatement()
{
    Token          summonToken = advance();
    SourceLocation loc = summonToken.getLocation();

    // expect identifier
    if (!check(IDENTIFIER))
    {
        reportError(peek(), "Expected identifier after 'summon'");
        return nullptr;
    }

    Token       nameToken = advance();
    std::string variableName = nameToken.getLexeme();

    // expect '='
    if (!match(EQUAL))
    {
        reportError(peek(), "Expected '=' after variable name in summon statement");
        return nullptr;
    }

    TokenType type = peek().getType();

    std::unordered_set<TokenType> allowedTokens{INTEGER,    BOOL,       STRING, MULTILINE_STRING,
                                                IDENTIFIER, LEFT_PAREN, NOT,    MINUS};

    if (!allowedTokens.count(type))
    {
        reportError(peek(), "This token is not allowed here");
        return nullptr;
    }

    // parse initializer expression
    std::unique_ptr<Expr> initializer = parseExpression();
    if (!initializer)
    {
        reportError(peek(), "Expected initializer expression after '='");
        return nullptr;
    }

    // expect ';'
    if (!match(SEMI_COLON))
    {
        reportError(peek(), "Expected ';' after summon statement");
        return nullptr;
    }

    return std::make_unique<SummonStmt>(variableName, std::move(initializer), loc.line, loc.column);
}

std::unique_ptr<Stmt> Parser::parseStatement()
{
    Token token = peek();

    switch (token.getType())
    {
    case SAY:
    {
        return parseSayStatement();
    }
    case SUMMON:
    {
        return parseSummonStatement();
    }
    default:
        return nullptr;
    }
}