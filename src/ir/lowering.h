#include "program.h"
#include "sema/analyzer.h"

#include <stack>
#include <unordered_map>
struct LoweringContext
{
    IrFunction* currentFunction;
    IrProgram*  program;

    std::stack<std::unordered_map<const Symbol*, LocalId>> localScopes;

    const TypeTable&       typeTable;
    const ResolutionTable& resolutionTable;

    bool hadError = false;
};