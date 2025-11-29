#include "lexer.h"

#include <cctype>
#include <variant>

Lexer::Lexer(std::string source) : source(source)
{
    start = 0;
    current = 0;
};

char Lexer::advance()
{
    char current_char = source.at(current);
    current += 1;
    if (current_char == '\n')
    {
        line += 1;
        column = 1;
    }
    else
    {
        column += 1;
    }
    return current_char;
}

char Lexer::peek()
{
    if (isAtEnd())
    {
        return '\0';
    }
    return source.at(current);
}

char Lexer::peekNext()
{
    if (current + 1 >= source.length())
    {
        return '\0';
    }
    return source.at(current + 1);
}

bool Lexer::match(char expected)
{
    if (isAtEnd())
    {
        return false;
    }
    char current_char = source.at(current);
    if (current_char != expected)
    {
        return false;
    }
    current++;

    if (current_char == '\n')
    {
        line++;
        column = 1;
    }
    else
    {
        column++;
    }
    return true;
}

bool Lexer::isAtEnd()
{
    return current >= source.length();
}

Token Lexer::scanNumber()
{
    while (std::isdigit(peek()))
    {
        advance();
    }
    std::string lexeme = source.substr(start, (current - start) + 1);
    int         number = std::stoi(lexeme);
    start = current;
    return makeToken(lexeme, TokenType::INTEGER, number);
}

Token Lexer::scanIdentifierOrKeyword()
{
    while (std::isalnum(peek()) || peek() == '_')
    {
        advance();
    }
    std::string lexeme = source.substr(start, (current - start) + 1);
    start = current;
    auto it = keywordMap.find(lexeme);
    if (it != keywordMap.end())
    {
        TokenType type = it->second;
        if (lexeme == "affirmative")
        {
            return makeToken(lexeme, type, true);
        }
        else if (lexeme == "negative")
        {
            return makeToken(lexeme, type, false);
        }
        return makeToken(lexeme, type, std::monostate{});
    }
    return makeToken(lexeme, TokenType::IDENTIFIER, std::monostate{});
}

Token Lexer::scanString() {}

Token Lexer::scanMultiLineString() {}

Token Lexer::scanSlashOrComment() {}

Token Lexer::scanOperator() {}

Token Lexer::scanPunctuation() {}

Token Lexer::makeToken(std::string lexeme, TokenType type,
                       std::variant<std::monostate, int, bool, std::string> literalValue)
{
    return Token(lexeme, type, literalValue, line, column);
}

Token Lexer::errorToken(std::string message) {}

bool Lexer::isMultilineString() {}

Token Lexer::scanToken()
{
    char c = advance();

    switch (c)
    {

    case ' ':
    case '\r':
    case '\t':
        return errorToken("");
    case '\n':
        return errorToken("");
    case '"':
        if (isMultilineString())
        {
            return scanMultiLineString();
        }
        else
        {
            return scanString();
        }
    case '<':
        return scanSlashOrComment();
    case '=':
    case '>':
    case '+':
    case '*':
    case '-':
    case '/':
    case '!':
        return scanOperator();
    case ';':
        return scanPunctuation();
    case '_':
        return scanIdentifierOrKeyword();
    default:
        if (std::isdigit(c))
        {
            return scanNumber();
        }
        if (std::isalpha(c))
        {
            return scanIdentifierOrKeyword();
        }
        return errorToken("Unexpected character");
    }
}