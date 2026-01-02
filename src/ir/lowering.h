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
};