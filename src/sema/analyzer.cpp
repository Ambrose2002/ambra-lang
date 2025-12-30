
#include "sema/analyzer.h"

#include "ast/expr.h"
#include "ast/stmt.h"

#include <cassert>
#include <memory>
#include <utility>

Resolver::Resolver() : currentScope(nullptr), rootScope(nullptr) {}

void Resolver::reportError(const std::string& message, SourceLoc loc)
{
    diagnostics.emplace_back(Diagnostic{message, loc});
}

void Resolver::enterScope()
{
    auto newScope = std::make_unique<Scope>();
    newScope->parent = currentScope;
    Scope* scopePtr = newScope.get();
    currentScope->children.push_back(std::move(newScope));

    currentScope = scopePtr;
}

void Resolver::exitScope()
{
    assert(currentScope && "exitScope called with null currentScope");
    assert(currentScope->parent && "attempted to exit root scope");
    currentScope = currentScope->parent;
}

void Resolver::resolveSummonStmt(const SummonStmt& stmt)
{
    resolveExpression(stmt.getInitializer());
    auto identifier = stmt.getIdentifier();

    auto symbol = std::make_unique<Symbol>();
    symbol->kind = Symbol::VARIABLE;
    symbol->name = identifier.getName();
    symbol->declLoc = identifier.loc;

    bool isDeclared = currentScope->declare(identifier.getName(), std::move(symbol));

    if (!isDeclared)
    {
        reportError("Redeclaration of variable " + identifier.getName() + " in the same scope",
                    stmt.loc);
    }
}

void Resolver::resolveSayStmt(const SayStmt& stmt)
{
    auto& expression = stmt.getExpression();
    resolveExpression(expression);
}
void Resolver::resolveBlockStmt(const BlockStmt& stmt)
{
    enterScope();

    for (auto& statement : stmt)
    {
        resolveStatement(*statement);
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
        resolveBlockStmt(*elseBranch);
    }
}
void Resolver::resolveWhileStmt(const WhileStmt& stmt)
{
    auto& condition = stmt.getCondition();
    resolveExpression(condition);
    auto& block = stmt.getBody();
    resolveBlockStmt(block);
}

void Resolver::resolveExpression(const Expr& expr)
{

    switch (expr.kind)
    {
    case Identifier:
    {
        auto& identiferExpression = static_cast<const IdentifierExpr&>(expr);
        resolveIdentifierExpression(identiferExpression);
        return;
    }
    case Unary:
    {
        auto& unaryExpression = static_cast<const UnaryExpr&>(expr);
        resolveExpression(unaryExpression.getOperand());
        return;
    }
    case Binary:
    {
        auto& binaryExpression = static_cast<const BinaryExpr&>(expr);
        resolveExpression(binaryExpression.getLeft());
        resolveExpression(binaryExpression.getRight());
        return;
    }
    case Grouping:
    {
        auto& groupingExpression = static_cast<const GroupingExpr&>(expr);
        resolveExpression(groupingExpression.getExpression());
        return;
    }
    case InterpolatedString:
    {
        auto& stringExpression = static_cast<const StringExpr&>(expr);

        for (auto& part : stringExpression.getParts())
        {
            if (part.kind == StringPart::EXPR)
            {
                resolveExpression(*part.expr);
            }
        }
        return;
    }
    case IntLiteral:
    case BoolLiteral:
    default:
    {
        return;
    }
    }
};
void Resolver::resolveIdentifierExpression(const IdentifierExpr& expr)
{

    std::string name = expr.getName();

    const Symbol* symbol = currentScope->lookup(name);
    if (symbol)
    {
        resolutionTable.mapping.emplace(&expr, symbol);
        return;
    }

    reportError("undeclared identifier", expr.loc);
};

void Resolver::resolveStatement(const Stmt& stmt)
{

    switch (stmt.kind)
    {
    case Summon:
    {
        auto& statement = static_cast<const SummonStmt&>(stmt);
        resolveSummonStmt(statement);
        return;
    }
    case Say:
    {
        auto& statement = static_cast<const SayStmt&>(stmt);
        resolveSayStmt(statement);
        return;
    }
    case Block:
    {
        auto& statement = static_cast<const BlockStmt&>(stmt);
        resolveBlockStmt(statement);
        return;
    }
    case IfChain:
    {
        auto& statement = static_cast<const IfChainStmt&>(stmt);
        resolveIfChainStmt(statement);
        return;
    }
    case While:
    {
        auto& statement = static_cast<const WhileStmt&>(stmt);
        resolveWhileStmt(statement);
        return;
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
    rootScope = std::make_unique<Scope>();
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

TypeChecker::TypeChecker(const ResolutionTable& resolutionTable, const Scope* rootScope)
    : resolutionTable(resolutionTable)
{
}

void TypeChecker::typeCheck(const Program& program)
{
    diagnostics.clear();
    typeTable.mapping.clear();

    checkProgram(program);
};

void TypeChecker::checkProgram(const Program& program)
{
    for (auto& stmt : program.getStatements())
    {
        checkStatement(*stmt);
    }
};
void TypeChecker::checkStatement(const Stmt& stmt)
{
    switch (stmt.kind)
    {
    case Summon:
    {
        auto& s = static_cast<const SummonStmt&>(stmt);
        checkSummonSatement(s);
        return;
    }
    case Say:
    {
        auto& s = static_cast<const SayStmt&>(stmt);
        checkSayStatement(s);
        return;
    }
    case Block:
    {
        auto& s = static_cast<const BlockStmt&>(stmt);
        checkBlockStatement(s);
        return;
    }
    case IfChain:
    {
        auto& s = static_cast<const IfChainStmt&>(stmt);
        checkIfChainStatement(s);
        return;
    }
    case While:
    {
        auto& s = static_cast<const WhileStmt&>(stmt);
        checkWhileStatement(s);
        return;
    }
    default:
        return;
    }
};
void TypeChecker::checkSummonSatement(const SummonStmt& stmt) {

};
void TypeChecker::checkSayStatement(const SayStmt& stmt) {

};
void TypeChecker::checkBlockStatement(const BlockStmt& stmt) {

};
void TypeChecker::checkIfChainStatement(const IfChainStmt& stmt) {};
void checkWhileStatement(const WhileStmt& stmt);

Type TypeChecker::checkExpression(const Expr& expr) {};
Type TypeChecker::checkUnaryExpression(const UnaryExpr& expr) {};
Type TypeChecker::checkBinaryExpression(const BinaryExpr& expr) {};
Type TypeChecker::checkGroupingExpression(const GroupingExpr& expr) {};
Type TypeChecker::checkIdentifierExpression(const IdentifierExpr& expr) {};
Type TypeChecker::checkStringExpression(const StringExpr& expr) {};

Type TypeChecker::checkLiteralExpression(const Expr& expr)
{
    if (expr.kind == IntLiteral)
    {
        return Int;
    }
    else if (expr.kind == BoolLiteral)
    {
        return Bool;
    }
    else
    {
        return Error;
    }
};