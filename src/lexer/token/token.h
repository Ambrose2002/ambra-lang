#include <iostream>
#include <variant>
#include <string>

enum TokenType
{
    INTEGER,
    STRING,
    MULTILINE_STRING,
    BOOL,

    IDENTIFIER,

    SUMMON,
    SHOULD,
    OTHERWISE,
    ASLONGAS,
    SAY,
    NOT,

    PLUS,
    MINUS,
    STAR,
    SLASH,
    EQUAL,
    EQUAL_EQUAL,
    BANG_EQUAL,
    LESS,
    LESS_EQUAL,
    GREATER,
    GREATER_EQUAL,

    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_BRACE,
    RIGHT_BRACE,
    COMMA,
    SEMI_COLON,

    EOF_TOKEN,
    ERROR
};

class Token
{

    public:
    
    Token ();

    private:
    TokenType type;
    std::string lexeme;
    std::variant<int, bool, std::string> value;

};