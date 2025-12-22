/**
 * @file parser.h
 * @brief Recursive-descent expression parser interface.
 *
 * The Parser consumes a stream of `Token` values produced by the lexer and
 * builds an abstract syntax tree (AST) of `Expr` nodes. It implements a
 * precedence-based recursive-descent parser with helper methods for each
 * precedence level.
 */
#include "ast/expr.h"
#include "ast/stmt.h"
#include "lexer/lexer.h"

#include <memory>
#include <string>
#include <vector>

/**
 * @brief Parses tokens into expression ASTs.
 *
 * Construct a Parser with a vector of tokens (usually produced by the lexer),
 * then call `parseExpression()` to obtain the parsed AST. Errors encountered
 * during parsing are recorded; `hadError()` reports whether an error occurred.
 */
class Parser
{
  public:
    /**
     * @brief Construct a parser over an existing token sequence.
     * @param tokens Reference to the token vector to parse (must outlive parser)
     */
    Parser(const std::vector<Token>& tokens) : tokens(tokens), current(0), hasError(false) {};

    /**
     * @brief Parse the next full expression from the token stream.
     *
     * This is the top-level entry point for expression parsing. It implements
     * the highest-level grammar rule and returns a heap-allocated `Expr` or
     * `nullptr` on error.
     *
     * @return unique_ptr to the parsed `Expr`, or nullptr if parsing failed
     */
    std::unique_ptr<Expr> parseExpression();

    std::unique_ptr<Stmt> parseStatement();

    /**
     * @brief Returns true if a parse error was encountered.
     */
    bool hadError();

  private:
    const std::vector<Token>& tokens;   ///< Token stream being parsed
    size_t                    current;  ///< Current index into `tokens`
    bool                      hasError; ///< Whether a parse error occurred

    /**
     * @brief Look at the current token without consuming it.
     *
     * Does not advance the parser position.
     * Undefined behavior if `current` is out of bounds, but tokens always end with EOF token.
     *
     * @return The current token.
     */
    Token peek();

    /**
     * @brief Return the most recently consumed token.
     *
     * Assumes at least one token has been consumed; behavior is undefined if called before any
     * advance.
     *
     * @return The previous token.
     */
    Token previous();

    /**
     * @brief True when the parser has reached the end-of-file token.
     *
     * Checks whether the current token is the EOF_TOKEN.
     *
     * @return true if at end of token stream, false otherwise.
     */
    bool isAtEnd();

    /**
     * @brief Consume and return the current token, advancing the cursor.
     *
     * Moves the parser forward by one token.
     *
     * @return The consumed token.
     */
    Token advance();

    /**
     * @brief Check whether the current token is of type `t` (without consuming).
     *
     * Returns false if at end of file.
     *
     * @param t Token type to check against.
     * @return true if current token matches `t`, false otherwise.
     */
    bool check(TokenType t);

    /**
     * @brief If the current token matches `t`, consume it and return true.
     *
     * Conditionally consumes the token if it matches the expected type.
     *
     * @param t Token type to match.
     * @return true if token matched and was consumed, false otherwise.
     */
    bool match(TokenType t);

    /**
     * @brief Consume a token of the expected type or report an error.
     * @param t Expected token type
     * @param msg Error message used when the token does not match
     *
     * If the current token is not of type `t`, reports a parse error and does not advance.
     * The returned token should not be used if `hadError()` returns true.
     *
     * @return The consumed token if it matches `t`.
     */
    Token consume(TokenType t, const std::string& msg);

    /**
     * @brief Record a parse error at a token location.
     *
     * Records that a parse error has occurred and logs the error message.
     * Intended to be called exactly once per detected error.
     *
     * @param where Token location where the error was detected.
     * @param msg Error message describing the problem.
     */
    void reportError(const Token& where, const std::string& msg);

    /* Parsing helpers for precedence levels */

    /**
     * @brief Parse equality expressions (==, !=).
     *
     * Handles the equality grammar level, returning nullptr on failure.
     *
     * @return Parsed Expr or nullptr on error.
     */
    std::unique_ptr<Expr> parseEquality();

    /**
     * @brief Parse comparison expressions (<, >, <=, >=).
     *
     * Handles the comparison grammar level, returning nullptr on failure.
     *
     * @return Parsed Expr or nullptr on error.
     */
    std::unique_ptr<Expr> parseComparison();

    /**
     * @brief Parse addition and subtraction expressions (+, -).
     *
     * Handles the addition grammar level, returning nullptr on failure.
     *
     * @return Parsed Expr or nullptr on error.
     */
    std::unique_ptr<Expr> parseAddition();

    /**
     * @brief Parse multiplication and division expressions (*, /).
     *
     * Handles the multiplication grammar level, returning nullptr on failure.
     *
     * @return Parsed Expr or nullptr on error.
     */
    std::unique_ptr<Expr> parseMultiplication();

    /**
     * @brief Parse unary expressions (-, !).
     *
     * Handles the unary grammar level, returning nullptr on failure.
     *
     * @return Parsed Expr or nullptr on error.
     */
    std::unique_ptr<Expr> parseUnary();

    /**
     * @brief Parse primary expressions (literals, grouping, identifiers).
     *
     * Handles the primary grammar level, returning nullptr on failure.
     *
     * @return Parsed Expr or nullptr on error.
     */
    std::unique_ptr<Expr> parsePrimary();

    std::unique_ptr<Stmt> parseSayStatement();

    std::unique_ptr<Stmt> parseSummonStatement();
};