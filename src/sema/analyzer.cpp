
#include "sema/analyzer.h"

#include "ast/stmt.h"

#include <memory>
#include <utility>

void Resolver::resolveSummonStmt(const SummonStmt& stmt)
{
    resolveExpression(stmt.getInitializer());

    Symbol symbol;
    symbol.kind = Symbol::VARIABLE;
    symbol.name = stmt.getName();
    symbol.declLoc = stmt.loc;

    bool isDeclared = currentScope->declare(stmt.getName(), std::make_unique<Symbol>(symbol));

    if (!isDeclared)
    {
        reportError("Error declaring variable", stmt.loc);
    }
}

void Resolver::resolveSayStmt(const SayStmt& stmt) {}
void Resolver::resolveBlockStmt(const BlockStmt& stmt)
{
    enterScope();

    for (auto& stmt : stmt)
    {
        resolveStatement(*stmt);
    }

    exitScope();
}

void Resolver::resolveIfChainStmt(const IfChainStmt& stmt)
{
    for (auto& branch : stmt.getBranches())
    {
        auto& condition = std::get<0>(branch);
        resolveExpression(*condition);
        auto& block = std::get<1>(branch);
        resolveBlockStmt(*block);
    }

    auto& elseBranch = stmt.getElseBranch();
    if (elseBranch)
    {
        resolveBlockStmt(**elseBranch);
    }
}
void Resolver::resolveWhileStmt(const WhileStmt& stmt)
{
    auto& condition = stmt.getCondition();
    resolveExpression(condition);
    auto& block = stmt.getBody();
    resolveBlockStmt(block);
}

void Resolver::resolveExpression(const Expr& expr) {
    
};
void Resolver::resolveIdentifierExpr(const Expr& expr) {

};

void Resolver::resolveStatement(const Stmt& stmt)
{

    switch (stmt.kind)
    {
    case Summon:
    {
        auto& statement = static_cast<const SummonStmt&>(stmt);
        resolveSummonStmt(statement);
    }
    case Say:
    {
        auto& statement = static_cast<const SayStmt&>(stmt);
        resolveSayStmt(statement);
    }
    case Block:
    {
        auto& statement = static_cast<const BlockStmt&>(stmt);
        resolveBlockStmt(statement);
    }
    case IfChain:
    {
        auto& statement = static_cast<const IfChainStmt&>(stmt);
        resolveIfChainStmt(statement);
    }
    case While:
    {
        auto& statement = static_cast<const WhileStmt&>(stmt);
        resolveWhileStmt(statement);
    }
    default:
        return;
    }
}

void Resolver::resolveProgram(const Program& program)
{
    for (auto& stmt : program)
    {
        resolveStatement(*stmt);
    }
}

SemanticResult Resolver::resolve(const Program& program)
{
    std::unique_ptr<Scope> rootScope = std::make_unique<Scope>();

    currentScope = rootScope.get();

    diagnostics.clear();

    resolutionTable.mapping.clear();

    resolveProgram(program);

    SemanticResult result;
    result.rootScope = std::move(rootScope);
    result.diagnostics = diagnostics;
    result.resolutionTable = resolutionTable;
    return result;
}