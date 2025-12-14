#include "ast/stmt.h"
#include "ast/expr.h"
#include "lexer/lexer.h"
#include <vector>
class Parser {
    public:
    Parser(std::vector<Token> tokens): tokens(tokens){};

    private:
    std::vector<Token> tokens;
};