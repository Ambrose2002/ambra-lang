
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
    symbol->declStmt = &stmt;

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

TypeCheckerResults TypeChecker::typeCheck(const Program& program)
{
    diagnostics.clear();
    typeTable.mapping.clear();

    checkProgram(program);

    return TypeCheckerResults{typeTable, diagnostics};
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
        checkSummonStatement(s);
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
void TypeChecker::checkSummonStatement(const SummonStmt& stmt)
{
    auto& initializer = stmt.getInitializer();
    Type  type = checkExpression(initializer);

    if (type == Void)
    {
        diagnostics.emplace_back(
            Diagnostic{"Variable initializer cannot be void", initializer.loc});
    }
};
void TypeChecker::checkSayStatement(const SayStmt& stmt)
{
    auto& expr = stmt.getExpression();
    checkExpression(expr);
};
void TypeChecker::checkBlockStatement(const BlockStmt& stmt)
{

    for (auto& s : stmt)
    {
        checkStatement(*s);
    }
};
void TypeChecker::checkIfChainStatement(const IfChainStmt& stmt)
{

    for (auto& branch : stmt.getBranches())
    {
        auto& condition = *std::get<0>(branch);
        Type  t = checkExpression(condition);
        if (t != Bool && t != Error)
        {
            diagnostics.emplace_back(
                Diagnostic{"If condition expression must be Bool", condition.loc});
        }
        auto& block = std::get<1>(branch);
        checkBlockStatement(*block);
    }

    auto& elseBlock = stmt.getElseBranch();
    if (elseBlock)
    {
        checkBlockStatement(*elseBlock);
    }
};
void TypeChecker::checkWhileStatement(const WhileStmt& stmt)
{
    auto& condition = stmt.getCondition();
    Type  t = checkExpression(condition);

    if (t != Bool && t != Error)
    {
        diagnostics.emplace_back(
            Diagnostic{"While condition expression must be Bool", condition.loc});
    }

    auto& block = stmt.getBody();
    checkBlockStatement(block);
};

Type TypeChecker::checkExpression(const Expr& expr)
{
    Type t;
    switch (expr.kind)
    {
    case IntLiteral:
    case BoolLiteral:
    {
        t = checkLiteralExpression(expr);
        break;
    }
    case Identifier:
    {
        auto& ex = static_cast<const IdentifierExpr&>(expr);
        t = checkIdentifierExpression(ex);
        break;
    }
    case Unary:
    {
        auto& ex = static_cast<const UnaryExpr&>(expr);
        t = checkUnaryExpression(ex);
        break;
    }
    case Binary:
    {
        auto& ex = static_cast<const BinaryExpr&>(expr);
        t = checkBinaryExpression(ex);
        break;
    }
    case Grouping:
    {
        auto& ex = static_cast<const GroupingExpr&>(expr);
        t = checkGroupingExpression(ex);
        break;
    }
    case InterpolatedString:
    {
        auto& ex = static_cast<const StringExpr&>(expr);
        t = checkStringExpression(ex);
        break;
    }
    default:
        t = Error;
        break;
    }
    typeTable.mapping.emplace(&expr, t);
    return t;
};
Type TypeChecker::checkUnaryExpression(const UnaryExpr& expr)
{
    auto  op = expr.getOperator();
    auto& operand = expr.getOperand();
    Type  t = checkExpression(operand);

    if (t == Error)
    {
        return t;
    }

    if (op == LogicalNot)
    {

        if (t == Bool)
        {
            return Bool;
        }
        diagnostics.emplace_back(Diagnostic{"Unary 'not' expects a Bool operand", expr.loc});
        return Error;
    }

    if (op == ArithmeticNegate)
    {

        if (t == Int)
        {
            return Int;
        }
        diagnostics.emplace_back(Diagnostic{"Unary '-' expects an Int operand", expr.loc});
        return Error;
    }
    return Error;
};
Type TypeChecker::checkBinaryExpression(const BinaryExpr& expr)
{
    auto  op = expr.getOperator();
    auto& left = expr.getLeft();
    auto& right = expr.getRight();
    Type  leftType = checkExpression(left);
    Type  rightType = checkExpression(right);

    if (leftType == Error || rightType == Error)
    {
        return Error;
    }

    switch (op)
    {
    case Add:
    case Divide:
    case Multiply:
    case Subtract:
    {
        if (leftType != Int || rightType != Int)
        {
            diagnostics.emplace_back(
                Diagnostic{"Arithmetic operators require Int operands", expr.loc});
            return Error;
        }
        return Int;
    }
    case Less:
    case Greater:
    case LessEqual:
    case GreaterEqual:
    {
        if (leftType != Int || rightType != Int)
        {
            diagnostics.emplace_back(
                Diagnostic{"Comparison operators require Int operands", expr.loc});
            return Error;
        }
        return Bool;
    }
    case NotEqual:
    case EqualEqual:
    {
        if (leftType != rightType)
        {
            diagnostics.emplace_back(
                Diagnostic{"Equality operands must have the same type", expr.loc});
            return Error;
        }
        return Bool;
    }
    default:
        return Error;
    }
};

Type TypeChecker::checkGroupingExpression(const GroupingExpr& expr)
{
    auto& e = expr.getExpression();
    return checkExpression(e);
};

Type TypeChecker::checkIdentifierExpression(const IdentifierExpr& expr)
{
    auto it = resolutionTable.mapping.find(&expr);
    if (it == resolutionTable.mapping.end())
    {
        diagnostics.emplace_back(
            Diagnostic{"Unresolved identifier '" + expr.getName() + "'", expr.loc});
        return Error;
    }
    const Symbol* symbol = it->second;

    const SummonStmt* decl = symbol->declStmt;
    if (!decl)
    {
        diagnostics.emplace_back(
            Diagnostic{"Internal error: missing declaration for identifier", expr.loc});
        return Error;
    }

    auto& initializer = decl->getInitializer();
    auto  iterator = typeTable.mapping.find(&initializer);
    if (iterator != typeTable.mapping.end())
    {
        return iterator->second;
    }

    if (activeDeclarations.find(&initializer) != activeDeclarations.end())
    {
        diagnostics.emplace_back(Diagnostic{"Circular type dependency detected", initializer.loc});
        return Error;
    }
    activeDeclarations.emplace(&initializer);
    Type t = checkExpression(initializer);
    activeDeclarations.erase(&initializer);
    return t;
};

Type TypeChecker::checkStringExpression(const StringExpr& expr)
{
    for (auto& part : expr.getParts())
    {
        if (part.kind == StringPart::EXPR)
        {
            Type t = checkExpression(*part.expr);

            if (t == Error)
                return Error;

            if (t == Void)
            {
                diagnostics.emplace_back(Diagnostic{
                    "Void expression cannot appear in string interpolation", part.expr->loc});
                return Error;
            }
        }
    }
    return String;
};

Type TypeChecker::checkLiteralExpression(const Expr& expr)
{
    if (expr.kind == IntLiteral)
    {
        return Int;
    }
    return Bool;
};