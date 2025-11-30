#include "lexer.h"

#include <cctype>
#include <variant>

Lexer::Lexer(std::string source) : source(source)
{
    start = 0;
    current = 0;
};

char Lexer::advance()
{
    char current_char = source.at(current);
    current += 1;
    if (current_char == '\n')
    {
        line += 1;
        column = 1;
    }
    else
    {
        column += 1;
    }
    return current_char;
}

char Lexer::peek()
{
    if (isAtEnd())
    {
        return '\0';
    }
    return source.at(current);
}

char Lexer::peekNext()
{
    if (current + 1 >= source.length())
    {
        return '\0';
    }
    return source.at(current + 1);
}

char Lexer::peekAhead(int pos)
{
    if (current + pos >= source.length())
    {
        return '\0';
    }
    return source.at(current + pos);
}

bool Lexer::match(char expected)
{
    if (isAtEnd())
    {
        return false;
    }
    char current_char = source.at(current);
    if (current_char != expected)
    {
        return false;
    }
    current++;

    if (current_char == '\n')
    {
        line++;
        column = 1;
    }
    else
    {
        column++;
    }
    return true;
}

bool Lexer::isAtEnd()
{
    return current >= source.length();
}

Token Lexer::scanNumber(int startLine, int startColumn)
{
    while (!isAtEnd() && std::isdigit(peek()))
    {
        advance();
    }
    std::string lexeme = source.substr(start, current - start);
    int         number = std::stoi(lexeme);
    return makeToken(TokenType::INTEGER, startLine, startColumn, number);
}

Token Lexer::scanIdentifierOrKeyword(int startLine, int startColumn)
{
    while (!isAtEnd() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_'))
    {
        advance();
    }

    std::string lexeme = source.substr(start, current - start);

    auto it = keywordMap.find(lexeme);
    if (it != keywordMap.end())
    {
        TokenType type = it->second;

        if (lexeme == "affirmative")
        {
            return makeToken(type, startLine, startColumn, true);
        }
        else if (lexeme == "negative")
        {
            return makeToken(type, startLine, startColumn, false);
        }

        return makeToken(type, startLine, startColumn, std::monostate{});
    }

    return makeToken(TokenType::IDENTIFIER, startLine, startColumn, std::monostate{});
}

Token Lexer::scanString(int startLine, int startColumn)
{
    // Scan until closing quote or error
    while (true)
    {
        if (isAtEnd())
        {
            return makeErrorToken("Unterminated string", line, column);
        }

        char c = peek();

        if (c == '"')
        {
            // Found the closing character — stop the loop
            break;
        }

        if (c == '\n')
        {
            return makeErrorToken("Unterminated string", line, column);
        }

        advance();

        // Consume the closing quote
        advance();

        std::string lexeme = source.substr(start + 1, (current - start - 2));
        return makeToken(STRING, startLine, startColumn, lexeme);
    }
}

Token Lexer::scanMultiLineString(int startLine, int startColumn)
{
    // Consume the opening triple quotes: """
    advance();
    advance();
    advance();

    while (true)
    {
        if (isAtEnd())
        {
            return makeErrorToken("Unterminated string", line, column);
        }

        // Found closing triple quote
        if (peek() == '"' && peekNext() == '"' && peekAhead(2) == '"')
        {
            break;
        }

        advance();
    }

    // Extract string content (excluding """ at end)
    std::string lexeme = source.substr(start, current - start);

    // Consume closing triple quotes
    advance();
    advance();
    advance();

    return makeToken(MULTILINE_STRING, startLine, startColumn, lexeme);
}

Token Lexer::scanSlashOrComment(int startLine, int startColumn)
{
    advance(); // consume the '/'

    bool isMultiLine = false;

    if (!isAtEnd() && peek() == '\n')
    {
        // Multi-line comment begins with "</\n"
        isMultiLine = true;
        advance();
    }

    if (!isMultiLine)
    {
        // Eat until newline or EOF
        while (!isAtEnd() && peek() != '\n')
        {
            advance();
        }
        return makeToken(SKIP, startLine, startColumn, std::monostate{});
    }

    while (true)
    {
        if (isAtEnd())
        {
            // Unterminated multi-line comment
            return makeErrorToken("Unterminated multi-line comment", line, column);
        }

        if (peek() == '/' && peekNext() == '>')
        {
            break;
        }

        advance();
    }
    // Consume clusing '/' and '>'
    advance();
    advance();

    return makeToken(SKIP, startLine, startColumn, std::monostate{});
}

Token Lexer::scanOperator(char c, int startLine, int startColumn)
{

    switch (c)
    {
    case '+':
        return makeToken(PLUS, startLine, startColumn, std::monostate{});
    case '-':
        return makeToken(MINUS, startLine, startColumn, std::monostate{});
    case '*':
        return makeToken(STAR, startLine, startColumn, std::monostate{});
    case '/':
        return makeToken(SLASH, startLine, startColumn, std::monostate{});
    case '=':
        if (!isAtEnd() && peek() == '=')
        {
            advance();
            return makeToken(EQUAL_EQUAL, startLine, startColumn, std::monostate{});
        }
        return makeToken(EQUAL, startLine, startColumn, std::monostate{});

    case '!':
        if (!isAtEnd() && peek() == '=')
        {
            advance();
            return makeToken(BANG_EQUAL, startLine, startColumn, std::monostate{});
        }
        return makeErrorToken("We lack support for unary bang", line, column);

    case '<':
        if (!isAtEnd() && peek() == '=')
        {
            advance();
            return makeToken(LESS_EQUAL, startLine, startColumn, std::monostate{});
        }
        return makeToken(LESS, startLine, startColumn, std::monostate{});
    case '>':
        if (!isAtEnd() && peek() == '=')
        {
            advance();
            return makeToken(GREATER_EQUAL, startLine, startColumn, std::monostate{});
        }
        return makeToken(GREATER, startLine, startColumn, std::monostate{});
    default:
        return makeErrorToken("Unexpected character", line, column);
    }
}

Token Lexer::scanPunctuation(char c, int startLine, int startColumn)
{
    switch (c)
    {
    case ';':
        return makeToken(SEMI_COLON, startLine, startColumn);
    case '(':
    return makeToken(LEFT_PAREN, startLine, startColumn);
    case ')':
    return makeToken(RIGHT_PAREN, startLine, startColumn);
    case '{':
    return makeToken(LEFT_BRACE, startLine, startColumn);
    case '}':
    return makeToken(RIGHT_BRACE, startLine, startColumn);
    case ',':
    return makeToken(COMMA, startLine, startColumn);
    default:
    return makeErrorToken("Unexpected character", line, column);
    }
}

Token Lexer::makeToken(TokenType type, int startLine, int startColumn,
                       std::variant<std::monostate, int, bool, std::string> literalValue)
{
    std::string lexeme = source.substr(start, current - start);
    return Token(lexeme, type, literalValue, startLine, startColumn);
}

Token Lexer::makeErrorToken(std::string message, int startLine, int startColumn)
{
    std::string lexeme = source.substr(start, current - start);
    return Token(lexeme, ERROR, message, startLine, startColumn);
}

bool Lexer::isMultilineString()
{
    return (peek() == '"') && (peekNext() == '"') && (peekAhead(2) == '"');
}

Token Lexer::scanToken()
{
    int startLine = line;
    int startColumn = column;
    start = current;
    if (isAtEnd())
    {
        return makeToken(EOF_TOKEN, startLine, startColumn, std::monostate{});
    }

    char c = advance();

    switch (c)
    {

    case ' ':
    case '\r':
    case '\t':
    case '\n':
        return makeToken(SKIP, startLine, startColumn, std::monostate{});
    case '"':
        if (isMultilineString())
        {
            return scanMultiLineString(startLine, startColumn);
        }
        else
        {
            return scanString(startLine, startColumn);
        }
    case '<':
        if (peek() == '/')
        {
            return scanSlashOrComment(startLine, startColumn);
        }
        return scanOperator(c, startLine, startColumn);
    case '=':
    case '>':
    case '+':
    case '*':
    case '-':
    case '/':
    case '!':
        return scanOperator(c, startLine, startColumn);
    case ';':
    case '(':
    case ')':
    case '{':
    case '}':
    case ',':
        return scanPunctuation(c, startLine, startColumn);
    default:
        if (std::isdigit(static_cast<unsigned char>(c)))
        {
            return scanNumber(startLine, startColumn);
        }
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
        {
            return scanIdentifierOrKeyword(startLine, startColumn);
        }

        return makeErrorToken("Unexpected character", line, column);
    }
}