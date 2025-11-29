#include "lexer.h"

Lexer::Lexer(std::string source) : source(source) {
    start = 0;
    current = 0;
};

char Lexer::advance() {
    char current_char = source.at(current);
    current += 1;
    if (current_char == '\n') {
        line += 1;
        column = 1;
    } else {
        column += 1;
    }
    return current_char;
}

char Lexer::peek() {
    if (isAtEnd()) {
        return '\0';
    }
    return source.at(current);
}

char Lexer::peekNext() {
    if (current + 1 >= source.length()) {
        return '\0';
    }
    return source.at(current + 1);
}

bool Lexer::match(char expected) {
    if (isAtEnd()) {
        return false;
    }
    char current_char = source.at(current);
    if (current_char != expected) {
        return false;
    }
    current ++;

    if (current_char == '\n') {
        line ++;
        column = 1;
    } else {
        column ++;
    }
    return true;
}

bool Lexer::isAtEnd() {
    return current >= source.length();
}
