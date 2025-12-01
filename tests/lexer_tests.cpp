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
    Token              token("-", MINUS, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "-";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}


TEST(SingleToken, LeftParen)
{
    Token              token("(", LEFT_PAREN, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "(";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

TEST(SingleToken, RightParen)
{
    Token              token(")", RIGHT_PAREN, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> actual = {token, eof_token};

    std::string source = ")";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}


TEST(SingleToken, LeftBrace)
{
    Token              token("{", LEFT_BRACE, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "{";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

TEST(SingleToken, RightBrace)
{
    Token              token("}", RIGHT_BRACE, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "}";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

TEST(SingleToken, SemiColon)
{
    Token              token(";", SEMI_COLON, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> actual = {token, eof_token};

    std::string source = ";";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

TEST(SingleToken, Star)
{
    Token              token("*", STAR, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "*";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

TEST(SingleToken, Slash)
{
    Token              token("/", SLASH, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "/";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

TEST(SingleToken, Equal)
{
    Token              token("=", EQUAL, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "=";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

TEST(SingleToken, EqualEqual)
{
    Token              token("==", EQUAL_EQUAL, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 3);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "==";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

TEST(SingleToken, BangEqual)
{
    Token              token("!=", BANG_EQUAL, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 3);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "!=";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

TEST(SingleToken, Less)
{
    Token              token("<", LESS, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "<";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

TEST(SingleToken, LessEqual)
{
    Token              token("<=", LESS_EQUAL, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 3);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "<=";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

TEST(SingleToken, Greater)
{
    Token              token(">", GREATER, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> actual = {token, eof_token};

    std::string source = ">";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

TEST(SingleToken, GreaterEqual)
{
    Token              token(">=", GREATER_EQUAL, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 3);
    std::vector<Token> actual = {token, eof_token};

    std::string source = ">=";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

TEST(SingleToken, Comma)
{
    Token              token(",", COMMA, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> actual = {token, eof_token};

    std::string source = ",";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

// Keyword single-token tests
TEST(SingleToken, Summon)
{
    Token              token("summon", SUMMON, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 7);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "summon";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

TEST(SingleToken, Should)
{
    Token              token("should", SHOULD, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 7);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "should";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

TEST(SingleToken, Otherwise)
{
    Token              token("otherwise", OTHERWISE, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 10);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "otherwise";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

TEST(SingleToken, AsLongAs)
{
    Token              token("aslongas", ASLONGAS, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 9);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "aslongas";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

TEST(SingleToken, Say)
{
    Token              token("say", SAY, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 4);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "say";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

TEST(SingleToken, Not)
{
    Token              token("not", NOT, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 4);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "not";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

TEST(SingleToken, Identifier)
{
    Token              token("foo", IDENTIFIER, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 4);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "foo";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

TEST(SingleToken, BoolAffirmative)
{
    Token              token("affirmative", BOOL, true, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 12);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "affirmative";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}

TEST(SingleToken, BoolNegative)
{
    Token              token("negative", BOOL, false, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 9);
    std::vector<Token> actual = {token, eof_token};

    std::string source = "negative";
    Lexer       lexer(source);

    std::vector<Token> expected = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(actual, expected));
}