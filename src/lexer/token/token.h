#include <string>
#include <variant>

enum TokenType
{
    // data types
    INTEGER,
    STRING,
    MULTILINE_STRING,
    BOOL,

    IDENTIFIER,

    // Keywords
    SUMMON,
    SHOULD,
    OTHERWISE,
    ASLONGAS,
    SAY,
    NOT,

    // Operators
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
    ERROR,

    SKIP
};

struct SourceLocation
{
    int line;
    int column;
};

class Token
{

  public:
    Token(std::string lexeme, TokenType type,
          std::variant<std::monostate, int, bool, std::string> value, int line, int column);

    TokenType getType() const;

    std::string getLexeme() const;

    std::variant<std::monostate, int, bool, std::string> getValue() const;

    SourceLocation getLocation() const;

  private:
    TokenType                                            type;
    std::string                                          lexeme;
    std::variant<std::monostate, int, bool, std::string> value;
    SourceLocation                                       location;
};