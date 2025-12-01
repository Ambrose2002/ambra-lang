#include "lexer/lexer.h"

#include <gtest/gtest.h>
#include <iostream>
#include <variant>

bool equalTokens(Token a, Token b)
{
    return a.getType() == b.getType() && a.getLexeme() == b.getLexeme() &&
           a.getLocation().line == b.getLocation().line && a.getValue() == b.getValue() &&
           a.getLocation().column == b.getLocation().column;
}

bool equalTokenVectors(std::vector<Token> a, std::vector<Token> b)
{
    if (a.size() != b.size())
    {
        return false;
    }

    for (int i = 0; i < a.size(); i++)
    {
        if (!equalTokens(a[i], b[i]))
        {
            std::cout << "failed with lexeme " << a[i].getLexeme();
            return false;
        }
    }
    return true;
}

TEST(SingleToken, Plus)
{
    Token              token("+", PLUS, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "+";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}


TEST(SingleToken, Minus)
{
    Token              token("-", PLUS, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}


TEST(SingleToken, LeftParen)
{
    Token              token("(", PLUS, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "(";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

TEST(SingleToken, RightParen)
{
    Token              token(")", PLUS, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> actual = {token, eof_token};

    std::string source = ")";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}


TEST(SingleToken, LeftBrace)
{
    Token              token("{", PLUS, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "{";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

TEST(SingleToken, RightBrace)
{
    Token              token("}", PLUS, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "}";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

TEST(SingleToken, SemiColon)
{
    Token              token(";", PLUS, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> actual = {token, eof_token};

    std::string source = ";";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}