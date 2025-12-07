#include "lexer.h"

#include <cctype>
#include <variant>

Lexer::Lexer(std::string source) : source(source)
{
    start = 0;
    current = 0;
    mode = NORMAL_MODE;
    insideMultiline = false;
};

std::unordered_map<std::string, TokenType> Lexer::keywordMap = {
    {"summon", SUMMON}, {"should", SHOULD}, {"otherwise", OTHERWISE}, {"aslongas", ASLONGAS},
    {"say", SAY},       {"not", NOT},       {"affirmative", BOOL},    {"negative", BOOL}};

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
    // Consume leading digits
    while (!isAtEnd() && std::isdigit(peek()))
    {
        advance();
    }

    if (!isAtEnd() && (std::isalpha(static_cast<unsigned char>(peek())) || peek() == '_'))
    {
        // The entire lexeme from start → current+rest is invalid.
        // Consume the rest of the alphanumeric run so the lexeme is correct.
        while (!isAtEnd() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_'))
        {
            advance();
        }
        return makeErrorToken("Invalid number", startLine, startColumn);
    }

    // Valid integer
    std::string lexeme = source.substr(start, current - start);
    int number = std::stoi(lexeme);
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
    // Check if we're starting fresh (mode is not STRING_MODE yet) or resuming (mode is already
    // STRING_MODE)
    bool isInitialEntry = (mode != STRING_MODE);

    insideMultiline = false;

    // Now enter STRING_MODE for scanning
    mode = STRING_MODE;

    // Scan until closing quote OR interpolation
    while (true)
    {
        if (isAtEnd())
        {
            return makeErrorToken("Unterminated string", startLine, startColumn);
        }

        char c = peek();

        // 1. End of string
        if (c == '"')
        {
            advance(); // consume closing quote
            mode = NORMAL_MODE;
            // If initial entry, skip the opening quote; if resuming, don't skip
            // Compute literal content correctly (exclude the closing quote)
            int offset = isInitialEntry ? 1 : 0;
            int length = (current - start - offset - 1);

            if (length < 0)
                length = 0;
            std::string literal = source.substr(start + offset, length);

            return makeToken(STRING, startLine, startColumn, literal);
        }

        // 2. Start of interpolation
        if (c == '{')
        {
            // Interpolation tracking
            interpStartLine = line;
            interpStartColumn = column;
            interpStart = current;
            // If we're resuming (not initial entry) and immediately hit '{',
            // don't emit an empty STRING token - just switch modes
            if (!isInitialEntry && current == start)
            {
                mode = INTERP_EXPR_MODE;
                // Return a SKIP token to indicate no string content to emit
                return makeToken(SKIP, startLine, startColumn, std::monostate{});
            }

            // Do NOT consume '{'
            // If initial entry, skip the opening quote; if resuming, don't skip
            int offset = isInitialEntry ? 1 : 0;
            int length = (current - start - offset);

            if (length < 0)
                length = 0;
            std::string literal = source.substr(start + offset, length);

            mode = INTERP_EXPR_MODE;
            return makeToken(STRING, startLine, startColumn, literal);
        }
        if (c == '\n')
        {
            return makeErrorToken("Unterminated string", startLine, startColumn);
        }
        advance();
    }
}

Token Lexer::scanMultiLineString(int startLine, int startColumn)
{
    // Enter multiline string mode
    mode = MULTILINE_STRING_MODE;
    insideMultiline = true;

    // If this is the first time we enter after seeing the opening """:
    //   - scanToken set start = index of first '"'
    //   - scanToken consumed that first '"', so current == start + 1
    //   - and source[start..start+2] == "\"\"\""
    if (current == start + 1 && start + 2 < static_cast<int>(source.length()) &&
        source[start] == '"' && source[start + 1] == '"' && source[start + 2] == '"')
    {
        // Consume the remaining two quotes of the opening """.
        advance();       // second quote
        advance();       // third quote
        start = current; // content starts after """
    }
    else
    {
        // Resuming after an interpolation: content starts at current.
        start = current;
    }

    while (true)
    {
        if (isAtEnd())
        {
            insideMultiline = false;
            return makeErrorToken("Unterminated multiline string", startLine, startColumn);
        }

        // Check for interpolation start
        if (peek() == '{')
        {
            // Interpolation tracking
            interpStartLine = line;
            interpStartColumn = column;
            interpStart = current;
            // Do NOT consume '{'
            std::string lexeme = source.substr(start, current - start);
            mode = INTERP_EXPR_MODE;
            return makeToken(MULTILINE_STRING, startLine, startColumn, lexeme);
        }

        // Check for closing triple quotes
        if (peek() == '"' && peekNext() == '"' && peekAhead(2) == '"')
        {
            // Produce the final multiline string chunk *before* consuming """.
            std::string lexeme = source.substr(start, current - start);

            // Create the token now (uses current/start as the bounds),
            // then consume the closing quotes so scanning continues after them.
            Token t = makeToken(MULTILINE_STRING, startLine, startColumn, lexeme);

            // Consume the closing triple quotes
            advance(); // "
            advance(); // "
            advance(); // "

            mode = NORMAL_MODE;
            return t;
        }

        // Otherwise: consume a character (newline allowed)
        advance();
    }
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
        if (mode == STRING_MODE)
        {
            // Begin interpolation
            mode = INTERP_EXPR_MODE;
            return makeToken(INTERP_START, startLine, startColumn);
        }
        else
        {
            // Normal brace (in NORMAL_MODE or INTERP_EXPR_MODE)
            return makeToken(LEFT_BRACE, startLine, startColumn);
        }

    case '}':
        if (mode == STRING_MODE)
        {
            // Cannot close a block inside a raw string section
            return makeErrorToken("Cannot close block inside string", startLine, startColumn);
        }
        else if (mode == INTERP_EXPR_MODE)
        {
            // Fallback: end interpolation here too
            if (insideMultiline)
                mode = MULTILINE_STRING_MODE;
            else
                mode = STRING_MODE;

            return makeToken(INTERP_END, startLine, startColumn);
        }
        else
        {
            // Normal RIGHT_BRACE in NORMAL_MODE
            return makeToken(RIGHT_BRACE, startLine, startColumn);
        }
    case ',':
        return makeToken(COMMA, startLine, startColumn);

    default:
        return makeErrorToken("Unexpected punctuation character", startLine, startColumn);
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
    // After consuming the first '"' in scanToken(), check whether the
    // next two characters are '"' to detect a opening triple-quote.
    return (peek() == '"') && (peekNext() == '"');
}

Token Lexer::scanToken()
{
    int startLine = line;
    int startColumn = column;
    start = current;
    // Interpolation unterminated error check
    if (mode == INTERP_EXPR_MODE && isAtEnd()) {
        std::string lex = source.substr(interpStart, 1);
        return Token(lex, ERROR, std::string("Unterminated interpolation"), interpStartLine, interpStartColumn);
    }
    if (mode == INTERP_EXPR_MODE && peek() == '"') {
        std::string lex = source.substr(interpStart, 1);
        return Token(lex, ERROR, std::string("Unterminated interpolation"), interpStartLine, interpStartColumn);
    }
    if (isAtEnd())
    {
        return makeToken(EOF_TOKEN, startLine, startColumn, std::monostate{});
    }

    // If we are in the middle of a multiline string, always delegate to scanMultiLineString
    if (mode == MULTILINE_STRING_MODE)
        return scanMultiLineString(startLine, startColumn);

    if (mode == STRING_MODE)
        return scanString(startLine, startColumn);

    // Handle interpolation delimiters BEFORE general tokenization
    if (mode == INTERP_EXPR_MODE && !isAtEnd() && peek() == '{')
    {
        advance(); // consume '{'
        return makeToken(INTERP_START, startLine, startColumn);
    }

    if (mode == INTERP_EXPR_MODE && !isAtEnd() && peek() == '}')
    {
        advance(); // consume '}'

        // After an interpolation ends, go back to the right string mode
        if (insideMultiline)
        {
            mode = MULTILINE_STRING_MODE;
        }
        else
        {
            mode = STRING_MODE;
        }

        return makeToken(INTERP_END, startLine, startColumn);
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

std::vector<Token> Lexer::scanTokens()
{

    std::vector<Token> tokens;

    while (true)
    {
        Token token = scanToken();

        if (token.getType() == SKIP)
        {
            continue;
        }

        tokens.push_back(token);

        if (token.getType() == ERROR || token.getType() == EOF_TOKEN)
        {
            return tokens;
        }
    }
}