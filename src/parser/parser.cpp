#include "parser.h"
#include <cstddef>

Token Parser::peek() {
    return tokens[current];
}

Token Parser::previous() {
    return tokens[current - 1];
}

bool Parser::isAtEnd() {
    return current >= tokens.size();
}

Token Parser::advance() {
    auto token = tokens[current];
    current += 1;
    return token;
}

bool Parser::check(TokenType t) {
    if (isAtEnd()) {
        return false;
    }
    return peek().getType() == t;
}