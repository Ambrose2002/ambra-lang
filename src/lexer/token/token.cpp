#include "token.h"

Token::Token(std::string lexeme, TokenType type,
             std::variant<std::monostate, int, bool, std::string> value, int line, int column)
    : lexeme(lexeme), type(type), value(value)
{
    location = {line, column};
};

TokenType Token::getType() const
{
    return type;
};

std::string Token::getLexeme() const
{
    return lexeme;
}

std::variant<std::monostate, int, bool, std::string> Token::getValue() const
{
    return value;
}

SourceLocation Token::getLocation() const
{
    return location;
}