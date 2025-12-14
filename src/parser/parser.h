#include "ast/stmt.h"
#include "ast/expr.h"
#include "lexer/lexer.h"
#include <vector>
class Parser {
    public:
    Parser(const std::vector<Token>& tokens): tokens(tokens){};

    void parseExpression();

    bool hasError();

    private:
    const std::vector<Token>& tokens;
    size_t current;
};