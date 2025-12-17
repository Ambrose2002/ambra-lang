#include "ast/expr.h"
#include "lexer/lexer.h"

#include <memory>
#include <string>
#include <vector>
class Parser
{
  public:
    Parser(const std::vector<Token>& tokens) : tokens(tokens)
    {
        current = 0;
    };

    std::unique_ptr<Expr> parseExpression();
    bool                  hadError();

  private:
    const std::vector<Token>& tokens;
    size_t                    current;
    bool                      hasError;
    Token                     peek();
    Token                     previous();
    bool                      isAtEnd();
    Token                     advance();
    bool                      check(TokenType t);
    bool                      match(TokenType t);
    Token                     consume(TokenType t, const std::string& msg);
    void                      reportError(const Token& where, const std::string& msg);

    std::unique_ptr<Expr> parseEquality();
    std::unique_ptr<Expr> parseComparison();
    std::unique_ptr<Expr> parseAddition();
    std::unique_ptr<Expr> parseMultiplication();
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parsePrimary();
    std::unique_ptr<Expr> parseInterpolatedString();
};