#include "ast/expr.h"
#include "ast/stmt.h"
#include "lexer/lexer.h"

#include <string>
#include <vector>
class Parser
{
  public:
    Parser(const std::vector<Token>& tokens) : tokens(tokens) {};

    void parseExpression();

    bool hadError();

  private:
    const std::vector<Token>& tokens;
    size_t                    current;

    Token peek();
    Token previous();
    bool  isAtEnd();
    Token advance();
    bool  check(TokenType t);
    bool  match();
    void  consume(TokenType t, std::string msg);
    void  reportError(const Token& where, const std::string& message);
};