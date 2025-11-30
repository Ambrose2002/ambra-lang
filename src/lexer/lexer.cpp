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
            return makeToken(ERROR, startLine, startColumn, std::monostate{});
        }

        char c = peek();

        if (c == '"')
        {
            // Found the closing character — stop the loop
            break;
        }

        if (c == '\n')
        {
            return makeToken(ERROR, startLine, startColumn, std::monostate{});
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
            return makeToken(ERROR, startLine, startColumn, std::monostate{});
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
            return makeToken(ERROR, startLine, startColumn, std::monostate{});
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

Token Lexer::scanOperator(int startLine, int startColumn) {}

Token Lexer::scanPunctuation(int startLine, int startColumn) {}

Token Lexer::makeToken(TokenType type, int startLine, int startColumn,
                       std::variant<std::monostate, int, bool, std::string> literalValue)
{
    std::string lexeme = source.substr(start, current - start);
    return Token(lexeme, type, literalValue, startLine, startColumn);
}

Token Lexer::errorToken(std::string message) {}

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
        return scanOperator(startLine, startColumn);
    case '=':
    case '>':
    case '+':
    case '*':
    case '-':
    case '/':
    case '!':
        return scanOperator(startLine, startColumn);
    case ';':
    case '(':
    case ')':
    case '{':
    case '}':
    case ',':
        return scanPunctuation(startLine, startColumn);
    default:
        if (std::isdigit(static_cast<unsigned char>(c)))
        {
            return scanNumber(startLine, startColumn);
        }
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
        {
            return scanIdentifierOrKeyword(startLine, startColumn);
        }

        return errorToken("Unexpected character");
    }
}