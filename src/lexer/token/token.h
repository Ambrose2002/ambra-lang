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
    INTERP_START,
    INTERP_END,
    COMMA,
    SEMI_COLON,

    EOF_TOKEN,
    ERROR
};

struct SourceLocation {
    int line;
    int column;
};

class Token
{

    public:
    
    Token (std::string lexeme, TokenType type, std::variant<int, bool, std::string> value, int line, int column);

    private:
    TokenType type;
    std::string lexeme;
    std::variant<int, bool, std::string> value;
    SourceLocation location;
};