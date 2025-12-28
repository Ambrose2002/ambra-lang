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
#include <vector>

struct Scope;
struct Diagnostic
{
    std::string message;
    SourceLoc   loc;

    std::string toString() const;
};

struct Symbol
{
    std::string name;
    enum Kind
    {
        VARIABLE
    } kind;

    SourceLoc declLoc;
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

struct ResolutionTable
{
    std::unordered_map<IdentifierExpr*, Symbol*> mapping;
};

struct SemanticResult
{
    std::unique_ptr<Scope>  rootScope;
    std::vector<Diagnostic> diagnostic;
    ResolutionTable         resolutionTable;
    bool                    hadError() const
    {
        return diagnostic.size() > 0;
    };
};

class Resolver
{
  private:
    ~Resolver() = default;

    Scope*                  currentScope;
    std::unique_ptr<Scope>  rootScope;
    ResolutionTable         resolutionTable;
    std::vector<Diagnostic> diagnostic;

    void resolveProgram(Program program);

    void resolveStatement(Stmt stmt);

    void resolveExpression(Expr expr);

    void enterScope();

    void exitScope();

    void resolveSummonStmt(Stmt stmt);

    void resolveSayStmt(Stmt stmt);

    void resolveBlockStmt(Stmt stmt);

    void resolveIfChainStmt(Stmt stmt);

    void resolveWhileStmt(Stmt stmt);

    void resolveIdentifierExpr(Expr expr);

  public:
    SemanticResult resolve();
};