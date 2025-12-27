#include <string>
#include <variant>

/**
 * @file token.h
 * @brief Token type definitions and Token class for the Ambra lexer.
 *
 * This file defines the TokenType enumeration containing all recognized
 * token types in the Ambra language, including literals, keywords,
 * operators, and punctuation. It also defines the Token class which
 * represents a single lexical unit with its type, lexeme, value, and
 * source location.
 */

/**
 * @brief Enumeration of all token types in the Ambra language.
 *
 * Token types are grouped by category:
 * - Data types: INTEGER, STRING, MULTILINE_STRING, BOOL
 * - Identifiers and keywords: IDENTIFIER, SUMMON, SHOULD, etc.
 * - Operators: PLUS, MINUS, STAR, SLASH, comparisons
 * - Punctuation: Parentheses, braces, delimiters
 * - Special: EOF_TOKEN, ERROR, SKIP (for internal use)
 */
enum TokenType
{
    // Data types
    INTEGER,          ///< Integer literal
    STRING,           ///< String literal (single or double quoted)
    MULTILINE_STRING, ///< Multiline string literal (triple quoted)
    BOOL,             ///< Boolean literal (affirmative/negative)

    IDENTIFIER, ///< User-defined identifier

    // Keywords
    SUMMON,    ///< Variable declaration keyword
    SHOULD,    ///< Conditional (if) keyword
    OTHERWISE, ///< Else keyword
    ASLONGAS,  ///< Loop (while) keyword
    SAY,       ///< Print statement keyword
    NOT,       ///< Logical negation keyword

    // Operators
    PLUS,          ///< Addition operator (+)
    MINUS,         ///< Subtraction operator (-)
    STAR,          ///< Multiplication operator (*)
    SLASH,         ///< Division operator (/)
    EQUAL,         ///< Assignment operator (=)
    EQUAL_EQUAL,   ///< Equality comparison (==)
    BANG_EQUAL,    ///< Inequality comparison (!=)
    LESS,          ///< Less than (<)
    LESS_EQUAL,    ///< Less than or equal (<=)
    GREATER,       ///< Greater than (>)
    GREATER_EQUAL, ///< Greater than or equal (>=)

    // Punctuation
    LEFT_PAREN,   ///< Opening parenthesis '('
    RIGHT_PAREN,  ///< Closing parenthesis ')'
    LEFT_BRACE,   ///< Opening brace '{'
    RIGHT_BRACE,  ///< Closing brace '}'
    INTERP_START, ///< Start of string interpolation '{' (in string context)
    INTERP_END,   ///< End of string interpolation '}' (in interpolation context)
    COMMA,        ///< Comma separator ','
    SEMI_COLON,   ///< Semicolon terminator ';'

    // Special
    EOF_TOKEN, ///< End of file marker
    ERROR,     ///< Error token (contains error message as value)

    SKIP ///< Internal token type for whitespace/comments (not emitted)
};

/**
 * @brief Represents a position in the source code.
 *
 * Used to track where tokens appear for error reporting and debugging.
 * Both line and column numbers are 1-indexed.
 */
struct SourceLocation
{
    int line;   ///< Line number (1-indexed)
    int column; ///< Column number (1-indexed)
};

/**
 * @brief Represents a single lexical token in the Ambra language.
 *
 * A Token encapsulates:
 * - type: The classification of the token (keyword, operator, literal, etc.)
 * - lexeme: The raw text from the source that formed this token
 * - value: The interpreted value (for literals) or metadata (for errors)
 * - location: Source position for error reporting
 *
 * Tokens are immutable once created and are the primary output of the lexer.
 */
class Token
{

  public:
    /**
     * @brief Constructs a new Token.
     *
     * @param lexeme The raw text from source code
     * @param type The token type classification
     * @param value The literal value (int, bool, string) or std::monostate if none
     * @param line Line number where token appears (1-indexed)
     * @param column Column number where token appears (1-indexed)
     */
    Token(std::string lexeme, TokenType type,
          std::variant<std::monostate, int, bool, std::string> value, int line, int column);

    /**
     * @brief Gets the token's type.
     * @return The TokenType enum value
     */
    TokenType getType() const;

    /**
     * @brief Gets the token's lexeme (raw source text).
     * @return The lexeme string
     */
    std::string getLexeme() const;

    /**
     * @brief Gets the token's interpreted value.
     *
     * For literals, this contains the parsed value (int, bool, or string).
     * For ERROR tokens, this contains the error message.
     * For most other tokens, this is std::monostate.
     *
     * @return A variant containing the value or std::monostate
     */
    std::variant<std::monostate, int, bool, std::string> getValue() const;

    /**
     * @brief Gets the token's source location.
     * @return A SourceLocation struct with line and column
     */
    SourceLocation getLocation() const;

  private:
    TokenType                                            type;   ///< Token classification
    std::string                                          lexeme; ///< Raw source text
    std::variant<std::monostate, int, bool, std::string> value;  ///< Interpreted value or metadata
    SourceLocation                                       location; ///< Source position
};