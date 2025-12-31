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

// ----------------------------
// Happy path tests (Resolver)
// ----------------------------

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

/**
 * summon x = 1;
 * { summon y = x; }
 * say x;
 *
 * Tests: inner scope can access outer symbol, outer scope unaffected by inner declarations.
 */
TEST(Resolver_Scopes, InnerScopeAccessesOuterThenOuterUsesOwnSymbol)
{
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("x", IDENTIFIER, std::monostate{}, 1, 8),
        Token("=", EQUAL, std::monostate{}, 1, 10),
        Token("1", INTEGER, 1, 1, 12),
        Token(";", SEMI_COLON, std::monostate{}, 1, 13),

        Token("{", LEFT_BRACE, std::monostate{}, 2, 1),
        Token("summon", SUMMON, std::monostate{}, 2, 3),
        Token("y", IDENTIFIER, std::monostate{}, 2, 10),
        Token("=", EQUAL, std::monostate{}, 2, 12),
        Token("x", IDENTIFIER, std::monostate{}, 2, 14),
        Token(";", SEMI_COLON, std::monostate{}, 2, 15),
        Token("}", RIGHT_BRACE, std::monostate{}, 3, 1),

        Token("say", SAY, std::monostate{}, 4, 1),
        Token("x", IDENTIFIER, std::monostate{}, 4, 5),
        Token(";", SEMI_COLON, std::monostate{}, 4, 6),

        Token("", EOF_TOKEN, std::monostate{}, 4, 7),
    };

    Parser  parser(tokens);
    Program program = parser.parseProgram();
    ASSERT_FALSE(program.hadError());

    Resolver       resolver;
    SemanticResult res = resolver.resolve(program);

    ASSERT_FALSE(res.hadError());
    ASSERT_EQ(res.diagnostics.size(), 0u);

    auto ids = collectIdentifiers(program);

    auto innerX = findIdentAt(ids, "x", 2, 14);
    auto outerX = findIdentAt(ids, "x", 4, 5);

    ASSERT_NE(innerX, nullptr);
    ASSERT_NE(outerX, nullptr);

    const Symbol* symInner = resolvedSymbol(res, innerX);
    const Symbol* symOuter = resolvedSymbol(res, outerX);

    ASSERT_NE(symInner, nullptr);
    ASSERT_NE(symOuter, nullptr);

    // Both should resolve to same outer x at (1,8)
    ASSERT_EQ(symInner->declLoc.line, 1);
    ASSERT_EQ(symInner->declLoc.col, 8);
    ASSERT_EQ(symOuter->declLoc.line, 1);
    ASSERT_EQ(symOuter->declLoc.col, 8);
}

/**
 * summon x = 1;
 * { summon x = 2; { summon x = 3; say x; } }
 *
 * Tests: triple-nested shadowing, innermost reference resolves to innermost declaration.
 */
TEST(Resolver_Scopes, TripleNestedShadowing)
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

        Token("{", LEFT_BRACE, std::monostate{}, 3, 3),
        Token("summon", SUMMON, std::monostate{}, 3, 5),
        Token("x", IDENTIFIER, std::monostate{}, 3, 12),
        Token("=", EQUAL, std::monostate{}, 3, 14),
        Token("3", INTEGER, 3, 3, 16),
        Token(";", SEMI_COLON, std::monostate{}, 3, 17),

        Token("say", SAY, std::monostate{}, 4, 5),
        Token("x", IDENTIFIER, std::monostate{}, 4, 9),
        Token(";", SEMI_COLON, std::monostate{}, 4, 10),
        Token("}", RIGHT_BRACE, std::monostate{}, 5, 3),
        Token("}", RIGHT_BRACE, std::monostate{}, 6, 1),

        Token("", EOF_TOKEN, std::monostate{}, 6, 2),
    };

    Parser  parser(tokens);
    Program program = parser.parseProgram();
    ASSERT_FALSE(program.hadError());

    Resolver       resolver;
    SemanticResult res = resolver.resolve(program);

    ASSERT_FALSE(res.hadError());
    ASSERT_EQ(res.diagnostics.size(), 0u);

    auto ids = collectIdentifiers(program);
    auto useX = findIdentAt(ids, "x", 4, 9);
    ASSERT_NE(useX, nullptr);

    const Symbol* sym = resolvedSymbol(res, useX);
    ASSERT_NE(sym, nullptr);

    // Should resolve to innermost x at (3,12)
    ASSERT_EQ(sym->declLoc.line, 3);
    ASSERT_EQ(sym->declLoc.col, 12);
}

/**
 * summon x = 1;
 * { say x; summon x = 2; say x; }
 *
 * Tests: first use in block sees outer, second use sees shadowing inner.
 */
TEST(Resolver_Scopes, ShadowingMidBlock)
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

        Token("summon", SUMMON, std::monostate{}, 3, 3),
        Token("x", IDENTIFIER, std::monostate{}, 3, 10),
        Token("=", EQUAL, std::monostate{}, 3, 12),
        Token("2", INTEGER, 2, 3, 14),
        Token(";", SEMI_COLON, std::monostate{}, 3, 15),

        Token("say", SAY, std::monostate{}, 4, 3),
        Token("x", IDENTIFIER, std::monostate{}, 4, 7),
        Token(";", SEMI_COLON, std::monostate{}, 4, 8),
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

    auto firstUse = findIdentAt(ids, "x", 2, 7);
    auto secondUse = findIdentAt(ids, "x", 4, 7);

    ASSERT_NE(firstUse, nullptr);
    ASSERT_NE(secondUse, nullptr);

    const Symbol* sym1 = resolvedSymbol(res, firstUse);
    const Symbol* sym2 = resolvedSymbol(res, secondUse);

    ASSERT_NE(sym1, nullptr);
    ASSERT_NE(sym2, nullptr);

    // First use sees outer x at (1,8)
    ASSERT_EQ(sym1->declLoc.line, 1);
    ASSERT_EQ(sym1->declLoc.col, 8);

    // Second use sees inner x at (3,10)
    ASSERT_EQ(sym2->declLoc.line, 3);
    ASSERT_EQ(sym2->declLoc.col, 10);
}

/**
 * summon a = 1;
 * summon b = a + a;
 *
 * Tests: multiple uses of same identifier in single expression.
 */
TEST(Resolver_Expressions, MultipleUsesInSingleExpression)
{
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("a", IDENTIFIER, std::monostate{}, 1, 8),
        Token("=", EQUAL, std::monostate{}, 1, 10),
        Token("1", INTEGER, 1, 1, 12),
        Token(";", SEMI_COLON, std::monostate{}, 1, 13),

        Token("summon", SUMMON, std::monostate{}, 2, 1),
        Token("b", IDENTIFIER, std::monostate{}, 2, 8),
        Token("=", EQUAL, std::monostate{}, 2, 10),
        Token("a", IDENTIFIER, std::monostate{}, 2, 12),
        Token("+", PLUS, std::monostate{}, 2, 14),
        Token("a", IDENTIFIER, std::monostate{}, 2, 16),
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

    auto firstA = findIdentAt(ids, "a", 2, 12);
    auto secondA = findIdentAt(ids, "a", 2, 16);

    ASSERT_NE(firstA, nullptr);
    ASSERT_NE(secondA, nullptr);

    const Symbol* sym1 = resolvedSymbol(res, firstA);
    const Symbol* sym2 = resolvedSymbol(res, secondA);

    ASSERT_NE(sym1, nullptr);
    ASSERT_NE(sym2, nullptr);

    // Both should resolve to same declaration at (1,8)
    ASSERT_EQ(sym1->declLoc.line, 1);
    ASSERT_EQ(sym1->declLoc.col, 8);
    ASSERT_EQ(sym2->declLoc.line, 1);
    ASSERT_EQ(sym2->declLoc.col, 8);
}

/**
 * summon x = 1;
 * summon y = (x + x) * x;
 *
 * Tests: nested grouping with multiple identifier references.
 */
TEST(Resolver_Expressions, GroupingWithMultipleReferences)
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
        Token("(", LEFT_PAREN, std::monostate{}, 2, 12),
        Token("x", IDENTIFIER, std::monostate{}, 2, 13),
        Token("+", PLUS, std::monostate{}, 2, 15),
        Token("x", IDENTIFIER, std::monostate{}, 2, 17),
        Token(")", RIGHT_PAREN, std::monostate{}, 2, 18),
        Token("*", STAR, std::monostate{}, 2, 20),
        Token("x", IDENTIFIER, std::monostate{}, 2, 22),
        Token(";", SEMI_COLON, std::monostate{}, 2, 23),

        Token("", EOF_TOKEN, std::monostate{}, 2, 24),
    };

    Parser  parser(tokens);
    Program program = parser.parseProgram();
    ASSERT_FALSE(program.hadError());

    Resolver       resolver;
    SemanticResult res = resolver.resolve(program);

    ASSERT_FALSE(res.hadError());
    ASSERT_EQ(res.diagnostics.size(), 0u);

    auto ids = collectIdentifiers(program);

    // All three 'x' uses in the expression
    auto x1 = findIdentAt(ids, "x", 2, 13);
    auto x2 = findIdentAt(ids, "x", 2, 17);
    auto x3 = findIdentAt(ids, "x", 2, 22);

    ASSERT_NE(x1, nullptr);
    ASSERT_NE(x2, nullptr);
    ASSERT_NE(x3, nullptr);

    const Symbol* sym1 = resolvedSymbol(res, x1);
    const Symbol* sym2 = resolvedSymbol(res, x2);
    const Symbol* sym3 = resolvedSymbol(res, x3);

    ASSERT_NE(sym1, nullptr);
    ASSERT_NE(sym2, nullptr);
    ASSERT_NE(sym3, nullptr);

    // All should resolve to (1,8)
    ASSERT_EQ(sym1->declLoc.line, 1);
    ASSERT_EQ(sym2->declLoc.line, 1);
    ASSERT_EQ(sym3->declLoc.line, 1);
    ASSERT_EQ(sym1->declLoc.col, 8);
    ASSERT_EQ(sym2->declLoc.col, 8);
    ASSERT_EQ(sym3->declLoc.col, 8);
}

/**
 * summon x = 1;
 * summon y = 2;
 * should (x > 0) { say y; } otherwise { say x; }
 *
 * Tests: different branches reference different outer symbols.
 */
TEST(Resolver_ControlFlow, IfBranchesDifferentOuterReferences)
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
        Token("2", INTEGER, 2, 2, 12),
        Token(";", SEMI_COLON, std::monostate{}, 2, 13),

        Token("should", SHOULD, std::monostate{}, 3, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 3, 8),
        Token("x", IDENTIFIER, std::monostate{}, 3, 9),
        Token(">", GREATER, std::monostate{}, 3, 11),
        Token("0", INTEGER, 0, 3, 13),
        Token(")", RIGHT_PAREN, std::monostate{}, 3, 14),
        Token("{", LEFT_BRACE, std::monostate{}, 3, 16),
        Token("say", SAY, std::monostate{}, 3, 18),
        Token("y", IDENTIFIER, std::monostate{}, 3, 22),
        Token(";", SEMI_COLON, std::monostate{}, 3, 23),
        Token("}", RIGHT_BRACE, std::monostate{}, 3, 25),

        Token("otherwise", OTHERWISE, std::monostate{}, 3, 27),
        Token("{", LEFT_BRACE, std::monostate{}, 3, 37),
        Token("say", SAY, std::monostate{}, 3, 39),
        Token("x", IDENTIFIER, std::monostate{}, 3, 43),
        Token(";", SEMI_COLON, std::monostate{}, 3, 44),
        Token("}", RIGHT_BRACE, std::monostate{}, 3, 46),

        Token("", EOF_TOKEN, std::monostate{}, 3, 47),
    };

    Parser  parser(tokens);
    Program program = parser.parseProgram();
    ASSERT_FALSE(program.hadError());

    Resolver       resolver;
    SemanticResult res = resolver.resolve(program);

    ASSERT_FALSE(res.hadError());
    ASSERT_EQ(res.diagnostics.size(), 0u);

    auto ids = collectIdentifiers(program);

    auto condX = findIdentAt(ids, "x", 3, 9);
    auto thenY = findIdentAt(ids, "y", 3, 22);
    auto elseX = findIdentAt(ids, "x", 3, 43);

    ASSERT_NE(condX, nullptr);
    ASSERT_NE(thenY, nullptr);
    ASSERT_NE(elseX, nullptr);

    const Symbol* symCondX = resolvedSymbol(res, condX);
    const Symbol* symThenY = resolvedSymbol(res, thenY);
    const Symbol* symElseX = resolvedSymbol(res, elseX);

    ASSERT_NE(symCondX, nullptr);
    ASSERT_NE(symThenY, nullptr);
    ASSERT_NE(symElseX, nullptr);

    ASSERT_EQ(symCondX->declLoc.line, 1);
    ASSERT_EQ(symCondX->declLoc.col, 8);

    ASSERT_EQ(symThenY->declLoc.line, 2);
    ASSERT_EQ(symThenY->declLoc.col, 8);

    ASSERT_EQ(symElseX->declLoc.line, 1);
    ASSERT_EQ(symElseX->declLoc.col, 8);
}

/**
 * summon x = 1;
 * should (x) { summon x = 2; say x; }
 *
 * Tests: condition references outer, body shadows and uses inner.
 */
TEST(Resolver_ControlFlow, IfConditionOuterBodyInner)
{
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("x", IDENTIFIER, std::monostate{}, 1, 8),
        Token("=", EQUAL, std::monostate{}, 1, 10),
        Token("1", INTEGER, 1, 1, 12),
        Token(";", SEMI_COLON, std::monostate{}, 1, 13),

        Token("should", SHOULD, std::monostate{}, 2, 1),
        Token("(", LEFT_PAREN, std::monostate{}, 2, 8),
        Token("x", IDENTIFIER, std::monostate{}, 2, 9),
        Token(")", RIGHT_PAREN, std::monostate{}, 2, 10),
        Token("{", LEFT_BRACE, std::monostate{}, 2, 12),

        Token("summon", SUMMON, std::monostate{}, 3, 5),
        Token("x", IDENTIFIER, std::monostate{}, 3, 12),
        Token("=", EQUAL, std::monostate{}, 3, 14),
        Token("2", INTEGER, 2, 3, 16),
        Token(";", SEMI_COLON, std::monostate{}, 3, 17),

        Token("say", SAY, std::monostate{}, 4, 5),
        Token("x", IDENTIFIER, std::monostate{}, 4, 9),
        Token(";", SEMI_COLON, std::monostate{}, 4, 10),

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

    auto condX = findIdentAt(ids, "x", 2, 9);
    auto bodyX = findIdentAt(ids, "x", 4, 9);

    ASSERT_NE(condX, nullptr);
    ASSERT_NE(bodyX, nullptr);

    const Symbol* symCond = resolvedSymbol(res, condX);
    const Symbol* symBody = resolvedSymbol(res, bodyX);

    ASSERT_NE(symCond, nullptr);
    ASSERT_NE(symBody, nullptr);

    // Condition sees outer x at (1,8)
    ASSERT_EQ(symCond->declLoc.line, 1);
    ASSERT_EQ(symCond->declLoc.col, 8);

    // Body sees inner x at (3,12)
    ASSERT_EQ(symBody->declLoc.line, 3);
    ASSERT_EQ(symBody->declLoc.col, 12);
}

/**
 * summon name = "Alice";
 * summon greeting = "Hello {name}, welcome {name}!";
 *
 * Tests: multiple interpolations of same identifier in one string.
 */
TEST(Resolver_Strings, MultipleInterpolationsOfSameIdentifier)
{
    std::vector<Token> tokens = {
        Token("summon", SUMMON, std::monostate{}, 1, 1),
        Token("name", IDENTIFIER, std::monostate{}, 1, 8),
        Token("=", EQUAL, std::monostate{}, 1, 13),
        Token("\"Alice\"", STRING, std::string("Alice"), 1, 15),
        Token(";", SEMI_COLON, std::monostate{}, 1, 22),

        Token("summon", SUMMON, std::monostate{}, 2, 1),
        Token("greeting", IDENTIFIER, std::monostate{}, 2, 8),
        Token("=", EQUAL, std::monostate{}, 2, 17),
        Token("\"Hello \"", STRING, std::string("Hello "), 2, 19),
        Token("{", INTERP_START, std::monostate{}, 2, 27),
        Token("name", IDENTIFIER, std::monostate{}, 2, 28),
        Token("}", INTERP_END, std::monostate{}, 2, 32),
        Token("\", welcome \"", STRING, std::string(", welcome "), 2, 33),
        Token("{", INTERP_START, std::monostate{}, 2, 45),
        Token("name", IDENTIFIER, std::monostate{}, 2, 46),
        Token("}", INTERP_END, std::monostate{}, 2, 50),
        Token("\"!\"", STRING, std::string("!"), 2, 51),
        Token(";", SEMI_COLON, std::monostate{}, 2, 54),

        Token("", EOF_TOKEN, std::monostate{}, 2, 55),
    };

    Parser  parser(tokens);
    Program program = parser.parseProgram();
    ASSERT_FALSE(program.hadError());

    Resolver       resolver;
    SemanticResult res = resolver.resolve(program);

    ASSERT_FALSE(res.hadError());
    ASSERT_EQ(res.diagnostics.size(), 0u);

    auto ids = collectIdentifiers(program);

    auto name1 = findIdentAt(ids, "name", 2, 28);
    auto name2 = findIdentAt(ids, "name", 2, 46);

    ASSERT_NE(name1, nullptr);
    ASSERT_NE(name2, nullptr);

    const Symbol* sym1 = resolvedSymbol(res, name1);
    const Symbol* sym2 = resolvedSymbol(res, name2);

    ASSERT_NE(sym1, nullptr);
    ASSERT_NE(sym2, nullptr);

    // Both should resolve to (1,8)
    ASSERT_EQ(sym1->declLoc.line, 1);
    ASSERT_EQ(sym1->declLoc.col, 8);
    ASSERT_EQ(sym2->declLoc.line, 1);
    ASSERT_EQ(sym2->declLoc.col, 8);
}

/**
 * summon x = 1;
 * summon y = 2;
 * summon msg = "{x + y}";
 *
 * Tests: complex expression inside string interpolation.
 */
TEST(Resolver_Strings, InterpolationWithComplexExpression)
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
        Token("2", INTEGER, 2, 2, 12),
        Token(";", SEMI_COLON, std::monostate{}, 2, 13),

        Token("summon", SUMMON, std::monostate{}, 3, 1),
        Token("msg", IDENTIFIER, std::monostate{}, 3, 8),
        Token("=", EQUAL, std::monostate{}, 3, 12),
        Token("\"\"", STRING, std::string(""), 3, 14),
        Token("{", INTERP_START, std::monostate{}, 3, 16),
        Token("x", IDENTIFIER, std::monostate{}, 3, 17),
        Token("+", PLUS, std::monostate{}, 3, 19),
        Token("y", IDENTIFIER, std::monostate{}, 3, 21),
        Token("}", INTERP_END, std::monostate{}, 3, 22),
        Token("\"\"", STRING, std::string(""), 3, 23),
        Token(";", SEMI_COLON, std::monostate{}, 3, 25),

        Token("", EOF_TOKEN, std::monostate{}, 3, 26),
    };

    Parser  parser(tokens);
    Program program = parser.parseProgram();
    ASSERT_FALSE(program.hadError());

    Resolver       resolver;
    SemanticResult res = resolver.resolve(program);

    ASSERT_FALSE(res.hadError());
    ASSERT_EQ(res.diagnostics.size(), 0u);

    auto ids = collectIdentifiers(program);

    auto interpX = findIdentAt(ids, "x", 3, 17);
    auto interpY = findIdentAt(ids, "y", 3, 21);

    ASSERT_NE(interpX, nullptr);
    ASSERT_NE(interpY, nullptr);

    const Symbol* symX = resolvedSymbol(res, interpX);
    const Symbol* symY = resolvedSymbol(res, interpY);

    ASSERT_NE(symX, nullptr);
    ASSERT_NE(symY, nullptr);

    ASSERT_EQ(symX->declLoc.line, 1);
    ASSERT_EQ(symX->declLoc.col, 8);
    ASSERT_EQ(symY->declLoc.line, 2);
    ASSERT_EQ(symY->declLoc.col, 8);
}

/**
 * say x;
 * say y;
 *
 * Error: multiple undeclared identifiers should generate multiple errors.
 */
TEST(Resolver_MultipleErrors, TwoUndeclaredIdentifiers)
{
    std::vector<Token> tokens = {
        Token("say", SAY, std::monostate{}, 1, 1),
        Token("x", IDENTIFIER, std::monostate{}, 1, 5),
        Token(";", SEMI_COLON, std::monostate{}, 1, 6),

        Token("say", SAY, std::monostate{}, 2, 1),
        Token("y", IDENTIFIER, std::monostate{}, 2, 5),
        Token(";", SEMI_COLON, std::monostate{}, 2, 6),

        Token("", EOF_TOKEN, std::monostate{}, 2, 7),
    };

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult result = resolver.resolve(program);

    ASSERT_EQ(result.diagnostics.size(), 2u);
    ASSERT_TRUE(result.hadError());
}

/**
 * summon x = 1;
 * summon x = 2;
 * summon x = 3;
 *
 * Error: redeclaration generates error for each duplicate.
 */
TEST(Resolver_MultipleErrors, MultipleRedeclarations)
{
    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1), Token("x", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 10),      Token("1", INTEGER, 1, 1, 12),
        Token(";", SEMI_COLON, {}, 1, 13),

        Token("summon", SUMMON, {}, 2, 1), Token("x", IDENTIFIER, {}, 2, 8),
        Token("=", EQUAL, {}, 2, 10),      Token("2", INTEGER, 2, 2, 12),
        Token(";", SEMI_COLON, {}, 2, 13),

        Token("summon", SUMMON, {}, 3, 1), Token("x", IDENTIFIER, {}, 3, 8),
        Token("=", EQUAL, {}, 3, 10),      Token("3", INTEGER, 3, 3, 12),
        Token(";", SEMI_COLON, {}, 3, 13),

        Token("", EOF_TOKEN, {}, 3, 14),
    };

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult result = resolver.resolve(program);

    ASSERT_EQ(result.diagnostics.size(), 2u);
    ASSERT_TRUE(result.hadError());
}

/**
 * summon x = a + b;
 *
 * Error: both a and b undeclared, should generate two errors.
 */
TEST(Resolver_MultipleErrors, MultipleUndeclaredInExpression)
{
    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1), Token("x", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 10),      Token("a", IDENTIFIER, {}, 1, 12),
        Token("+", PLUS, {}, 1, 14),       Token("b", IDENTIFIER, {}, 1, 16),
        Token(";", SEMI_COLON, {}, 1, 17), Token("", EOF_TOKEN, {}, 1, 18),
    };

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult result = resolver.resolve(program);

    ASSERT_EQ(result.diagnostics.size(), 2u);
    ASSERT_TRUE(result.hadError());
}

// ----------------------------
// Happy path tests (TypeChecker)
// ----------------------------

TEST(TypeChecker_Basics, GlobalDeclareThenUse)
{
    // summon x = 10;
    // say x;

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1), Token("x", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 10),      Token("10", INTEGER, 10, 1, 12),
        Token(";", SEMI_COLON, {}, 1, 14),

        Token("say", SAY, {}, 2, 1),       Token("x", IDENTIFIER, {}, 2, 5),
        Token(";", SEMI_COLON, {}, 2, 6),

        Token("", EOF_TOKEN, {}, 3, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);

    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Basics, ArithmeticExpression)
{
    // summon x = 1 + 2 * 3;

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1), Token("x", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 10),      Token("1", INTEGER, 1, 1, 12),
        Token("+", PLUS, {}, 1, 14),       Token("2", INTEGER, 2, 1, 16),
        Token("*", STAR, {}, 1, 18),       Token("3", INTEGER, 3, 1, 20),
        Token(";", SEMI_COLON, {}, 1, 21), Token("", EOF_TOKEN, {}, 2, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Basics, IfConditionBool)
{
    // summon x = 10;
    // should (x > 5) { say x; }

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1), Token("x", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 10),      Token("10", INTEGER, 10, 1, 12),
        Token(";", SEMI_COLON, {}, 1, 14),

        Token("should", SHOULD, {}, 2, 1), Token("(", LEFT_PAREN, {}, 2, 8),
        Token("x", IDENTIFIER, {}, 2, 9),  Token(">", GREATER, {}, 2, 11),
        Token("5", INTEGER, 5, 2, 13),     Token(")", RIGHT_PAREN, {}, 2, 14),
        Token("{", LEFT_BRACE, {}, 2, 16),

        Token("say", SAY, {}, 3, 3),       Token("x", IDENTIFIER, {}, 3, 7),
        Token(";", SEMI_COLON, {}, 3, 8),

        Token("}", RIGHT_BRACE, {}, 4, 1), Token("", EOF_TOKEN, {}, 5, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Basics, StringLiteral)
{
    // summon name = "Alice";
    // say name;

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1), Token("name", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 13),      Token("\"Alice\"", STRING, std::string("Alice"), 1, 15),
        Token(";", SEMI_COLON, {}, 1, 22),

        Token("say", SAY, {}, 2, 1),       Token("name", IDENTIFIER, {}, 2, 5),
        Token(";", SEMI_COLON, {}, 2, 9),

        Token("", EOF_TOKEN, {}, 3, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Basics, BooleanLiteral)
{
    // summon flag = affirmative;
    // summon other = negative;

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1), Token("flag", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 13),      Token("affirmative", BOOL, true, 1, 15),
        Token(";", SEMI_COLON, {}, 1, 26),

        Token("summon", SUMMON, {}, 2, 1), Token("other", IDENTIFIER, {}, 2, 8),
        Token("=", EQUAL, {}, 2, 14),      Token("negative", BOOL, false, 2, 16),
        Token(";", SEMI_COLON, {}, 2, 24),

        Token("", EOF_TOKEN, {}, 3, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Arithmetic, AdditionOfIntegers)
{
    // summon a = 10;
    // summon b = 20;
    // summon c = a + b;

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1), Token("a", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 10),      Token("10", INTEGER, 10, 1, 12),
        Token(";", SEMI_COLON, {}, 1, 14),

        Token("summon", SUMMON, {}, 2, 1), Token("b", IDENTIFIER, {}, 2, 8),
        Token("=", EQUAL, {}, 2, 10),      Token("20", INTEGER, 20, 2, 12),
        Token(";", SEMI_COLON, {}, 2, 14),

        Token("summon", SUMMON, {}, 3, 1), Token("c", IDENTIFIER, {}, 3, 8),
        Token("=", EQUAL, {}, 3, 10),      Token("a", IDENTIFIER, {}, 3, 12),
        Token("+", PLUS, {}, 3, 14),       Token("b", IDENTIFIER, {}, 3, 16),
        Token(";", SEMI_COLON, {}, 3, 17),

        Token("", EOF_TOKEN, {}, 4, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Arithmetic, SubtractionOfIntegers)
{
    // summon diff = 100 - 42;

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1), Token("diff", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 13),      Token("100", INTEGER, 100, 1, 15),
        Token("-", MINUS, {}, 1, 19),      Token("42", INTEGER, 42, 1, 21),
        Token(";", SEMI_COLON, {}, 1, 23),

        Token("", EOF_TOKEN, {}, 2, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Arithmetic, MultiplicationOfIntegers)
{
    // summon product = 7 * 6;

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1), Token("product", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 16),      Token("7", INTEGER, 7, 1, 18),
        Token("*", STAR, {}, 1, 20),       Token("6", INTEGER, 6, 1, 22),
        Token(";", SEMI_COLON, {}, 1, 23),

        Token("", EOF_TOKEN, {}, 2, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Arithmetic, DivisionOfIntegers)
{
    // summon quotient = 50 / 5;

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1), Token("quotient", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 17),      Token("50", INTEGER, 50, 1, 19),
        Token("/", SLASH, {}, 1, 22),      Token("5", INTEGER, 5, 1, 24),
        Token(";", SEMI_COLON, {}, 1, 25),

        Token("", EOF_TOKEN, {}, 2, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Arithmetic, ComplexExpression)
{
    // summon result = (10 + 5) * 2 - 8 / 4;

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1), Token("result", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 15),      Token("(", LEFT_PAREN, {}, 1, 17),
        Token("10", INTEGER, 10, 1, 18),   Token("+", PLUS, {}, 1, 21),
        Token("5", INTEGER, 5, 1, 23),     Token(")", RIGHT_PAREN, {}, 1, 24),
        Token("*", STAR, {}, 1, 26),       Token("2", INTEGER, 2, 1, 28),
        Token("-", MINUS, {}, 1, 30),      Token("8", INTEGER, 8, 1, 32),
        Token("/", SLASH, {}, 1, 34),      Token("4", INTEGER, 4, 1, 36),
        Token(";", SEMI_COLON, {}, 1, 37),

        Token("", EOF_TOKEN, {}, 2, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Arithmetic, UnaryNegation)
{
    // summon neg = -42;

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1), Token("neg", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 12),      Token("-", MINUS, {}, 1, 14),
        Token("42", INTEGER, 42, 1, 15),   Token(";", SEMI_COLON, {}, 1, 17),

        Token("", EOF_TOKEN, {}, 2, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Comparison, GreaterThan)
{
    // summon x = 10;
    // summon y = 5;
    // summon result = x > y;

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1), Token("x", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 10),      Token("10", INTEGER, 10, 1, 12),
        Token(";", SEMI_COLON, {}, 1, 14),

        Token("summon", SUMMON, {}, 2, 1), Token("y", IDENTIFIER, {}, 2, 8),
        Token("=", EQUAL, {}, 2, 10),      Token("5", INTEGER, 5, 2, 12),
        Token(";", SEMI_COLON, {}, 2, 13),

        Token("summon", SUMMON, {}, 3, 1), Token("result", IDENTIFIER, {}, 3, 8),
        Token("=", EQUAL, {}, 3, 15),      Token("x", IDENTIFIER, {}, 3, 17),
        Token(">", GREATER, {}, 3, 19),    Token("y", IDENTIFIER, {}, 3, 21),
        Token(";", SEMI_COLON, {}, 3, 22),

        Token("", EOF_TOKEN, {}, 4, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Comparison, GreaterThanOrEqual)
{
    // summon check = 10 >= 10;

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1),     Token("check", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 14),          Token("10", INTEGER, 10, 1, 16),
        Token(">=", GREATER_EQUAL, {}, 1, 19), Token("10", INTEGER, 10, 1, 22),
        Token(";", SEMI_COLON, {}, 1, 24),

        Token("", EOF_TOKEN, {}, 2, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Comparison, LessThan)
{
    // summon check = 3 < 7;

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1), Token("check", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 14),      Token("3", INTEGER, 3, 1, 16),
        Token("<", LESS, {}, 1, 18),       Token("7", INTEGER, 7, 1, 20),
        Token(";", SEMI_COLON, {}, 1, 21),

        Token("", EOF_TOKEN, {}, 2, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Comparison, LessThanOrEqual)
{
    // summon check = 5 <= 5;

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1),  Token("check", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 14),       Token("5", INTEGER, 5, 1, 16),
        Token("<=", LESS_EQUAL, {}, 1, 18), Token("5", INTEGER, 5, 1, 21),
        Token(";", SEMI_COLON, {}, 1, 22),

        Token("", EOF_TOKEN, {}, 2, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Equality, IntegerEquality)
{
    // summon check = 42 == 42;

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1),   Token("check", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 14),        Token("42", INTEGER, 42, 1, 16),
        Token("==", EQUAL_EQUAL, {}, 1, 19), Token("42", INTEGER, 42, 1, 22),
        Token(";", SEMI_COLON, {}, 1, 24),

        Token("", EOF_TOKEN, {}, 2, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Equality, IntegerInequality)
{
    // summon check = 10 != 20;

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1),  Token("check", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 14),       Token("10", INTEGER, 10, 1, 16),
        Token("!=", BANG_EQUAL, {}, 1, 19), Token("20", INTEGER, 20, 1, 22),
        Token(";", SEMI_COLON, {}, 1, 24),

        Token("", EOF_TOKEN, {}, 2, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Equality, StringEquality)
{
    // summon check = "hello" == "hello";

    std::vector<Token> tokens = {Token("summon", SUMMON, {}, 1, 1),
                                 Token("check", IDENTIFIER, {}, 1, 8),
                                 Token("=", EQUAL, {}, 1, 14),
                                 Token("\"hello\"", STRING, std::string("hello"), 1, 16),
                                 Token("==", EQUAL_EQUAL, {}, 1, 24),
                                 Token("\"hello\"", STRING, std::string("hello"), 1, 27),
                                 Token(";", SEMI_COLON, {}, 1, 35),

                                 Token("", EOF_TOKEN, {}, 2, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Equality, BooleanEquality)
{
    // summon check = affirmative == negative;

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1),   Token("check", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 14),        Token("affirmative", BOOL, true, 1, 16),
        Token("==", EQUAL_EQUAL, {}, 1, 28), Token("negative", BOOL, false, 1, 31),
        Token(";", SEMI_COLON, {}, 1, 39),

        Token("", EOF_TOKEN, {}, 2, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Logical, NotOperator)
{
    // summon result = not affirmative;

    std::vector<Token> tokens = {Token("summon", SUMMON, {}, 1, 1),
                                 Token("result", IDENTIFIER, {}, 1, 8),
                                 Token("=", EQUAL, {}, 1, 15),
                                 Token("not", NOT, {}, 1, 17),
                                 Token("affirmative", BOOL, true, 1, 21),
                                 Token(";", SEMI_COLON, {}, 1, 32),

                                 Token("", EOF_TOKEN, {}, 2, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Logical, NotWithNegation)
{
    // summon x = 10;
    // summon result = not (x > 5);

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1),  Token("x", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 10),       Token("10", INTEGER, 10, 1, 12),
        Token(";", SEMI_COLON, {}, 1, 14),

        Token("summon", SUMMON, {}, 2, 1),  Token("result", IDENTIFIER, {}, 2, 8),
        Token("=", EQUAL, {}, 2, 15),       Token("not", NOT, {}, 2, 17),
        Token("(", LEFT_PAREN, {}, 2, 21),  Token("x", IDENTIFIER, {}, 2, 22),
        Token(">", GREATER, {}, 2, 24),     Token("5", INTEGER, 5, 2, 26),
        Token(")", RIGHT_PAREN, {}, 2, 27), Token(";", SEMI_COLON, {}, 2, 28),

        Token("", EOF_TOKEN, {}, 3, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Logical, DoubleNegation)
{
    // summon result = not not affirmative;

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1), Token("result", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 15),      Token("not", NOT, {}, 1, 17),
        Token("not", NOT, {}, 1, 21),      Token("affirmative", BOOL, true, 1, 25),
        Token(";", SEMI_COLON, {}, 1, 36),

        Token("", EOF_TOKEN, {}, 2, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Comparison, ComparingBooleanVariables)
{
    // summon flag1 = affirmative;
    // summon flag2 = negative;
    // summon result = flag1 == flag2;

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1),   Token("flag1", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 14),        Token("affirmative", BOOL, true, 1, 16),
        Token(";", SEMI_COLON, {}, 1, 27),

        Token("summon", SUMMON, {}, 2, 1),   Token("flag2", IDENTIFIER, {}, 2, 8),
        Token("=", EQUAL, {}, 2, 14),        Token("negative", BOOL, false, 2, 16),
        Token(";", SEMI_COLON, {}, 2, 24),

        Token("summon", SUMMON, {}, 3, 1),   Token("result", IDENTIFIER, {}, 3, 8),
        Token("=", EQUAL, {}, 3, 15),        Token("flag1", IDENTIFIER, {}, 3, 17),
        Token("==", EQUAL_EQUAL, {}, 3, 23), Token("flag2", IDENTIFIER, {}, 3, 26),
        Token(";", SEMI_COLON, {}, 3, 31),

        Token("", EOF_TOKEN, {}, 4, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_ControlFlow, WhileLoopWithIntCondition)
{
    // summon counter = 10;
    // aslongas (counter > 0) { say counter; }

    std::vector<Token> tokens = {Token("summon", SUMMON, {}, 1, 1),
                                 Token("counter", IDENTIFIER, {}, 1, 8),
                                 Token("=", EQUAL, {}, 1, 16),
                                 Token("10", INTEGER, 10, 1, 18),
                                 Token(";", SEMI_COLON, {}, 1, 20),

                                 Token("aslongas", ASLONGAS, {}, 2, 1),
                                 Token("(", LEFT_PAREN, {}, 2, 10),
                                 Token("counter", IDENTIFIER, {}, 2, 11),
                                 Token(">", GREATER, {}, 2, 19),
                                 Token("0", INTEGER, 0, 2, 21),
                                 Token(")", RIGHT_PAREN, {}, 2, 22),
                                 Token("{", LEFT_BRACE, {}, 2, 24),

                                 Token("say", SAY, {}, 3, 3),
                                 Token("counter", IDENTIFIER, {}, 3, 7),
                                 Token(";", SEMI_COLON, {}, 3, 14),

                                 Token("}", RIGHT_BRACE, {}, 4, 1),
                                 Token("", EOF_TOKEN, {}, 5, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_ControlFlow, IfWithElseBranches)
{
    // summon x = 10;
    // should (x > 5) { say x; } otherwise { say 0; }

    std::vector<Token> tokens = {Token("summon", SUMMON, {}, 1, 1),
                                 Token("x", IDENTIFIER, {}, 1, 8),
                                 Token("=", EQUAL, {}, 1, 10),
                                 Token("10", INTEGER, 10, 1, 12),
                                 Token(";", SEMI_COLON, {}, 1, 14),

                                 Token("should", SHOULD, {}, 2, 1),
                                 Token("(", LEFT_PAREN, {}, 2, 8),
                                 Token("x", IDENTIFIER, {}, 2, 9),
                                 Token(">", GREATER, {}, 2, 11),
                                 Token("5", INTEGER, 5, 2, 13),
                                 Token(")", RIGHT_PAREN, {}, 2, 14),
                                 Token("{", LEFT_BRACE, {}, 2, 16),

                                 Token("say", SAY, {}, 3, 3),
                                 Token("x", IDENTIFIER, {}, 3, 7),
                                 Token(";", SEMI_COLON, {}, 3, 8),

                                 Token("}", RIGHT_BRACE, {}, 4, 1),

                                 Token("otherwise", OTHERWISE, {}, 4, 3),
                                 Token("{", LEFT_BRACE, {}, 4, 13),
                                 Token("say", SAY, {}, 5, 3),
                                 Token("0", INTEGER, 0, 5, 7),
                                 Token(";", SEMI_COLON, {}, 5, 8),
                                 Token("}", RIGHT_BRACE, {}, 6, 1),

                                 Token("", EOF_TOKEN, {}, 7, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_ControlFlow, MultipleIfChainBranches)
{
    // summon score = 85;
    // should (score >= 90) { say "A"; }
    // otherwise should (score >= 80) { say "B"; }
    // otherwise { say "C"; }

    std::vector<Token> tokens = {Token("summon", SUMMON, {}, 1, 1),
                                 Token("score", IDENTIFIER, {}, 1, 8),
                                 Token("=", EQUAL, {}, 1, 14),
                                 Token("85", INTEGER, 85, 1, 16),
                                 Token(";", SEMI_COLON, {}, 1, 18),

                                 Token("should", SHOULD, {}, 2, 1),
                                 Token("(", LEFT_PAREN, {}, 2, 8),
                                 Token("score", IDENTIFIER, {}, 2, 9),
                                 Token(">=", GREATER_EQUAL, {}, 2, 15),
                                 Token("90", INTEGER, 90, 2, 18),
                                 Token(")", RIGHT_PAREN, {}, 2, 20),
                                 Token("{", LEFT_BRACE, {}, 2, 22),
                                 Token("say", SAY, {}, 2, 24),
                                 Token("\"A\"", STRING, std::string("A"), 2, 28),
                                 Token(";", SEMI_COLON, {}, 2, 31),
                                 Token("}", RIGHT_BRACE, {}, 2, 33),

                                 Token("otherwise", OTHERWISE, {}, 3, 1),
                                 Token("should", SHOULD, {}, 3, 11),
                                 Token("(", LEFT_PAREN, {}, 3, 18),
                                 Token("score", IDENTIFIER, {}, 3, 19),
                                 Token(">=", GREATER_EQUAL, {}, 3, 25),
                                 Token("80", INTEGER, 80, 3, 28),
                                 Token(")", RIGHT_PAREN, {}, 3, 30),
                                 Token("{", LEFT_BRACE, {}, 3, 32),
                                 Token("say", SAY, {}, 3, 34),
                                 Token("\"B\"", STRING, std::string("B"), 3, 38),
                                 Token(";", SEMI_COLON, {}, 3, 41),
                                 Token("}", RIGHT_BRACE, {}, 3, 43),

                                 Token("otherwise", OTHERWISE, {}, 4, 1),
                                 Token("{", LEFT_BRACE, {}, 4, 11),
                                 Token("say", SAY, {}, 4, 13),
                                 Token("\"C\"", STRING, std::string("C"), 4, 17),
                                 Token(";", SEMI_COLON, {}, 4, 20),
                                 Token("}", RIGHT_BRACE, {}, 4, 22),

                                 Token("", EOF_TOKEN, {}, 5, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Strings, SimpleInterpolation)
{
    // summon name = "Alice";
    // summon greeting = "Hello {name}";

    std::vector<Token> tokens = {Token("summon", SUMMON, {}, 1, 1),
                                 Token("name", IDENTIFIER, {}, 1, 8),
                                 Token("=", EQUAL, {}, 1, 13),
                                 Token("\"Alice\"", STRING, std::string("Alice"), 1, 15),
                                 Token(";", SEMI_COLON, {}, 1, 22),

                                 Token("summon", SUMMON, {}, 2, 1),
                                 Token("greeting", IDENTIFIER, {}, 2, 8),
                                 Token("=", EQUAL, {}, 2, 17),
                                 Token("\"Hello \"", STRING, std::string("Hello "), 2, 19),
                                 Token("{", INTERP_START, {}, 2, 27),
                                 Token("name", IDENTIFIER, {}, 2, 28),
                                 Token("}", INTERP_END, {}, 2, 32),
                                 Token("\"\"", STRING, std::string(""), 2, 33),
                                 Token(";", SEMI_COLON, {}, 2, 35),

                                 Token("", EOF_TOKEN, {}, 3, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Strings, InterpolationWithIntegerExpression)
{
    // summon x = 10;
    // summon y = 20;
    // summon msg = "Sum is {x + y}";

    std::vector<Token> tokens = {Token("summon", SUMMON, {}, 1, 1),
                                 Token("x", IDENTIFIER, {}, 1, 8),
                                 Token("=", EQUAL, {}, 1, 10),
                                 Token("10", INTEGER, 10, 1, 12),
                                 Token(";", SEMI_COLON, {}, 1, 14),

                                 Token("summon", SUMMON, {}, 2, 1),
                                 Token("y", IDENTIFIER, {}, 2, 8),
                                 Token("=", EQUAL, {}, 2, 10),
                                 Token("20", INTEGER, 20, 2, 12),
                                 Token(";", SEMI_COLON, {}, 2, 14),

                                 Token("summon", SUMMON, {}, 3, 1),
                                 Token("msg", IDENTIFIER, {}, 3, 8),
                                 Token("=", EQUAL, {}, 3, 12),
                                 Token("\"Sum is \"", STRING, std::string("Sum is "), 3, 14),
                                 Token("{", INTERP_START, {}, 3, 23),
                                 Token("x", IDENTIFIER, {}, 3, 24),
                                 Token("+", PLUS, {}, 3, 26),
                                 Token("y", IDENTIFIER, {}, 3, 28),
                                 Token("}", INTERP_END, {}, 3, 29),
                                 Token("\"\"", STRING, std::string(""), 3, 30),
                                 Token(";", SEMI_COLON, {}, 3, 32),

                                 Token("", EOF_TOKEN, {}, 4, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Strings, MultipleInterpolations)
{
    // summon first = "John";
    // summon last = "Doe";
    // summon full = "{first} {last}";

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1),   Token("first", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 14),        Token("\"John\"", STRING, std::string("John"), 1, 16),
        Token(";", SEMI_COLON, {}, 1, 22),

        Token("summon", SUMMON, {}, 2, 1),   Token("last", IDENTIFIER, {}, 2, 8),
        Token("=", EQUAL, {}, 2, 13),        Token("\"Doe\"", STRING, std::string("Doe"), 2, 15),
        Token(";", SEMI_COLON, {}, 2, 20),

        Token("summon", SUMMON, {}, 3, 1),   Token("full", IDENTIFIER, {}, 3, 8),
        Token("=", EQUAL, {}, 3, 13),        Token("\"\"", STRING, std::string(""), 3, 15),
        Token("{", INTERP_START, {}, 3, 17), Token("first", IDENTIFIER, {}, 3, 18),
        Token("}", INTERP_END, {}, 3, 23),   Token("\" \"", STRING, std::string(" "), 3, 24),
        Token("{", INTERP_START, {}, 3, 27), Token("last", IDENTIFIER, {}, 3, 28),
        Token("}", INTERP_END, {}, 3, 32),   Token("\"\"", STRING, std::string(""), 3, 33),
        Token(";", SEMI_COLON, {}, 3, 35),

        Token("", EOF_TOKEN, {}, 4, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Scopes, VariableInNestedBlock)
{
    // summon x = 10;
    // { summon y = x + 5; say y; }

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1), Token("x", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 10),      Token("10", INTEGER, 10, 1, 12),
        Token(";", SEMI_COLON, {}, 1, 14),

        Token("{", LEFT_BRACE, {}, 2, 1),  Token("summon", SUMMON, {}, 2, 3),
        Token("y", IDENTIFIER, {}, 2, 10), Token("=", EQUAL, {}, 2, 12),
        Token("x", IDENTIFIER, {}, 2, 14), Token("+", PLUS, {}, 2, 16),
        Token("5", INTEGER, 5, 2, 18),     Token(";", SEMI_COLON, {}, 2, 19),

        Token("say", SAY, {}, 3, 3),       Token("y", IDENTIFIER, {}, 3, 7),
        Token(";", SEMI_COLON, {}, 3, 8),  Token("}", RIGHT_BRACE, {}, 4, 1),

        Token("", EOF_TOKEN, {}, 5, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Scopes, ShadowingWithSameType)
{
    // summon x = 10;
    // { summon x = 20; say x; }

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1), Token("x", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 10),      Token("10", INTEGER, 10, 1, 12),
        Token(";", SEMI_COLON, {}, 1, 14),

        Token("{", LEFT_BRACE, {}, 2, 1),  Token("summon", SUMMON, {}, 2, 3),
        Token("x", IDENTIFIER, {}, 2, 10), Token("=", EQUAL, {}, 2, 12),
        Token("20", INTEGER, 20, 2, 14),   Token(";", SEMI_COLON, {}, 2, 16),

        Token("say", SAY, {}, 3, 3),       Token("x", IDENTIFIER, {}, 3, 7),
        Token(";", SEMI_COLON, {}, 3, 8),  Token("}", RIGHT_BRACE, {}, 4, 1),

        Token("", EOF_TOKEN, {}, 5, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Scopes, ShadowingWithDifferentType)
{
    // summon x = 10;
    // { summon x = "hello"; say x; }

    std::vector<Token> tokens = {Token("summon", SUMMON, {}, 1, 1),
                                 Token("x", IDENTIFIER, {}, 1, 8),
                                 Token("=", EQUAL, {}, 1, 10),
                                 Token("10", INTEGER, 10, 1, 12),
                                 Token(";", SEMI_COLON, {}, 1, 14),

                                 Token("{", LEFT_BRACE, {}, 2, 1),
                                 Token("summon", SUMMON, {}, 2, 3),
                                 Token("x", IDENTIFIER, {}, 2, 10),
                                 Token("=", EQUAL, {}, 2, 12),
                                 Token("\"hello\"", STRING, std::string("hello"), 2, 14),
                                 Token(";", SEMI_COLON, {}, 2, 21),

                                 Token("say", SAY, {}, 3, 3),
                                 Token("x", IDENTIFIER, {}, 3, 7),
                                 Token(";", SEMI_COLON, {}, 3, 8),
                                 Token("}", RIGHT_BRACE, {}, 4, 1),

                                 Token("", EOF_TOKEN, {}, 5, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Mixed, ComplexProgramWithMultipleTypes)
{
    // summon count = 5;
    // summon name = "test";
    // summon active = affirmative;
    // should (count > 0) { say name; }

    std::vector<Token> tokens = {
        Token("summon", SUMMON, {}, 1, 1),    Token("count", IDENTIFIER, {}, 1, 8),
        Token("=", EQUAL, {}, 1, 14),         Token("5", INTEGER, 5, 1, 16),
        Token(";", SEMI_COLON, {}, 1, 17),

        Token("summon", SUMMON, {}, 2, 1),    Token("name", IDENTIFIER, {}, 2, 8),
        Token("=", EQUAL, {}, 2, 13),         Token("\"test\"", STRING, std::string("test"), 2, 15),
        Token(";", SEMI_COLON, {}, 2, 21),

        Token("summon", SUMMON, {}, 3, 1),    Token("active", IDENTIFIER, {}, 3, 8),
        Token("=", EQUAL, {}, 3, 15),         Token("affirmative", BOOL, true, 3, 17),
        Token(";", SEMI_COLON, {}, 3, 28),

        Token("should", SHOULD, {}, 4, 1),    Token("(", LEFT_PAREN, {}, 4, 8),
        Token("count", IDENTIFIER, {}, 4, 9), Token(">", GREATER, {}, 4, 15),
        Token("0", INTEGER, 0, 4, 17),        Token(")", RIGHT_PAREN, {}, 4, 18),
        Token("{", LEFT_BRACE, {}, 4, 20),

        Token("say", SAY, {}, 5, 3),          Token("name", IDENTIFIER, {}, 5, 7),
        Token(";", SEMI_COLON, {}, 5, 11),

        Token("}", RIGHT_BRACE, {}, 6, 1),    Token("", EOF_TOKEN, {}, 7, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}

TEST(TypeChecker_Mixed, ArithmeticInConditionAndInterpolation)
{
    // summon a = 10;
    // summon b = 20;
    // should ((a + b) > 25) { say "Result: {a * b}"; }

    std::vector<Token> tokens = {Token("summon", SUMMON, {}, 1, 1),
                                 Token("a", IDENTIFIER, {}, 1, 8),
                                 Token("=", EQUAL, {}, 1, 10),
                                 Token("10", INTEGER, 10, 1, 12),
                                 Token(";", SEMI_COLON, {}, 1, 14),

                                 Token("summon", SUMMON, {}, 2, 1),
                                 Token("b", IDENTIFIER, {}, 2, 8),
                                 Token("=", EQUAL, {}, 2, 10),
                                 Token("20", INTEGER, 20, 2, 12),
                                 Token(";", SEMI_COLON, {}, 2, 14),

                                 Token("should", SHOULD, {}, 3, 1),
                                 Token("(", LEFT_PAREN, {}, 3, 8),
                                 Token("(", LEFT_PAREN, {}, 3, 9),
                                 Token("a", IDENTIFIER, {}, 3, 10),
                                 Token("+", PLUS, {}, 3, 12),
                                 Token("b", IDENTIFIER, {}, 3, 14),
                                 Token(")", RIGHT_PAREN, {}, 3, 15),
                                 Token(">", GREATER, {}, 3, 17),
                                 Token("25", INTEGER, 25, 3, 19),
                                 Token(")", RIGHT_PAREN, {}, 3, 21),
                                 Token("{", LEFT_BRACE, {}, 3, 23),

                                 Token("say", SAY, {}, 4, 3),
                                 Token("\"Result: \"", STRING, std::string("Result: "), 4, 7),
                                 Token("{", INTERP_START, {}, 4, 17),
                                 Token("a", IDENTIFIER, {}, 4, 18),
                                 Token("*", STAR, {}, 4, 20),
                                 Token("b", IDENTIFIER, {}, 4, 22),
                                 Token("}", INTERP_END, {}, 4, 23),
                                 Token("\"\"", STRING, std::string(""), 4, 24),
                                 Token(";", SEMI_COLON, {}, 4, 26),

                                 Token("}", RIGHT_BRACE, {}, 5, 1),
                                 Token("", EOF_TOKEN, {}, 6, 1)};

    Parser  parser(tokens);
    Program program = parser.parseProgram();

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    ASSERT_FALSE(sema.hadError());

    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);

    ASSERT_FALSE(types.hadError());
}