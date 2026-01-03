#include "ast/expr.h"
#include "ast/stmt.h"
#include "program.h"
#include "sema/analyzer.h"

#include <unordered_map>
struct LoweringContext
{
    IrFunction* currentFunction;
    IrProgram*  program;

    std::vector<std::unordered_map<const Symbol*, LocalId>> localScopes;

    const TypeTable&       typeTable;
    const ResolutionTable& resolutionTable;

    bool hadError = false;

    void lowerExpression(const Expr* expr, Type expectedType);
    void lowerIntExpr(const IntLiteralExpr* expr, Type expectedType);
    void lowerStringExpr(const StringExpr* expr, Type expectedType);
    void lowerBoolExpr(const BoolLiteralExpr* expr, Type expectedType);
    void lowerIdentifierExpr(const IdentifierExpr* expr, Type expectedType);
    void lowerUnaryExpr(const UnaryExpr* expr, Type expectedType);
    void lowerBinaryExpr(const BinaryExpr* expr, Type expectedType);
    void lowerGroupingExpr(const GroupingExpr* expr, Type expectedType);

    void lowerStatement(const Stmt* stmt);
    void lowerSummonStatement(const SummonStmt* stmt);
    void lowerSayStatement(const SayStmt* stmt);
    void lowerBlockStatement(const BlockStmt* stmt);
    void lowerIfChainStatement(const IfChainStmt* stmt);
    void lowerWhileStatement(const WhileStmt* stmt);

    IrProgram lowerProgram(const Program* program);
};