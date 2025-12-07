#include "lexer/lexer.h"

#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

bool equalTokens(Token a, Token b)
{
    return a.getType() == b.getType() && a.getLexeme() == b.getLexeme() &&
           a.getLocation().line == b.getLocation().line && a.getValue() == b.getValue() &&
           a.getLocation().column == b.getLocation().column;
}

void printExpectedVsActual(std::vector<Token> expected, std::vector<Token> actual)
{

    auto valToStr = [](const std::variant<std::monostate, int, bool, std::string>& v)
    {
        if (std::holds_alternative<int>(v))
            return std::to_string(std::get<int>(v));
        if (std::holds_alternative<bool>(v))
            return std::string(std::get<bool>(v) ? "true" : "false");
        if (std::holds_alternative<std::string>(v))
            return std::get<std::string>(v);
        return std::string("<none>");
    };

    std::cout << "EXPECTED TOKENS:\n";
    for (auto& t : expected)
    {
        std::cout << "  lexeme='" << t.getLexeme() << "' type=" << t.getType() << " value='"
                  << valToStr(t.getValue()) << "' loc=" << t.getLocation().line << ","
                  << t.getLocation().column << "\n";
    }
    std::cout << "ACTUAL TOKENS:\n";
    for (auto& t : actual)
    {
        std::cout << "  lexeme='" << t.getLexeme() << "' type=" << t.getType() << " value='"
                  << valToStr(t.getValue()) << "' loc=" << t.getLocation().line << ","
                  << t.getLocation().column << "\n";
    }
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
            std::cout << "failed with lexeme " << a[i].getLexeme() << std::endl;
            return false;
        }
    }
    return true;
}

TEST(SingleToken, Plus)
{
    Token              token("+", PLUS, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "+";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, Minus)
{
    Token              token("-", MINUS, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "-";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, LeftParen)
{
    Token              token("(", LEFT_PAREN, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "(";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, RightParen)
{
    Token              token(")", RIGHT_PAREN, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> expected = {token, eof_token};

    std::string source = ")";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, LeftBrace)
{
    Token              token("{", LEFT_BRACE, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "{";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, RightBrace)
{
    Token              token("}", RIGHT_BRACE, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "}";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, SemiColon)
{
    Token              token(";", SEMI_COLON, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> expected = {token, eof_token};

    std::string source = ";";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, Star)
{
    Token              token("*", STAR, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "*";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, Slash)
{
    Token              token("/", SLASH, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "/";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, Equal)
{
    Token              token("=", EQUAL, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "=";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, EqualEqual)
{
    Token              token("==", EQUAL_EQUAL, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 3);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "==";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, BangEqual)
{
    Token              token("!=", BANG_EQUAL, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 3);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "!=";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, Less)
{
    Token              token("<", LESS, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "<";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, LessEqual)
{
    Token              token("<=", LESS_EQUAL, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 3);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "<=";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, Greater)
{
    Token              token(">", GREATER, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> expected = {token, eof_token};

    std::string source = ">";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, GreaterEqual)
{
    Token              token(">=", GREATER_EQUAL, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 3);
    std::vector<Token> expected = {token, eof_token};

    std::string source = ">=";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, Comma)
{
    Token              token(",", COMMA, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> expected = {token, eof_token};

    std::string source = ",";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

// Keyword single-token tests
TEST(SingleToken, Summon)
{
    Token              token("summon", SUMMON, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 7);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "summon";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, Should)
{
    Token              token("should", SHOULD, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 7);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "should";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, Otherwise)
{
    Token              token("otherwise", OTHERWISE, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 10);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "otherwise";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, AsLongAs)
{
    Token              token("aslongas", ASLONGAS, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 9);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "aslongas";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, Say)
{
    Token              token("say", SAY, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 4);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "say";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, Not)
{
    Token              token("not", NOT, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 4);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "not";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, Identifier)
{
    Token              token("foo", IDENTIFIER, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 4);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "foo";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, IdentifierWithUnderscoreAtStart)
{
    Token              token("_foo", IDENTIFIER, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 5);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "_foo";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, IdentifierWithUnderscoreInMiddle)
{
    Token              token("fo_o", IDENTIFIER, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 5);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "fo_o";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, IdentifierWithUnderscoreAtEnd)
{
    Token              token("foo_", IDENTIFIER, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 5);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "foo_";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, IdentiferWithNumbers)
{
    Token              token("foo123", IDENTIFIER, std::monostate{}, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 7);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "foo123";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, BoolAffirmative)
{
    Token              token("affirmative", BOOL, true, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 12);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "affirmative";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, BoolNegative)
{
    Token              token("negative", BOOL, false, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 9);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "negative";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, IntegerSingleDigit)
{
    Token              token("7", INTEGER, 7, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "7";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, IntegerMultiDigit)
{
    Token              token("123", INTEGER, 123, 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 4);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "123";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, StringSimple)
{
    Token              token("\"hello\"", STRING, std::string("hello"), 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 8);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "\"hello\"";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    bool res = equalTokenVectors(expected, actual);

    if (!res)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(res);
}

TEST(SingleToken, UnterminatedString)
{
    Token              token("\"hello", ERROR, std::string("Unterminated string"), 1, 1);
    std::vector<Token> expected = {token};

    std::string source = "\"hello";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    ASSERT_TRUE(equalTokenVectors(expected, actual));
}

TEST(SingleToken, MultiLineStringSimple)
{
    // source contains opening and closing triple quotes with content "hello"
    Token              token("\nhello\n", MULTILINE_STRING, std::string("\nhello\n"), 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 3, 4);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "\"\"\"\nhello\n\"\"\""; // """hello"""
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    bool res = equalTokenVectors(expected, actual);

    if (!res)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(res);
}

TEST(SingleToken, UnterminatedMultiLineString)
{
    // opening triple quotes but no closing sequence
    Token              token("hello", ERROR, std::string("Unterminated multiline string"), 1, 1);
    std::vector<Token> expected = {token};

    std::string source = "\"\"\"hello"; // """hello
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    bool res = equalTokenVectors(expected, actual);

    if (!res)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(res);
}

TEST(SingleToken, MultiLineStringEmpty)
{
    Token              token("\n", MULTILINE_STRING, std::string("\n"), 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 2, 4);
    std::vector<Token> expected = {token, eof_token};

    std::string source =
        "\"\"\"\n\"\"\""; // 6 quotes: """ (opening) + """ (closing) with empty content
    Lexer lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    bool res = equalTokenVectors(expected, actual);

    if (!res)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(res);
}

TEST(SingleToken, MultiLineStringWithQuotes)
{
    Token              token("he\"llo", MULTILINE_STRING, std::string("he\"llo"), 1, 1);
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 13);
    std::vector<Token> expected = {token, eof_token};

    std::string source = "\"\"\"he\"llo\"\"\""; // """he"llo"""
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    bool res = equalTokenVectors(expected, actual);

    if (!res)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(res);
}

TEST(SingleToken, MultiLineStringInterpolationStart)
{
    // Expect full token stream: MULTILINE_STRING("hello"), INTERP_START("{"), EOF
    Token              token("hello", MULTILINE_STRING, std::string("hello"), 1, 1);
    Token              interp("{", INTERP_START, std::monostate{}, 1, 9);
    Token              error_token("{", ERROR, std::string("Unterminated interpolation"), 1, 9);
    std::vector<Token> expected = {token, interp, error_token};

    std::string source = "\"\"\"hello{"; // """hello{
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    bool res = equalTokenVectors(expected, actual);

    if (!res)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(res);
}

TEST(IgnoredToken, Whitespace)
{

    Token              token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> expected = {token};

    std::string source = " ";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    bool testResults = equalTokenVectors(expected, actual);

    if (!testResults)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(testResults);
}

TEST(IgnoredToken, EndOfLine)
{

    Token              token("", EOF_TOKEN, std::monostate{}, 2, 1);
    std::vector<Token> expected = {token};

    std::string source = "\n";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    bool testResults = equalTokenVectors(expected, actual);

    if (!testResults)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(testResults);
}

TEST(IgnoredToken, HorizontalTab)
{

    Token              token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> expected = {token};

    std::string source = "\t";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    bool testResults = equalTokenVectors(expected, actual);

    if (!testResults)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(testResults);
}

TEST(IgnoredToken, CarriageReturnEscape)
{

    Token              token("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> expected = {token};

    std::string source = "\r";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    bool testResults = equalTokenVectors(expected, actual);

    if (!testResults)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(testResults);
}

TEST(IgnoredToken, SingleLineComment)
{

    Token              token("", EOF_TOKEN, std::monostate{}, 1, 14);
    std::vector<Token> expected = {token};

    std::string source = "</Hello World";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    bool testResults = equalTokenVectors(expected, actual);

    if (!testResults)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(testResults);
}

TEST(IgnoredToken, MultiLineComment)
{

    Token              token("", EOF_TOKEN, std::monostate{}, 4, 3);
    std::vector<Token> expected = {token};

    std::string source = "</\nhello how are you \n I hope you're fine\n/>";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    bool testResults = equalTokenVectors(expected, actual);

    if (!testResults)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(testResults);
}

TEST(MultiTokens, ShouldIgnoreWhiteSpaceBetweenTokens)
{
    Token summonToken("summon", SUMMON, std::monostate{}, 1, 1);
    Token xToken("x", IDENTIFIER, std::monostate{}, 1, 14);
    Token eofToken("", EOF_TOKEN, std::monostate{}, 1, 15);

    std::vector<Token> expected = {summonToken, xToken, eofToken};

    std::string source = "summon       x";
    Lexer       lexer(source);

    std::vector<Token> actual = lexer.scanTokens();

    bool testResults = equalTokenVectors(expected, actual);

    if (!testResults)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(testResults);
}

// Simple cases
TEST(Interpolation_Simple, ExprOnly)
{
    // source: "\"{x}\""
    Token              t1("\"", STRING, std::string(""), 1, 1);
    Token              t2("{", INTERP_START, std::monostate{}, 1, 2);
    Token              t3("x", IDENTIFIER, std::monostate{}, 1, 3);
    Token              t4("}", INTERP_END, std::monostate{}, 1, 4);
    Token              t5("\"", STRING, std::string(""), 1, 5);
    Token              te("", EOF_TOKEN, std::monostate{}, 1, 6);
    std::vector<Token> expected = {t1, t2, t3, t4, t5, te};

    std::string source = "\"{x}\"";
    Lexer       lexer(source);
    auto        actual = lexer.scanTokens();

    bool testResults = equalTokenVectors(expected, actual);

    if (!testResults)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(testResults);
}

TEST(Interpolation_Simple, HelloName)
{
    // source: "\"hello {name}\""
    Token              t1("\"hello ", STRING, std::string("hello "), 1, 1);
    Token              t2("{", INTERP_START, std::monostate{}, 1, 8);
    Token              t3("name", IDENTIFIER, std::monostate{}, 1, 9);
    Token              t4("}", INTERP_END, std::monostate{}, 1, 13);
    Token              t5("\"", STRING, std::string(""), 1, 14);
    Token              te("", EOF_TOKEN, std::monostate{}, 1, 15);
    std::vector<Token> expected = {t1, t2, t3, t4, t5, te};

    std::string source = "\"hello {name}\"";
    Lexer       lexer(source);
    auto        actual = lexer.scanTokens();

    bool testResults = equalTokenVectors(expected, actual);

    if (!testResults)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(testResults);
}

TEST(Interpolation_Simple, ExprWithOps)
{
    // source: "\"{x + 1}\""
    Token              t1("\"", STRING, std::string(""), 1, 1);
    Token              t2("{", INTERP_START, std::monostate{}, 1, 2);
    Token              t3("x", IDENTIFIER, std::monostate{}, 1, 3);
    Token              t4("+", PLUS, std::monostate{}, 1, 5);
    Token              t5("1", INTEGER, 1, 1, 7);
    Token              t6("}", INTERP_END, std::monostate{}, 1, 8);
    Token              t7("\"", STRING, std::string(""), 1, 9);
    Token              te("", EOF_TOKEN, std::monostate{}, 1, 10);
    std::vector<Token> expected = {t1, t2, t3, t4, t5, t6, t7, te};

    std::string source = "\"{x + 1}\"";
    Lexer       lexer(source);
    auto        actual = lexer.scanTokens();

    bool testResults = equalTokenVectors(expected, actual);

    if (!testResults)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(testResults);
}

// Edge cases
TEST(Interpolation_Edge, BareInterpInIsolation)
{
    // source: "{x}" -> should be treated like empty-string + brace seq + empty-string
    Token              t1("{", LEFT_BRACE, std::monostate{}, 1, 1);
    Token              t2("x", IDENTIFIER, std::monostate{}, 1, 2);
    Token              t3("}", RIGHT_BRACE, std::monostate{}, 1, 3);
    Token              te("", EOF_TOKEN, std::monostate{}, 1, 4);
    std::vector<Token> expected = {t1, t2, t3, te};

    std::string source = "{x}";
    Lexer       lexer(source);
    auto        actual = lexer.scanTokens();

    bool testResults = equalTokenVectors(expected, actual);

    if (!testResults)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(testResults);
}

TEST(Interpolation_Edge, BackToBackInterpolations)
{
    // source: "\"{x}{y}\""
    Token              t1("\"", STRING, std::string(""), 1, 1);
    Token              t2("{", INTERP_START, std::monostate{}, 1, 2);
    Token              t3("x", IDENTIFIER, std::monostate{}, 1, 3);
    Token              t4("}", INTERP_END, std::monostate{}, 1, 4);
    Token              t5("{", INTERP_START, std::monostate{}, 1, 5);
    Token              t6("y", IDENTIFIER, std::monostate{}, 1, 6);
    Token              t7("}", INTERP_END, std::monostate{}, 1, 7);
    Token              t8("\"", STRING, std::string(""), 1, 8);
    Token              te("", EOF_TOKEN, std::monostate{}, 1, 9);
    std::vector<Token> expected = {t1, t2, t3, t4, t5, t6, t7, t8, te};

    std::string source = "\"{x}{y}\"";
    Lexer       lexer(source);
    auto        actual = lexer.scanTokens();

    bool testResults = equalTokenVectors(expected, actual);

    if (!testResults)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(testResults);
}

TEST(Interpolation_Edge, SpacedName)
{
    // source: "\"hello { name } world\""
    Token              t1("\"hello ", STRING, std::string("hello "), 1, 1);
    Token              t2("{", INTERP_START, std::monostate{}, 1, 8);
    Token              t3("name", IDENTIFIER, std::monostate{}, 1, 10);
    Token              t4("}", INTERP_END, std::monostate{}, 1, 15);
    Token              t5(" world\"", STRING, std::string(" world"), 1, 16);
    Token              te("", EOF_TOKEN, std::monostate{}, 1, 23);
    std::vector<Token> expected = {t1, t2, t3, t4, t5, te};

    std::string source = "\"hello { name } world\"";
    Lexer       lexer(source);
    auto        actual = lexer.scanTokens();

    bool testResults = equalTokenVectors(expected, actual);

    if (!testResults)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(testResults);
}

TEST(Interpolation_Edge, UnterminatedStringBeforeInterp)
{
    // source: "\"hello {" -> should produce ERROR for unterminated string
    Token              t1("\"hello ", STRING, "hello ", 1, 1);
    Token              t2("{", INTERP_START, std::monostate{}, 1, 8);
    Token              t3("{", ERROR, std::string("Unterminated interpolation"), 1, 8);
    std::vector<Token> expected = {t1, t2, t3};

    std::string source = "\"hello {";
    Lexer       lexer(source);
    auto        actual = lexer.scanTokens();

    bool testResults = equalTokenVectors(expected, actual);

    if (!testResults)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(testResults);
}

// Interpolation inside multiline
TEST(Interpolation_Multiline, SimpleInside)
{
    // source: "\"\"\"Hello {name}\"\"\""
    Token              t1("\nHello ", MULTILINE_STRING, std::string("\nHello "), 1, 1);
    Token              t2("{", INTERP_START, std::monostate{}, 2, 7);
    Token              t3("name", IDENTIFIER, std::monostate{}, 2, 8);
    Token              t4("}", INTERP_END, std::monostate{}, 2, 12);
    Token              t5("\n", MULTILINE_STRING, std::string("\n"), 2, 13);
    Token              te("", EOF_TOKEN, std::monostate{}, 3, 4);
    std::vector<Token> expected = {t1, t2, t3, t4, t5, te};

    std::string source = "\"\"\"\nHello {name}\n\"\"\"";
    Lexer       lexer(source);
    auto        actual = lexer.scanTokens();

    bool testResults = equalTokenVectors(expected, actual);

    if (!testResults)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(testResults);
}

TEST(Interpolation_Multiline, MultipleAdjacent)
{
    // source: "\"\"\"{a}{b}\"\"\""
    Token              t1("\n", MULTILINE_STRING, std::string("\n"), 1, 1);
    Token              t2("{", INTERP_START, std::monostate{}, 2, 1);
    Token              t3("a", IDENTIFIER, std::monostate{}, 2, 2);
    Token              t4("}", INTERP_END, std::monostate{}, 2, 3);
    Token              t5("", MULTILINE_STRING, "", 2, 4);
    Token              t6("{", INTERP_START, std::monostate{}, 2, 4);
    Token              t7("b", IDENTIFIER, std::monostate{}, 2, 5);
    Token              t8("}", INTERP_END, std::monostate{}, 2, 6);
    Token              t9("\n", MULTILINE_STRING, std::string("\n"), 2, 7);
    Token              t10("", EOF_TOKEN, std::monostate{}, 3, 4);
    std::vector<Token> expected = {t1, t2, t3, t4, t5, t6, t7, t8, t9, t10};

    std::string source = "\"\"\"\n{a}{b}\n\"\"\"";
    Lexer       lexer(source);
    auto        actual = lexer.scanTokens();

    bool testResults = equalTokenVectors(expected, actual);

    if (!testResults)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(testResults);
}

TEST(MultiTokens, SimpleBoolAssignment)
{
    // source: "summon flag = affirmative;"
    Token              t1("summon", SUMMON, std::monostate{}, 1, 1);
    Token              t2("flag", IDENTIFIER, std::monostate{}, 1, 8);
    Token              t3("=", EQUAL, std::monostate{}, 1, 13);
    Token              t4("affirmative", BOOL, true, 1, 15);
    Token              t5(";", SEMI_COLON, std::monostate{}, 1, 26);
    Token              te("", EOF_TOKEN, std::monostate{}, 1, 27);
    std::vector<Token> expected = {t1, t2, t3, t4, t5, te};

    std::string source = "summon flag = affirmative;";
    Lexer       lexer(source);

    auto actual = lexer.scanTokens();
    bool testResults = equalTokenVectors(expected, actual);

    if (!testResults)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(testResults);
}

TEST(Interpolation_Simple, BoolComparisonInExpr)
{
    // source: "\"{affirmative == negative}\""
    Token              t1("\"", STRING, std::string(""), 1, 1);
    Token              t2("{", INTERP_START, std::monostate{}, 1, 2);
    Token              t3("affirmative", BOOL, true, 1, 3);
    Token              t4("==", EQUAL_EQUAL, std::monostate{}, 1, 15);
    Token              t5("negative", BOOL, false, 1, 18);
    Token              t6("}", INTERP_END, std::monostate{}, 1, 26);
    Token              t7("\"", STRING, std::string(""), 1, 27);
    Token              te("", EOF_TOKEN, std::monostate{}, 1, 28);
    std::vector<Token> expected = {t1, t2, t3, t4, t5, t6, t7, te};

    std::string source = "\"{affirmative == negative}\"";
    Lexer       lexer(source);
    auto        actual = lexer.scanTokens();

    bool testResults = equalTokenVectors(expected, actual);
    if (!testResults)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(testResults);
}

TEST(Interpolation_Multiline, ComplexExprInside)
{
    // source: "\"\"\"\nValue: {x + 42}\n\"\"\""
    //
    // Line 1: """\n            (columns 1–3 are quotes, 4 is '\n')
    // Line 2: "Value: {x + 42}\n"
    //   "Value: " = cols 1–7
    //   '{'       = col 8
    //   'x'       = col 9
    //   '+'       = col 11
    //   "42"      = cols 13–14
    //   '}'       = col 15
    //   '\n'      = col 16
    // Line 3: """ (cols 1–3), EOF at column 4

    Token              t1("\nValue: ", MULTILINE_STRING, std::string("\nValue: "), 1, 1);
    Token              t2("{", INTERP_START, std::monostate{}, 2, 8);
    Token              t3("x", IDENTIFIER, std::monostate{}, 2, 9);
    Token              t4("+", PLUS, std::monostate{}, 2, 11);
    Token              t5("42", INTEGER, 42, 2, 13);
    Token              t6("}", INTERP_END, std::monostate{}, 2, 15);
    Token              t7("\n", MULTILINE_STRING, std::string("\n"), 2, 16);
    Token              te("", EOF_TOKEN, std::monostate{}, 3, 4);
    std::vector<Token> expected = {t1, t2, t3, t4, t5, t6, t7, te};

    std::string source = "\"\"\"\nValue: {x + 42}\n\"\"\"";
    Lexer       lexer(source);
    auto        actual = lexer.scanTokens();

    bool testResults = equalTokenVectors(expected, actual);
    if (!testResults)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(testResults);
}

TEST(MultiTokens, TokensAroundSingleLineComment)
{
    // source: "summon x</ this is a comment\nshould y"
    //
    // Line 1: "summon x</ this is a comment"
    //   "summon" = cols 1–6
    //   'x'      = col 8
    //   "</..."  = comment (ignored)
    // Line 2: "should y"
    //   "should" = cols 1–6
    //   'y'      = col 8

    Token              t1("summon", SUMMON, std::monostate{}, 1, 1);
    Token              t2("x", IDENTIFIER, std::monostate{}, 1, 8);
    Token              t3("should", SHOULD, std::monostate{}, 2, 1);
    Token              t4("y", IDENTIFIER, std::monostate{}, 2, 8);
    Token              te("", EOF_TOKEN, std::monostate{}, 2, 9);
    std::vector<Token> expected = {t1, t2, t3, t4, te};

    std::string source = "summon x</ this is a comment\nshould y";
    Lexer       lexer(source);
    auto        actual = lexer.scanTokens();

    bool testResults = equalTokenVectors(expected, actual);
    if (!testResults)
    {
        printExpectedVsActual(expected, actual);
    }
    ASSERT_TRUE(testResults);
}

TEST(Errors_Number, NumberFollowedByIdentifier)
{
    Token              t1("12foo", ERROR, std::string("Invalid number"), 1, 1);
    std::vector<Token> expected = {t1};

    std::string source = "12foo";
    Lexer       lexer(source);
    auto        actual = lexer.scanTokens();

    bool res = equalTokenVectors(expected, actual);
    if (!res)
        printExpectedVsActual(expected, actual);
    ASSERT_TRUE(res);
}

TEST(Errors_Number, UnsupportedHexLiteral)
{
    Token t1("0x123", ERROR, std::string("Invalid number"), 1, 1);
    std::vector<Token> expected = {t1};

    std::string source = "0x123";
    Lexer lexer(source);
    auto actual = lexer.scanTokens();

    bool res = equalTokenVectors(expected, actual);
    if (!res) printExpectedVsActual(expected, actual);
    ASSERT_TRUE(res);
}

TEST(Interpolation_Errors, UnterminatedInterpolationMissingBrace)
{
    Token t1("\"hello ", STRING, std::string("hello "), 1, 1);
    Token t2("{", INTERP_START, std::monostate{}, 1, 8);
    Token t3("name", IDENTIFIER, std::monostate{}, 1, 9);
    Token t4("{", ERROR, std::string("Unterminated interpolation"), 1, 8);

    std::vector<Token> expected = {t1, t2, t3, t4};

    std::string source = "\"hello {name\"";
    Lexer lexer(source);
    auto actual = lexer.scanTokens();

    bool res = equalTokenVectors(expected, actual);
    if (!res) printExpectedVsActual(expected, actual);
    ASSERT_TRUE(res);
}

TEST(EmptyInput, OnlyEOF)
{
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 1, 1);
    std::vector<Token> expected = {eof_token};

    std::string source = "";
    Lexer       lexer(source);
    auto        actual = lexer.scanTokens();

    bool res = equalTokenVectors(expected, actual);
    if (!res) printExpectedVsActual(expected, actual);
    ASSERT_TRUE(res);
}

TEST(Whitespace_Mixed, SpacesTabsNewlines)
{
    // " \t\n  "
    // Line/col evolution:
    //   ' '  -> line 1, col 2
    //   '\t' -> line 1, col 3
    //   '\n' -> line 2, col 1
    //   ' '  -> line 2, col 2
    //   ' '  -> line 2, col 3
    // EOF at line 2, col 3
    Token              eof_token("", EOF_TOKEN, std::monostate{}, 2, 3);
    std::vector<Token> expected = {eof_token};

    std::string source = " \t\n  ";
    Lexer       lexer(source);
    auto        actual = lexer.scanTokens();

    bool res = equalTokenVectors(expected, actual);
    if (!res) printExpectedVsActual(expected, actual);
    ASSERT_TRUE(res);
}

TEST(Identifiers_Edge, SingleUnderscore)
{
    Token              t1("_", IDENTIFIER, std::monostate{}, 1, 1);
    Token              te("", EOF_TOKEN, std::monostate{}, 1, 2);
    std::vector<Token> expected = {t1, te};

    std::string source = "_";
    Lexer       lexer(source);
    auto        actual = lexer.scanTokens();

    bool res = equalTokenVectors(expected, actual);
    if (!res) printExpectedVsActual(expected, actual);
    ASSERT_TRUE(res);
}

TEST(Identifiers_Edge, MultipleUnderscores)
{
    Token              t1("___", IDENTIFIER, std::monostate{}, 1, 1);
    Token              te("", EOF_TOKEN, std::monostate{}, 1, 4);
    std::vector<Token> expected = {t1, te};

    std::string source = "___";
    Lexer       lexer(source);
    auto        actual = lexer.scanTokens();

    bool res = equalTokenVectors(expected, actual);
    if (!res) printExpectedVsActual(expected, actual);
    ASSERT_TRUE(res);
}

TEST(Identifiers_Edge, UnderscoreThenDigits)
{
    Token              t1("_123", IDENTIFIER, std::monostate{}, 1, 1);
    Token              te("", EOF_TOKEN, std::monostate{}, 1, 5);
    std::vector<Token> expected = {t1, te};

    std::string source = "_123";
    Lexer       lexer(source);
    auto        actual = lexer.scanTokens();

    bool res = equalTokenVectors(expected, actual);
    if (!res) printExpectedVsActual(expected, actual);
    ASSERT_TRUE(res);
}

// Numeric-with-underscore should be an invalid number, not IDENT + INT
TEST(Errors_Number, NumberWithUnderscoreInside)
{
    Token              t1("1_2", ERROR, std::string("Invalid number"), 1, 1);
    std::vector<Token> expected = {t1};

    std::string source = "1_2";
    Lexer       lexer(source);
    auto        actual = lexer.scanTokens();

    bool res = equalTokenVectors(expected, actual);
    if (!res) printExpectedVsActual(expected, actual);
    ASSERT_TRUE(res);
}