/**
 * @file token.cpp
 * @brief Implementation of the Token class.
 *
 * This file contains the implementation of Token member functions,
 * providing construction and accessor methods for token data.
 */

#include "token.h"

/**
 * @brief Constructs a Token with the specified attributes.
 *
 * Initializes all token fields including the source location structure.
 *
 * @param lexeme Raw text from source code
 * @param type Token type classification
 * @param value Literal value or metadata (std::monostate if not applicable)
 * @param line Line number in source (1-indexed)
 * @param column Column number in source (1-indexed)
 */
Token::Token(std::string lexeme, TokenType type,
             std::variant<std::monostate, int, bool, std::string> value, int line, int column)
    : lexeme(lexeme), type(type), value(value)
{
    location = {line, column};
};

/**
 * @brief Returns the token's type.
 * @return TokenType enum value
 */
TokenType Token::getType() const
{
    return type;
};

/**
 * @brief Returns the token's lexeme (raw source text).
 * @return Lexeme string
 */
std::string Token::getLexeme() const
{
    return lexeme;
}

/**
 * @brief Returns the token's value.
 *
 * The variant holds different types depending on the token:
 * - int for INTEGER tokens
 * - bool for BOOL tokens
 * - std::string for STRING, MULTILINE_STRING, and ERROR tokens
 * - std::monostate for all other token types
 *
 * @return A variant containing the value or std::monostate
 */
std::variant<std::monostate, int, bool, std::string> Token::getValue() const
{
    return value;
}

/**
 * @brief Returns the token's source location.
 * @return SourceLocation struct with line and column numbers
 */
SourceLocation Token::getLocation() const
{
    return location;
}