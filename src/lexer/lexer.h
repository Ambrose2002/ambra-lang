#include "token/token.h"

#include <string>
#include <unordered_map>
#include <vector>
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
    std::unordered_map<std::string, TokenType> keywordMap = {
        {"summon", SUMMON}, {"should", SHOULD}, {"otherwise", OTHERWISE}, {"aslongas", ASLONGAS},
        {"say", SAY},       {"not", NOT},       {"affirmative", BOOL},    {"negative", BOOL}};

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

    Token scanNumber(int startLine, int startColumn);

    Token scanIdentifierOrKeyword(int startLine, int startColumn);

    Token scanString(int startLine, int startColumn);

    Token scanMultiLineString(int startLine, int startColumn);

    Token scanSlashOrComment(int startLine, int startColumn);

    Token scanOperator(char c, int startLine, int startColumn);

    Token scanPunctuation(char c, int startLine, int startColumn);

    Token
    makeToken(TokenType type, int startLine, int startColumn,
              std::variant<std::monostate, int, bool, std::string> literalValue = std::monostate{});

    Token makeErrorToken(std::string message, int startLine, int startColumn);

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
};