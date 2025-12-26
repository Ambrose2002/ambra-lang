#include "parser.h"

#include "ast/expr.h"
#include "ast/stmt.h"

#include <cstddef>
#include <cstdio>
#include <memory>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

Token Parser::peek()
{
    // Returns the current token without consuming it.
    // returns EOF token if current is out of bounds.
    if (current >= tokens.size())
    {
        return tokens.back();
    }
    return tokens[current];
}

Token Parser::peekAhead(int pos)
{
    size_t index = current + pos;

    if (index >= tokens.size())
    {
        // return the EOF token
        return tokens.back();
    }
    return tokens[index];
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

std::unique_ptr<Stmt> Parser::parseBlockStatement()
{

    Token          leftBraceToken = advance();
    SourceLocation loc = leftBraceToken.getLocation();

    std::vector<std::unique_ptr<Stmt>> statements;

    while (peek().getType() != RIGHT_BRACE)
    {

        if (peek().getType() == EOF_TOKEN)
        {
            reportError(peek(), "Expected } to close block");
            return nullptr;
        }
        std::unique_ptr<Stmt> statement = parseStatement();
        if (!statement)
        {
            return nullptr;
        }
        statements.push_back(std::move(statement));
    }
    if (!match(RIGHT_BRACE))
    {
        reportError(peek(), "Expected } to close block");
        return nullptr;
    }
    return std::make_unique<BlockStmt>(std::move(statements), loc.line, loc.column);
}

std::tuple<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>> Parser::parseConditionAndBlock()
{
    // Expect '('
    if (!match(LEFT_PAREN))
    {
        reportError(peek(), "Expected '('");
        return {nullptr, nullptr};
    }

    // Parse condition expression
    std::unique_ptr<Expr> condition = parseExpression();
    if (!condition)
    {
        return {nullptr, nullptr};
    }

    // Expect ')'
    if (!match(RIGHT_PAREN))
    {
        reportError(peek(), "Expected ')' after condition");
        return {nullptr, nullptr};
    }

    if (!check(LEFT_BRACE))
    {
        reportError(peek(), "Expected '{' to start block");
        return {nullptr, nullptr};
    }

    std::unique_ptr<Stmt> stmt = parseBlockStatement();
    if (!stmt)
    {
        return {nullptr, nullptr};
    }

    auto block = std::unique_ptr<BlockStmt>(static_cast<BlockStmt*>(stmt.release()));

    return {std::move(condition), std::move(block)};
}

std::unique_ptr<Stmt> Parser::parseIfChainStatement()
{
    // Consume 'should'
    Token          shouldToken = advance();
    SourceLocation loc = shouldToken.getLocation();

    std::vector<std::tuple<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> branches;

    // Parse first should-branch
    auto [condition, block] = parseConditionAndBlock();
    if (!condition || !block)
    {
        return nullptr;
    }

    branches.emplace_back(std::move(condition), std::move(block));

    // Parse zero or more "otherwise should" branches
    while (check(OTHERWISE))
    {
        // Lookahead to distinguish:
        //   otherwise should (...) { ... }
        //   otherwise { ... }
        if (peekAhead(1).getType() != SHOULD)
        {
            break;
        }

        advance(); // consume OTHERWISE
        advance(); // consume SHOULD

        auto [elseIfCond, elseIfBlock] = parseConditionAndBlock();
        if (!elseIfCond || !elseIfBlock)
        {
            return nullptr;
        }

        branches.emplace_back(std::move(elseIfCond), std::move(elseIfBlock));
    }

    // Optional trailing else-branch
    std::unique_ptr<BlockStmt> elseBranch = nullptr;

    if (match(OTHERWISE))
    {
        if (!check(LEFT_BRACE))
        {
            reportError(peek(), "Expected '{' after 'otherwise'");
            return nullptr;
        }

        std::unique_ptr<Stmt> stmt = parseBlockStatement();
        if (!stmt)
        {
            return nullptr;
        }

        elseBranch = std::unique_ptr<BlockStmt>(static_cast<BlockStmt*>(stmt.release()));
    }

    return std::make_unique<IfChainStmt>(std::move(branches), std::move(elseBranch), loc.line,
                                         loc.column);
}

std::unique_ptr<Stmt> Parser::parseWhileStatement()
{
    Token          aslongasToken = advance();
    SourceLocation loc = aslongasToken.getLocation();

    auto [condition, block] = parseConditionAndBlock();

    if (!condition || !block)
    {
        return nullptr;
    }

    return std::make_unique<WhileStmt>(std::move(condition), std::move(block), loc.line,
                                       loc.column);
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
    case LEFT_BRACE:
    {
        return parseBlockStatement();
    }
    case SHOULD:
    {
        return parseIfChainStatement();
    }
    case ASLONGAS:
    {
        return parseWhileStatement();
    }
    default:
        reportError(peek(), "Unexpected token");
        return nullptr;
    }
}

Program Parser::parseProgram()
{
    std::vector<std::unique_ptr<Stmt>> statements;

    // Program start location = first token (may be EOF for empty file)
    Token     firstToken = peek();
    SourceLoc startLoc{firstToken.getLocation().line, firstToken.getLocation().column};

    while (!check(EOF_TOKEN))
    {
        std::unique_ptr<Stmt> stmt = parseStatement();

        if (stmt)
        {
            statements.push_back(std::move(stmt));
        }
        else
        {
            // Skip tokens until we reach a statement boundary
            while (!check(EOF_TOKEN) && !check(SEMI_COLON) && !check(LEFT_BRACE) &&
                   !check(RIGHT_BRACE))
            {
                advance();
            }

            if (match(SEMI_COLON))
            {
                continue;
            }
        }
    }

    // Program end location = EOF token
    Token     lastToken = peek();
    SourceLoc endLoc{lastToken.getLocation().line, lastToken.getLocation().column};

    return Program(std::move(statements), hasError, startLoc, endLoc);
}