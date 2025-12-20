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
    Parser(const std::vector<Token>& tokens) : tokens(tokens)
    {
        current = 0;
    };

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
     */
    Token peek();

    /**
     * @brief Return the most recently consumed token.
     */
    Token previous();

    /**
     * @brief True when the parser has reached the end-of-file token.
     */
    bool isAtEnd();

    /**
     * @brief Consume and return the current token, advancing the cursor.
     */
    Token advance();

    /**
     * @brief Check whether the current token is of type `t` (without consuming).
     */
    bool check(TokenType t);

    /**
     * @brief If the current token matches `t`, consume it and return true.
     */
    bool match(TokenType t);

    /**
     * @brief Consume a token of the expected type or report an error.
     * @param t Expected token type
     * @param msg Error message used when the token does not match
     */
    Token consume(TokenType t, const std::string& msg);

    /**
     * @brief Record a parse error at a token location.
     */
    void reportError(const Token& where, const std::string& msg);

    /* Parsing helpers for precedence levels */
    std::unique_ptr<Expr> parseEquality();
    std::unique_ptr<Expr> parseComparison();
    std::unique_ptr<Expr> parseAddition();
    std::unique_ptr<Expr> parseMultiplication();
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parsePrimary();
};