/**
 * Semantic analysis structures.
 *
 * Note: `Scope` is a low-level data structure.
 * It stores symbols and provides deterministic lexical lookup, but it does not
 * emit diagnostics or make policy decisions.
 */

#pragma once

#include "ast/expr.h"
#include "ast/prog.h"
#include "ast/stmt.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

enum Type
{
    Int,
    Bool,
    String,
    Void,
    Error
};

struct Scope;
/**
 * Represents a semantic error or warning during analysis.
 *
 * Contains the error message and source location where the error occurred,
 * enabling clear error reporting to users.
 */
struct Diagnostic
{
    std::string message;
    SourceLoc   loc;

    std::string toString() const;
};

/**
 * Represents a declared symbol (variable) in the program.
 *
 * Tracks the symbol's name, kind (currently only variables), declaration location,
 * and a pointer to the AST node where it was declared. This allows the TypeChecker
 * to access variable metadata and enables good error diagnostics.
 */
struct Symbol
{
    std::string name;
    enum Kind
    {
        VARIABLE
    } kind;

    SourceLoc declLoc;

    const SummonStmt* declStmt;
};

struct Scope
{
    // Non-owning link to the lexically enclosing scope (nullptr for root).
    Scope* parent = nullptr;

    std::unordered_map<std::string, std::unique_ptr<Symbol>> table;

    std::vector<std::unique_ptr<Scope>> children;

    /**
     * @brief Register a new symbol in the current scope.
     *
     * This operation affects only the current scope's symbol table. It never
     * consults parent scopes. Shadowing of outer-scope symbols is allowed.
     *
     * If a symbol with the same name already exists in the current scope, this
     * function returns false and leaves the existing symbol unchanged.
     *
     * Scope does not emit diagnostics; the caller is responsible for reporting
     * redeclaration errors.
     *
     * @param name Symbol name to declare (current-scope only)
     * @param symbol The symbol to insert on success
     * @return true if the declaration succeeds, false if the name already exists
     *         in the current scope
     */
    bool declare(const std::string& name, std::unique_ptr<Symbol> symbol)
    {
        if (table.find(name) != table.end())
        {
            return false;
        }
        table.emplace(name, std::move(symbol));
        return true;
    };

    /**
     * @brief Look up a symbol declared directly in the current scope.
     *
     * This function searches only the current scope's symbol table and does
     * not inspect parent scopes.
     *
     * @param name Symbol name to search for
     * @return Pointer to the symbol if found in the current scope, otherwise
     *         nullptr
     */
    const Symbol* lookupLocal(const std::string& name) const
    {
        auto found = table.find(name);
        if (found != table.end())
        {
            return found->second.get();
        }
        return nullptr;
    };

    /**
     * @brief Resolve a symbol using lexical scoping rules.
     *
     * Performs a lexically upward search: current scope, then parent scope,
     * then parent's parent, and so on until the root scope. The search stops
     * at the first match, naturally implementing shadowing.
     *
     * @param name Symbol name to resolve
     * @return Pointer to the resolved symbol if found in any reachable scope,
     *         otherwise nullptr
     */
    const Symbol* lookup(const std::string& name) const
    {

        auto foundLocal = lookupLocal(name);
        if (foundLocal)
        {
            return foundLocal;
        }

        if (parent)
            return parent->lookup(name);
        return nullptr;
    };
};

/**
 * Maps identifier expressions to their corresponding symbol declarations.
 *
 * Built by the Resolver phase and used by the TypeChecker to look up variable
 * types and validate that identifiers reference declared symbols.
 */
struct ResolutionTable
{
    std::unordered_map<const IdentifierExpr*, const Symbol*> mapping;
};

/**
 * Encapsulates the results of semantic analysis (symbol resolution).
 *
 * Contains the scope tree, resolution mappings, and any diagnostic errors
 * encountered during resolution. This data structure bridges resolution and
 * type checking phases.
 */
struct SemanticResult
{
    std::unique_ptr<Scope>  rootScope;
    std::vector<Diagnostic> diagnostics;
    ResolutionTable         resolutionTable;

    bool hadError() const
    {
        return diagnostics.size() > 0;
    };
};

/**
 * Performs symbol resolution and scoping analysis on the Abstract Syntax Tree.
 *
 * The Resolver traverses the AST to:
 * - Build a scope tree representing lexical nesting (blocks, functions, etc.)
 * - Register symbols (variable declarations) in appropriate scopes
 * - Create a symbol resolution table mapping identifier expressions to their declarations
 * - Report redeclaration errors and undefined symbol references
 *
 * This phase runs after parsing and before type checking. It establishes the
 * mapping between identifier uses and their declarations, which is essential
 * for subsequent semantic analysis and type checking.
 */
class Resolver
{
  private:
    Scope*                  currentScope;
    std::unique_ptr<Scope>  rootScope;
    ResolutionTable         resolutionTable;
    std::vector<Diagnostic> diagnostics;

    /** Initiates symbol resolution by processing the program's statements. */
    void resolveProgram(const Program& program);

    /** Dispatches statement resolution to the appropriate statement handler. */
    void resolveStatement(const Stmt& stmt);

    /** Registers variable declarations in the current scope. */
    void resolveSummonStmt(const SummonStmt& stmt);

    /** Resolves print statements and their expressions. */
    void resolveSayStmt(const SayStmt& stmt);

    /** Enters a new block scope and resolves contained statements. */
    void resolveBlockStmt(const BlockStmt& stmt);

    /** Resolves if-chain conditions and blocks. */
    void resolveIfChainStmt(const IfChainStmt& stmt);

    /** Resolves while loop conditions and bodies. */
    void resolveWhileStmt(const WhileStmt& stmt);

    /** Recursively traverses and resolves expressions. */
    void resolveExpression(const Expr& expr);

    /** Maps identifier expressions to their symbol declarations. */
    void resolveIdentifierExpression(const IdentifierExpr& expr);

    /** Creates a new child scope and makes it the current scope. */
    void enterScope();

    /** Restores the parent scope as the current scope. */
    void exitScope();

    /** Records a diagnostic error with location information. */
    void reportError(const std::string& message, SourceLoc loc);

  public:
    /**
     * Constructs a new Resolver instance.
     * Initializes the root scope and internal state for symbol resolution.
     */
    Resolver();
    ~Resolver() = default;

    /**
     * Performs symbol resolution on the given AST.
     *
     * Traverses the entire program, builds the scope tree, registers symbols,
     * and creates the resolution table. Any errors (redeclarations, undefined
     * symbols) are recorded in diagnostics.
     *
     * @param program The parsed Abstract Syntax Tree to resolve
     * @return SemanticResult containing scope tree, resolution table, and diagnostics
     */
    SemanticResult resolve(const Program& program);
};

/**
 * Maps each expression in the AST to its inferred type.
 *
 * The type checker populates this table during type checking. It allows
 * subsequent phases to query the type of any expression node.
 */
struct TypeTable
{
    std::unordered_map<const Expr*, Type> mapping;
};

/**
 * Encapsulates the results of type checking analysis.
 *
 * Contains the type inference table mapping expressions to their types,
 * and any diagnostic errors encountered during type checking (type mismatches,
 * invalid operations, etc.).
 */
struct TypeCheckerResults
{
    TypeTable               typeTable;
    std::vector<Diagnostic> diagnostics;

    bool hadError() const
    {
        return diagnostics.size() > 0;
    };
};
/**
 * Performs type checking and type inference on the Abstract Syntax Tree.
 *
 * The TypeChecker traverses the AST to:
 * - Verify that operations are applied to compatible types
 * - Infer types for expressions
 * - Report type errors (e.g., adding string and integer, non-boolean conditions)
 * - Build a type table mapping expressions to their inferred types
 *
 * This phase runs after symbol resolution and relies on the resolution table
 * to locate symbol declarations. It enforces Ambra's type system rules and
 * ensures type safety throughout the program.
 */
class TypeChecker
{
  private:
    const Scope*            currentScope;
    const ResolutionTable&  resolutionTable;
    TypeTable               typeTable;
    std::vector<Diagnostic> diagnostics;

    /** Tracks expressions currently being type-checked to detect circular dependencies. */
    std::unordered_set<const Expr*> activeDeclarations;

    /** Initiates type checking by processing the program's statements. */
    void checkProgram(const Program& program);

    /** Dispatches statement type checking to the appropriate statement handler. */
    void checkStatement(const Stmt& stmt);

    /** Type checks variable declaration statements. */
    void checkSummonStatement(const SummonStmt& stmt);

    /** Type checks print statements and validates interpolated expressions. */
    void checkSayStatement(const SayStmt& stmt);

    /** Type checks block statements and updates current scope. */
    void checkBlockStatement(const BlockStmt& stmt);

    /** Type checks if-chain conditions (must be boolean) and branches. */
    void checkIfChainStatement(const IfChainStmt& stmt);

    /** Type checks while loop conditions (must be boolean) and bodies. */
    void checkWhileStatement(const WhileStmt& stmt);

    /** Recursively infers and checks expression types. */
    Type checkExpression(const Expr& expr);

    /** Type checks unary operations (negation, logical NOT). */
    Type checkUnaryExpression(const UnaryExpr& expr);

    /** Type checks binary operations (arithmetic, comparison, equality). */
    Type checkBinaryExpression(const BinaryExpr& expr);

    /** Infers type from parenthesized expressions. */
    Type checkGroupingExpression(const GroupingExpr& expr);

    /** Looks up identifier type from the resolution table and symbol information. */
    Type checkIdentifierExpression(const IdentifierExpr& expr);

    /** Infers type from string interpolation expressions. */
    Type checkStringExpression(const StringExpr& expr);

    /** Infers type from literal values (integers, booleans, strings). */
    Type checkLiteralExpression(const Expr& expr);

  public:
    /**
     * Constructs a TypeChecker instance.
     *
     * @param resolutionTable The symbol resolution table from the Resolver phase
     * @param rootScope The root scope from the Resolver phase
     */
    TypeChecker(const ResolutionTable& resolutionTable, const Scope* rootScope);

    /**
     * Performs type checking on the given AST.
     *
     * Traverses the program, infers types for all expressions, verifies type
     * compatibility for all operations, and builds the type table. Any type
     * errors are recorded in diagnostics.
     *
     * @param program The parsed Abstract Syntax Tree to type check
     * @return TypeCheckerResults containing type table and diagnostics
     */
    TypeCheckerResults typeCheck(const Program& program);
};