#include "ast/expr.h"
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

    void lowerExpression(const Expr* expr);
    void lowerStatement(const Stmt* stmt);
    void lowerIntExpr(const IntLiteralExpr* expr);
    void lowerStringExpr(const StringExpr* expr);
    void lowerBoolExpr(const BoolLiteralExpr* expr);
    void lowerIdentifierExpr(const IdentifierExpr* expr);
    void lowerUnaryExpr(const UnaryExpr* expr);
    void lowerBinaryExpr(const BinaryExpr* expr);
    void lowerGroupingExpr(const GroupingExpr* expr);
};