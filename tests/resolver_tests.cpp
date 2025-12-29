#include "ast/expr.h"
#include "ast/stmt.h"
#include "parser/parser.h"
#include "sema/analyzer.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

// ----------------------
// Test-only AST utilities
// ----------------------

/**
 * @brief Recursively collects all identifier expressions from an expression node.
 * @param expr The expression to traverse
 * @param out Vector to accumulate identifier pointers
 */
static void collectIdentifiersExpr(const Expr& expr, std::vector<const IdentifierExpr*>& out);

/**
 * @brief Recursively collects all identifier expressions from a statement node.
 *
 * Traverses the statement tree, visiting all nested expressions and sub-statements
 * to find identifier usages. Handles all statement types including blocks, control flow,
 * and declarations.
 *
 * @param stmt The statement to traverse
 * @param out Vector to accumulate identifier pointers
 */
static void collectIdentifiersStmt(const Stmt& stmt, std::vector<const IdentifierExpr*>& out)
{
    switch (stmt.kind)
    {
    case Summon:
    {
        auto& s = static_cast<const SummonStmt&>(stmt);
        collectIdentifiersExpr(s.getInitializer(), out);
        return;
    }
    case Say:
    {
        auto& s = static_cast<const SayStmt&>(stmt);
        collectIdentifiersExpr(s.getExpression(), out);
        return;
    }
    case Block:
    {
        auto& s = static_cast<const BlockStmt&>(stmt);
        for (auto& inner : s)
            collectIdentifiersStmt(*inner, out);
        return;
    }
    case IfChain:
    {
        auto& s = static_cast<const IfChainStmt&>(stmt);
        for (auto& branch : s.getBranches())
        {
            auto& cond = *std::get<0>(branch);
            auto& blk = *std::get<1>(branch);
            collectIdentifiersExpr(cond, out);
            collectIdentifiersStmt(blk, out);
        }
        if (s.getElseBranch())
        {
            collectIdentifiersStmt(*s.getElseBranch(), out);
        }
        return;
    }
    case While:
    {
        auto& s = static_cast<const WhileStmt&>(stmt);
        collectIdentifiersExpr(s.getCondition(), out);
        collectIdentifiersStmt(s.getBody(), out);
        return;
    }
    default:
        return;
    }
}

/**
 * @brief Recursively collects all identifier expressions from an expression node.
 *
 * Traverses the expression tree depth-first, collecting pointers to all IdentifierExpr
 * nodes found. Handles all expression types including unary, binary, grouping, and
 * interpolated strings.
 *
 * @param expr The expression to traverse
 * @param out Vector to accumulate identifier pointers
 */
static void collectIdentifiersExpr(const Expr& expr, std::vector<const IdentifierExpr*>& out)
{
    switch (expr.kind)
    {
    case Identifier:
    {
        out.push_back(&static_cast<const IdentifierExpr&>(expr));
        return;
    }
    case Unary:
    {
        auto& e = static_cast<const UnaryExpr&>(expr);
        collectIdentifiersExpr(e.getOperand(), out);
        return;
    }
    case Binary:
    {
        auto& e = static_cast<const BinaryExpr&>(expr);
        collectIdentifiersExpr(e.getLeft(), out);
        collectIdentifiersExpr(e.getRight(), out);
        return;
    }
    case Grouping:
    {
        auto& e = static_cast<const GroupingExpr&>(expr);
        collectIdentifiersExpr(e.getExpression(), out);
        return;
    }
    case InterpolatedString:
    {
        auto& e = static_cast<const StringExpr&>(expr);
        for (auto& part : e.getParts())
        {
            if (part.kind == StringPart::EXPR)
            {
                collectIdentifiersExpr(*part.expr, out);
            }
        }
        return;
    }
    default:
        return; // literals
    }
}

/**
 * @brief Collects all identifier expressions from a program's top-level statements.
 *
 * Convenience function that creates an empty vector and traverses all statements
 * in the program to collect identifier nodes.
 *
 * @param program The parsed program to traverse
 * @return Vector containing pointers to all identifier expressions in the program
 */
static std::vector<const IdentifierExpr*> collectIdentifiers(const Program& program)
{
    std::vector<const IdentifierExpr*> out;
    for (auto& stmt : program)
        collectIdentifiersStmt(*stmt, out);
    return out;
}

/**
 * @brief Looks up the resolved symbol for a given identifier expression.
 *
 * Queries the resolution table to find which symbol (declaration) an identifier
 * reference was bound to during semantic analysis.
 *
 * @param res The semantic analysis result containing the resolution table
 * @param id The identifier expression to look up
 * @return Pointer to the resolved Symbol, or nullptr if not found
 */
static const Symbol* resolvedSymbol(const SemanticResult& res, const IdentifierExpr* id)
{
    auto it = res.resolutionTable.mapping.find(const_cast<IdentifierExpr*>(id));
    if (it == res.resolutionTable.mapping.end())
        return nullptr;
    return it->second;
}

/**
 * @brief Finds a specific identifier by name and source location.
 *
 * Searches through a collection of identifier expressions to find one matching
 * the given name and exact source position. Useful for pinpointing specific
 * identifier usages in test assertions.
 *
 * @param ids Vector of identifier expressions to search
 * @param name The identifier name to match
 * @param line Source line number (1-based)
 * @param col Source column number (1-based)
 * @return Pointer to the matching identifier, or nullptr if not found
 */
static const IdentifierExpr* findIdentAt(const std::vector<const IdentifierExpr*>& ids,
                                         const std::string& name, int line, int col)
{
    for (auto* id : ids)
    {
        if (id->getName() == name && id->loc.line == line && id->loc.col == col)
        {
            return id;
        }
    }
    return nullptr;
}

// ----------------------
// Happy path tests
// ----------------------

/**
 * summon age = 10;
 * say age;
 *
 * Baseline: identifier resolves to global declaration.
 */
TEST(Resolver_Basics, GlobalDeclareThenUseInSay)
{
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("age", IDENTIFIER, std::monostate{}, 1, 8),
        Token("=", EQUAL, std::monostate{}, 1, 12),
        Token("10", INTEGER, 10, 1, 14),
        Token(";", SEMI_COLON, std::monostate{}, 1, 16),

        Token("say", SAY, std::monostate{}, 2, 1),
        Token("age", IDENTIFIER, std::monostate{}, 2, 5),
        Token(";", SEMI_COLON, std::monostate{}, 2, 8),

        Token("", EOF_TOKEN, std::monostate{}, 2, 9),
    };

    Parser  parser(tokens);
    Program program = parser.parseProgram();
    ASSERT_FALSE(program.hadError()); // parser should succeed

    Resolver       resolver;
    SemanticResult res = resolver.resolve(program);

    ASSERT_FALSE(res.hadError());
    ASSERT_EQ(res.diagnostics.size(), 0u);

    auto ids = collectIdentifiers(program);
    auto useAge = findIdentAt(ids, "age", 2, 5);
    ASSERT_NE(useAge, nullptr);

    const Symbol* sym = resolvedSymbol(res, useAge);
    ASSERT_NE(sym, nullptr);
    ASSERT_EQ(sym->name, "age");
    ASSERT_EQ(sym->declLoc.line, 1);
    ASSERT_EQ(sym->declLoc.col, 8);
}

/**
 * summon x = 1;
 * summon y = x + 2;
 *
 * Tests resolution inside initializer expressions.
 */
TEST(Resolver_Basics, UseInInitializerExpression)
{
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("x", IDENTIFIER, std::monostate{}, 1, 8),
        Token("=", EQUAL, std::monostate{}, 1, 10),
        Token("1", INTEGER, 1, 1, 12),
        Token(";", SEMI_COLON, std::monostate{}, 1, 13),

        Token("summon", SUMMON, std::monostate{}, 2, 1),
        Token("y", IDENTIFIER, std::monostate{}, 2, 8),
        Token("=", EQUAL, std::monostate{}, 2, 10),
        Token("x", IDENTIFIER, std::monostate{}, 2, 12),
        Token("+", PLUS, std::monostate{}, 2, 14),
        Token("2", INTEGER, 2, 2, 16),
        Token(";", SEMI_COLON, std::monostate{}, 2, 17),

        Token("", EOF_TOKEN, std::monostate{}, 2, 18),
    };

    Parser  parser(tokens);
    Program program = parser.parseProgram();
    ASSERT_FALSE(program.hadError());

    Resolver       resolver;
    SemanticResult res = resolver.resolve(program);

    ASSERT_FALSE(res.hadError());
    ASSERT_EQ(res.diagnostics.size(), 0u);

    auto ids = collectIdentifiers(program);
    auto useX = findIdentAt(ids, "x", 2, 12);
    ASSERT_NE(useX, nullptr);

    const Symbol* sym = resolvedSymbol(res, useX);
    ASSERT_NE(sym, nullptr);
    ASSERT_EQ(sym->name, "x");
    ASSERT_EQ(sym->declLoc.line, 1);
    ASSERT_EQ(sym->declLoc.col, 8);
}

/**
 * summon x = 1;
 * { say x; }
 *
 * Tests: blocks create scopes, but lookup finds parent symbol.
 */
TEST(Resolver_Scopes, BlockUsesOuterBinding)
{
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("x", IDENTIFIER, std::monostate{}, 1, 8),
        Token("=", EQUAL, std::monostate{}, 1, 10),
        Token("1", INTEGER, 1, 1, 12),
        Token(";", SEMI_COLON, std::monostate{}, 1, 13),

        Token("{", LEFT_BRACE, std::monostate{}, 2, 1),
        Token("say", SAY, std::monostate{}, 2, 3),
        Token("x", IDENTIFIER, std::monostate{}, 2, 7),
        Token(";", SEMI_COLON, std::monostate{}, 2, 8),
        Token("}", RIGHT_BRACE, std::monostate{}, 2, 10),

        Token("", EOF_TOKEN, std::monostate{}, 2, 11),
    };

    Parser  parser(tokens);
    Program program = parser.parseProgram();
    ASSERT_FALSE(program.hadError());

    Resolver       resolver;
    SemanticResult res = resolver.resolve(program);

    ASSERT_FALSE(res.hadError());
    ASSERT_EQ(res.diagnostics.size(), 0u);

    auto ids = collectIdentifiers(program);
    auto useX = findIdentAt(ids, "x", 2, 7);
    ASSERT_NE(useX, nullptr);

    const Symbol* sym = resolvedSymbol(res, useX);
    ASSERT_NE(sym, nullptr);
    ASSERT_EQ(sym->name, "x");
    ASSERT_EQ(sym->declLoc.line, 1);
    ASSERT_EQ(sym->declLoc.col, 8);
}

/**
 * summon x = 1;
 * { summon x = 2; say x; }
 *
 * Tests: shadowing is allowed; inner "x" resolves to inner symbol.
 */
TEST(Resolver_Scopes, ShadowingInInnerBlockResolvesToNearest)
{
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("x", IDENTIFIER, std::monostate{}, 1, 8),
        Token("=", EQUAL, std::monostate{}, 1, 10),
        Token("1", INTEGER, 1, 1, 12),
        Token(";", SEMI_COLON, std::monostate{}, 1, 13),

        Token("{", LEFT_BRACE, std::monostate{}, 2, 1),
        Token("summon", SUMMON, std::monostate{}, 2, 3),
        Token("x", IDENTIFIER, std::monostate{}, 2, 10),
        Token("=", EQUAL, std::monostate{}, 2, 12),
        Token("2", INTEGER, 2, 2, 14),
        Token(";", SEMI_COLON, std::monostate{}, 2, 15),

        Token("say", SAY, std::monostate{}, 3, 3),
        Token("x", IDENTIFIER, std::monostate{}, 3, 7),
        Token(";", SEMI_COLON, std::monostate{}, 3, 8),
        Token("}", RIGHT_BRACE, std::monostate{}, 4, 1),

        Token("", EOF_TOKEN, std::monostate{}, 4, 2),
    };

    Parser  parser(tokens);
    Program program = parser.parseProgram();
    ASSERT_FALSE(program.hadError());

    Resolver       resolver;
    SemanticResult res = resolver.resolve(program);

    ASSERT_FALSE(res.hadError());
    ASSERT_EQ(res.diagnostics.size(), 0u);

    auto ids = collectIdentifiers(program);
    auto useX = findIdentAt(ids, "x", 3, 7);
    ASSERT_NE(useX, nullptr);

    const Symbol* sym = resolvedSymbol(res, useX);
    ASSERT_NE(sym, nullptr);

    // Should bind to inner x at (2,10)
    ASSERT_EQ(sym->name, "x");
    ASSERT_EQ(sym->declLoc.line, 2);
    ASSERT_EQ(sym->declLoc.col, 10);
}

/**
 * should (age >= 10) { summon msg = "hi {age}"; say msg; }
 *
 * Tests: resolution inside interpolated strings within control flow.
 */
TEST(Resolver_Strings, InterpolationResolvesIdentifierInsideIfBranch)
{
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("age", IDENTIFIER, std::monostate{}, 1, 8),
        Token("=", EQUAL, std::monostate{}, 1, 12),
        Token("10", INTEGER, 10, 1, 14),
        Token(";", SEMI_COLON, std::monostate{}, 1, 16),

        Token("should", SHOULD, std::monostate{}, 2, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 2, 8),
        Token("age", IDENTIFIER, std::monostate{}, 2, 9),
        Token(">=", GREATER_EQUAL, std::monostate{}, 2, 13),
        Token("10", INTEGER, 10, 2, 16),
        Token(")", RIGHT_PAREN, std::monostate{}, 2, 18),
        Token("{", LEFT_BRACE, std::monostate{}, 2, 20),

        Token("summon", SUMMON, std::monostate{}, 3, 5),
        Token("msg", IDENTIFIER, std::monostate{}, 3, 12),
        Token("=", EQUAL, std::monostate{}, 3, 16),
        Token("\"hi \"", STRING, std::string("hi "), 3, 18),
        Token("{", INTERP_START, std::monostate{}, 3, 23),
        Token("age", IDENTIFIER, std::monostate{}, 3, 24),
        Token("}", INTERP_END, std::monostate{}, 3, 27),
        Token("\"\"", STRING, std::string(""), 3, 28),
        Token(";", SEMI_COLON, std::monostate{}, 3, 30),

        Token("say", SAY, std::monostate{}, 4, 5),
        Token("msg", IDENTIFIER, std::monostate{}, 4, 9),
        Token(";", SEMI_COLON, std::monostate{}, 4, 12),

        Token("}", RIGHT_BRACE, std::monostate{}, 5, 1),
        Token("", EOF_TOKEN, std::monostate{}, 5, 2),
    };

    Parser  parser(tokens);
    Program program = parser.parseProgram();
    ASSERT_FALSE(program.hadError());

    Resolver       resolver;
    SemanticResult res = resolver.resolve(program);

    ASSERT_FALSE(res.hadError());
    ASSERT_EQ(res.diagnostics.size(), 0u);

    auto ids = collectIdentifiers(program);

    auto interpAge = findIdentAt(ids, "age", 3, 24);
    ASSERT_NE(interpAge, nullptr);

    auto useMsg = findIdentAt(ids, "msg", 4, 9);
    ASSERT_NE(useMsg, nullptr);

    const Symbol* symAge = resolvedSymbol(res, interpAge);
    const Symbol* symMsg = resolvedSymbol(res, useMsg);

    ASSERT_NE(symAge, nullptr);
    ASSERT_NE(symMsg, nullptr);

    ASSERT_EQ(symAge->declLoc.line, 1);
    ASSERT_EQ(symAge->declLoc.col, 8);

    ASSERT_EQ(symMsg->declLoc.line, 3);
    ASSERT_EQ(symMsg->declLoc.col, 12);
}

/**
 * aslongas (age > 0) { say age; }
 *
 * Tests: while loop condition and body resolve outer symbols.
 */
TEST(Resolver_ControlFlow, WhileConditionAndBodyResolveOuterSymbol)
{
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("age", IDENTIFIER, std::monostate{}, 1, 8),
        Token("=", EQUAL, std::monostate{}, 1, 12),
        Token("10", INTEGER, 10, 1, 14),
        Token(";", SEMI_COLON, std::monostate{}, 1, 16),

        Token("aslongas", ASLONGAS, std::monostate{}, 2, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 2, 10),
        Token("age", IDENTIFIER, std::monostate{}, 2, 11),
        Token(">", GREATER, std::monostate{}, 2, 15),
        Token("0", INTEGER, 0, 2, 17),
        Token(")", RIGHT_PAREN, std::monostate{}, 2, 18),
        Token("{", LEFT_BRACE, std::monostate{}, 2, 20),

        Token("say", SAY, std::monostate{}, 3, 5),
        Token("age", IDENTIFIER, std::monostate{}, 3, 9),
        Token(";", SEMI_COLON, std::monostate{}, 3, 12),

        Token("}", RIGHT_BRACE, std::monostate{}, 4, 1),
        Token("", EOF_TOKEN, std::monostate{}, 4, 2),
    };

    Parser  parser(tokens);
    Program program = parser.parseProgram();
    ASSERT_FALSE(program.hadError());

    Resolver       resolver;
    SemanticResult res = resolver.resolve(program);

    ASSERT_FALSE(res.hadError());
    ASSERT_EQ(res.diagnostics.size(), 0u);

    auto ids = collectIdentifiers(program);

    auto condAge = findIdentAt(ids, "age", 2, 11);
    auto sayAge = findIdentAt(ids, "age", 3, 9);

    ASSERT_NE(condAge, nullptr);
    ASSERT_NE(sayAge, nullptr);

    const Symbol* symCond = resolvedSymbol(res, condAge);
    const Symbol* symSay = resolvedSymbol(res, sayAge);

    ASSERT_NE(symCond, nullptr);
    ASSERT_NE(symSay, nullptr);

    ASSERT_EQ(symCond->declLoc.line, 1);
    ASSERT_EQ(symSay->declLoc.line, 1);

    ASSERT_EQ(symCond->declLoc.col, 8);
    ASSERT_EQ(symSay->declLoc.col, 8);
}

/**
 * Multi-branch if-chain:
 * should (age >= 10) { say age; }
 * otherwise should (age > 5) { say age; }
 * otherwise { say age; }
 *
 * Tests: each branch gets its own scope (your rule), but still resolves outer symbol.
 */
TEST(Resolver_ControlFlow, IfChainAllBranchesResolveOuterSymbols)
{
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("age", IDENTIFIER, std::monostate{}, 1, 8),
        Token("=", EQUAL, std::monostate{}, 1, 12),
        Token("10", INTEGER, 10, 1, 14),
        Token(";", SEMI_COLON, std::monostate{}, 1, 16),

        // should (age >= 10) { say age; }
        Token("should", SHOULD, std::monostate{}, 2, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 2, 8),
        Token("age", IDENTIFIER, std::monostate{}, 2, 9),
        Token(">=", GREATER_EQUAL, std::monostate{}, 2, 13),
        Token("10", INTEGER, 10, 2, 16),
        Token(")", RIGHT_PAREN, std::monostate{}, 2, 18),
        Token("{", LEFT_BRACE, std::monostate{}, 2, 20),
        Token("say", SAY, std::monostate{}, 3, 5),
        Token("age", IDENTIFIER, std::monostate{}, 3, 9),
        Token(";", SEMI_COLON, std::monostate{}, 3, 12),
        Token("}", RIGHT_BRACE, std::monostate{}, 4, 1),

        // otherwise should (age > 5) { say age; }
        Token("otherwise", OTHERWISE, std::monostate{}, 4, 3),
        Token("should", SHOULD, std::monostate{}, 4, 13),
        Token("(", LEFT_PAREN, std::monostate{}, 4, 20),
        Token("age", IDENTIFIER, std::monostate{}, 4, 21),
        Token(">", GREATER, std::monostate{}, 4, 25),
        Token("5", INTEGER, 5, 4, 27),
        Token(")", RIGHT_PAREN, std::monostate{}, 4, 28),
        Token("{", LEFT_BRACE, std::monostate{}, 4, 30),
        Token("say", SAY, std::monostate{}, 5, 5),
        Token("age", IDENTIFIER, std::monostate{}, 5, 9),
        Token(";", SEMI_COLON, std::monostate{}, 5, 12),
        Token("}", RIGHT_BRACE, std::monostate{}, 6, 1),

        // otherwise { say age; }
        Token("otherwise", OTHERWISE, std::monostate{}, 6, 3),
        Token("{", LEFT_BRACE, std::monostate{}, 6, 13),
        Token("say", SAY, std::monostate{}, 7, 5),
        Token("age", IDENTIFIER, std::monostate{}, 7, 9),
        Token(";", SEMI_COLON, std::monostate{}, 7, 12),
        Token("}", RIGHT_BRACE, std::monostate{}, 8, 1),

        Token("", EOF_TOKEN, std::monostate{}, 8, 2),
    };

    Parser  parser(tokens);
    Program program = parser.parseProgram();
    ASSERT_FALSE(program.hadError());

    Resolver       resolver;
    SemanticResult res = resolver.resolve(program);

    ASSERT_FALSE(res.hadError());
    ASSERT_EQ(res.diagnostics.size(), 0u);

    auto ids = collectIdentifiers(program);

    // We should have multiple "age" identifier uses in conditions and say bodies.
    // Verify each one resolves to decl at (1,8).
    for (auto* id : ids)
    {
        if (id->getName() != "age")
            continue;
        const Symbol* sym = resolvedSymbol(res, id);
        ASSERT_NE(sym, nullptr);
        ASSERT_EQ(sym->declLoc.line, 1);
        ASSERT_EQ(sym->declLoc.col, 8);
    }
}

/**
 * say x;
 * Error: x is not declared.
 */
TEST(Resolver_SingleError, UndeclaredIdentifier)
{
    std::vector<Token> tokens = {
        Token("say", SAY, std::monostate{}, 1, 1),
        Token("x", IDENTIFIER, std::monostate{}, 1, 5),
        Token(";", SEMI_COLON, std::monostate{}, 1, 6),
        Token("", EOF_TOKEN, std::monostate{}, 1, 7),
    };

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult result = resolver.resolve(program);

    ASSERT_EQ(result.diagnostics.size(), 1);
    ASSERT_TRUE(result.hadError());
}

/**
 * summon x = 1;
 * summon x = 2;
 * Error: redeclaration of x in same scope.
 */
TEST(Resolver_SingleError, RedeclarationSameScope)
{
    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1), Token("x", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 10),      Token("1", INTEGER, 1, 1, 12),
        Token(";", SEMI_COLON, {}, 1, 13),

        Token("summon", SUMMON, {}, 2, 1), Token("x", IDENTIFIER, {}, 2, 8),
        Token("=", EQUAL, {}, 2, 10),      Token("2", INTEGER, 2, 2, 12),
        Token(";", SEMI_COLON, {}, 2, 13),

        Token("", EOF_TOKEN, {}, 2, 14),
    };

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult result = resolver.resolve(program);

    ASSERT_EQ(result.diagnostics.size(), 1);
    ASSERT_TRUE(result.hadError());
}

/**
 * summon x = y;
 * Error: y is not declared.
 */
TEST(Resolver_SingleError, UndeclaredInInitializer)
{
    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1), Token("x", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 10),      Token("y", IDENTIFIER, {}, 1, 12),
        Token(";", SEMI_COLON, {}, 1, 13), Token("", EOF_TOKEN, {}, 1, 14),
    };

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult result = resolver.resolve(program);

    ASSERT_EQ(result.diagnostics.size(), 1);
    ASSERT_TRUE(result.hadError());
}

/**
 * should (x) { }
 * Error: x is not declared.
 */
TEST(Resolver_SingleError, UndeclaredInIfCondition)
{
    std::vector<Token> tokens = {
        Token("should", SHOULD, {}, 1, 1), Token("(", LEFT_PAREN, {}, 1, 8),
        Token("x", IDENTIFIER, {}, 1, 9),  Token(")", RIGHT_PAREN, {}, 1, 10),
        Token("{", LEFT_BRACE, {}, 1, 12), Token("}", RIGHT_BRACE, {}, 1, 13),
        Token("", EOF_TOKEN, {}, 1, 14),
    };

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult result = resolver.resolve(program);

    ASSERT_EQ(result.diagnostics.size(), 1);
    ASSERT_TRUE(result.hadError());
}

/**
 * say "hello {x}";
 * Error: x is not declared.
 */
TEST(Resolver_SingleError, UndeclaredInInterpolatedString)
{
    std::vector<Token> tokens = {
        Token("say", SAY, {}, 1, 1),
        Token("\"hello \"", STRING, std::string("hello "), 1, 5),
        Token("{", INTERP_START, {}, 1, 13),
        Token("x", IDENTIFIER, {}, 1, 14),
        Token("}", INTERP_END, {}, 1, 15),
        Token("\"\"", STRING, std::string(""), 1, 16),
        Token(";", SEMI_COLON, {}, 1, 18),
        Token("", EOF_TOKEN, {}, 1, 19),
    };

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult result = resolver.resolve(program);

    ASSERT_EQ(result.diagnostics.size(), 1);
    ASSERT_TRUE(result.hadError());
}

/**
 * aslongas (x) { }
 * Error: x is not declared.
 */
TEST(Resolver_SingleError, UndeclaredInWhileCondition)
{
    std::vector<Token> tokens = {
        Token("aslongas", ASLONGAS, {}, 1, 1), Token("(", LEFT_PAREN, {}, 1, 10),
        Token("x", IDENTIFIER, {}, 1, 11),     Token(")", RIGHT_PAREN, {}, 1, 12),
        Token("{", LEFT_BRACE, {}, 1, 14),     Token("}", RIGHT_BRACE, {}, 1, 15),
        Token("", EOF_TOKEN, {}, 1, 16),
    };

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult result = resolver.resolve(program);

    ASSERT_EQ(result.diagnostics.size(), 1);
    ASSERT_TRUE(result.hadError());
}