#include "token/token.h"

#include <string>
#include <unordered_map>
#include <vector>

enum LexerMode
{
    NORMAL_MODE,
    STRING_MODE,
    INTERP_EXPR_MODE,
    MULTILINE_STRING_MODE
};
/**
 * @brief The Lexer (scanner) for the Ambra language.
 *
 * The Lexer is responsible for converting raw source code into a
 * sequence of Tokens that the parser can consume. It performs
 * character-by-character scanning, grouping characters into lexemes
 * according to the lexical rules defined in LANGUAGE_SPEC.md.
 *
 * Whitespace and comments are skipped. All other meaningful sequences
 * (identifiers, keywords, literals, operators, punctuation) are
 * converted into strongly-typed Token objects with associated source
 * location information.
 *
 * This class implements a standard single-pass, stateful scanner.
 */
class Lexer
{
  private:
    /**
     * @brief The full source code being scanned.
     *
     * Stored as a const string to prevent modification during lexing.
     */
    const std::string source;

    /**
     * @brief Index of the first character of the current token.
     *
     * Used to extract the lexeme substring after scanning a token.
     */
    int start;

    /**
     * @brief Index of the character currently being examined.
     *
     * Always points to the next unprocessed character in the source.
     */
    int current;

    /**
     * @brief Current line number (starting at 1).
     *
     * Updated whenever a newline character is consumed. Used for
     * accurate error reporting in tokens.
     */
    int line = 1;

    /**
     * @brief Current column number (starting at 1).
     *
     * Reset after each newline. Tracks horizontal position within a line.
     */
    int column = 1;

    /**
     * @brief Line number where the current interpolation started.
     *
     * Used for error reporting when an interpolation is unterminated.
     */
    int interpStartLine = 0;

    /**
     * @brief Column number where the current interpolation started.
     *
     * Used for error reporting when an interpolation is unterminated.
     */
    int interpStartColumn = 0;

    /**
     * @brief Character position where the current interpolation started.
     *
     * Used for error reporting when an interpolation is unterminated.
     */
    int interpStart = 0;

    /**
     * @brief Tracks whether we're currently inside a multiline string.
     *
     * This flag helps determine whether to resume multiline string mode
     * after an interpolation ends.
     */
    bool insideMultiline;

    /**
     * @brief Current scanning mode of the lexer.
     *
     * The lexer operates in different modes depending on context:
     * - NORMAL_MODE: Regular code tokenization
     * - STRING_MODE: Inside a string literal, scanning for interpolations or closing quote
     * - INTERP_EXPR_MODE: Inside an interpolation expression {...}
     * - MULTILINE_STRING_MODE: Inside a triple-quoted multiline string
     */
    LexerMode mode;
    /**
     * @brief Advances the scanner by one character and returns it.
     *
     * Also updates column/line counters as appropriate.
     */
    char advance();

    /**
     * @brief Returns the current unconsumed character without advancing.
     */
    char peek();

    /**
     * @brief Returns the next character after the current one.
     *
     * Used for lookahead (e.g., distinguishing '=' from '==').
     */
    char peekNext();

    /**
     * @brief Looks ahead a specified number of positions from current.
     *
     * @param pos Number of positions ahead to look
     * @return The character at current + pos, or '\0' if out of bounds
     */
    char peekAhead(int pos);

    /**
     * @brief Consumes the next character only if it matches `expected`.
     *
     * Used for scanning multi-character operators like "==", "!=", "<=", ">=".
     */
    bool match(char expected);

    /**
     * @brief Checks whether the end of the source text has been reached.
     */
    bool isAtEnd();

    /**
     * @brief Scans a numeric literal token.
     *
     * Consumes digits until a non-digit is encountered. If followed by
     * letters or underscores, the entire sequence is consumed and an
     * error token is returned.
     *
     * @param startLine Line where the number started
     * @param startColumn Column where the number started
     * @return INTEGER token with the parsed value, or ERROR token
     */
    Token scanNumber(int startLine, int startColumn);

    /**
     * @brief Scans an identifier or keyword token.
     *
     * Consumes alphanumeric characters and underscores. Checks the
     * resulting lexeme against the keyword map to determine if it's
     * a reserved word. Special handling for boolean literals
     * "affirmative" and "negative".
     *
     * @param startLine Line where the identifier started
     * @param startColumn Column where the identifier started
     * @return IDENTIFIER token, keyword token, or BOOL token
     */
    Token scanIdentifierOrKeyword(int startLine, int startColumn);

    /**
     * @brief Scans a string literal, handling interpolations.
     *
     * Processes string content until reaching a closing quote or
     * interpolation marker '{'. Handles both initial entry (from opening
     * quote) and resumption (after interpolation ends). Returns SKIP
     * token when resuming and immediately encountering another interpolation.
     *
     * @param startLine Line where the string/segment started
     * @param startColumn Column where the string/segment started
     * @return STRING token with literal value, or ERROR if unterminated
     */
    Token scanString(int startLine, int startColumn);

    /**
     * @brief Scans a multiline string literal (triple-quoted).
     *
     * Processes content within triple quotes ("""), allowing newlines.
     * Handles interpolations within multiline strings. Can be called
     * initially or when resuming after an interpolation.
     *
     * @param startLine Line where the multiline string/segment started
     * @param startColumn Column where the multiline string/segment started
     * @return MULTILINE_STRING token, or ERROR if unterminated
     */
    Token scanMultiLineString(int startLine, int startColumn);

    /**
     * @brief Scans a comment (single-line or multi-line).
     *
     * Single-line comments start with '</' and continue to end of line.
     * Multi-line comments start with '</\n' and end with '/>'.
     *
     * @param startLine Line where the comment started
     * @param startColumn Column where the comment started
     * @return SKIP token, or ERROR if multi-line comment is unterminated
     */
    Token scanSlashOrComment(int startLine, int startColumn);

    /**
     * @brief Scans an operator token.
     *
     * Handles arithmetic operators (+, -, *, /) and comparison operators
     * (=, ==, !=, <, <=, >, >=). Multi-character operators are recognized
     * using lookahead.
     *
     * @param c The first character of the operator
     * @param startLine Line where the operator started
     * @param startColumn Column where the operator started
     * @return The corresponding operator token, or ERROR for unary '!'
     */
    Token scanOperator(char c, int startLine, int startColumn);

    /**
     * @brief Scans a punctuation token.
     *
     * Handles delimiters like parentheses, braces, semicolons, and commas.
     * Special handling for braces in string/interpolation contexts:
     * - '{' in STRING_MODE becomes INTERP_START
     * - '}' in INTERP_EXPR_MODE becomes INTERP_END
     *
     * @param c The punctuation character
     * @param startLine Line where the punctuation started
     * @param startColumn Column where the punctuation started
     * @return The corresponding punctuation token or special interpolation token
     */
    Token scanPunctuation(char c, int startLine, int startColumn);

    /**
     * @brief Constructs a token from the current lexeme.
     *
     * Extracts the lexeme from source[start..current) and creates a Token
     * with the specified type, location, and optional literal value.
     *
     * @param type The token type
     * @param startLine Line where the token started
     * @param startColumn Column where the token started
     * @param literalValue Optional literal value (for integers, bools, strings)
     * @return The constructed Token
     */
    Token
    makeToken(TokenType type, int startLine, int startColumn,
              std::variant<std::monostate, int, bool, std::string> literalValue = std::monostate{});

    /**
     * @brief Creates an error token with a diagnostic message.
     *
     * Extracts the problematic lexeme and creates an ERROR token containing
     * the error message as its value.
     *
     * @param message Description of the error
     * @param startLine Line where the error occurred
     * @param startColumn Column where the error occurred
     * @return An ERROR token with the message as its value
     */
    Token makeErrorToken(std::string message, int startLine, int startColumn);

    /**
     * @brief Checks if the current position starts a multiline string.
     *
     * After consuming the first '"' character, this checks whether the
     * next two characters are also '"', indicating triple-quote syntax.
     *
     * @return true if the next two characters are both '"'
     */
    bool isMultilineString();

  public:
    /**
     * @brief Constructs a Lexer for the given source text.
     *
     * @param source The raw Ambra program to be tokenized.
     */
    Lexer(std::string source);

    /**
     * @brief Scans the entire source and produces a list of Tokens.
     *
     * This is the main entry point for the lexer. It repeatedly invokes
     * the internal scanning routines until the end of the input is reached,
     * then appends an EOF token.
     *
     * @return A vector containing the full token stream.
     */
    std::vector<Token> scanTokens();

    /**
     * @brief Scans a single token from the source.
     *
     * This is the core routine of the lexer. It reads one logical token
     * starting at the current scanner position, consuming as many characters
     * as necessary to fully recognize the token. Depending on the leading
     * character, this method dispatches to specialized scanning routines
     * (e.g., number literals, identifiers/keywords, string literals,
     * operators, or comments).
     *
     * Whitespace and comments are skipped and do not produce tokens; in
     * those cases, scanToken() advances the scanner and returns no token.
     * The caller (scanTokens) decides whether to append the token to the
     * token stream or ignore the result.
     *
     * If the end of input is reached, this method returns an EOF token.
     * If an unexpected or invalid character is encountered, an ERROR token
     * is produced, annotated with source location information.
     *
     * @return A Token representing the next meaningful unit in the source.
     */
    Token scanToken();

    /**
     * @brief Mapping of language keywords to their corresponding token types.
     *
     * During identifier scanning, the lexer must determine whether a scanned
     * lexeme is a user-defined identifier or a reserved keyword with special
     * syntactic meaning. All reserved words of the Ambra language are stored
     * in this table, allowing the lexer to perform an O(1) lookup after
     * scanning an identifier.
     *
     * When an identifier lexeme matches an entry in this table, the lexer
     * emits the associated keyword TokenType instead of IDENTIFIER. Boolean
     * keywords ("affirmative" and "negative") are also included here and are
     * handled specially by the identifier-scanning routine to produce BOOL
     * literal tokens with the appropriate boolean value.
     *
     * This table is initialized once (typically in the constructor) and is
     * treated as read-only throughout lexing.
     */
    static std::unordered_map<std::string, TokenType> keywordMap;
};