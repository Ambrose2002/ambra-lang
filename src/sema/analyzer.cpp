
#include "sema/analyzer.h"

#include "ast/stmt.h"

#include <memory>
#include <utility>

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