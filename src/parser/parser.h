#include "ast/expr.h"
#include "ast/stmt.h"
#include "lexer/lexer.h"

#include <memory>
#include <string>
#include <vector>
class Parser
{
  public:
    Parser(const std::vector<Token>& tokens) : tokens(tokens) {};

    std::unique_ptr<Expr> parseExpression();
    bool                  hadError();

  private:
    const std::vector<Token>& tokens;
    size_t                    current;

    Token peek();
    Token previous();
    bool  isAtEnd();
    Token advance();
    bool  check(TokenType t);
    bool  match(TokenType t);
    Token  consume(TokenType t, std::string msg);
    void  reportError(const Token& where, const std::string& message);

    std::unique_ptr<Expr> parseEquality();
    std::unique_ptr<Expr> parseComparison();
    std::unique_ptr<Expr> parseAddition();
    std::unique_ptr<Expr> parseMultiplication();
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parsePrimary();
    std::unique_ptr<Expr> parseInterpolatedString();
};