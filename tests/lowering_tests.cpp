#include "ir/lowering.h"
#include "parser/parser.h"
#include "sema/analyzer.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>

static IrProgram lowerFromSource(const std::string& source)
{
    Lexer              lexer(source);
    std::vector<Token> tokenList = lexer.scanTokens();

    Parser  parser(tokenList);
    Program program = parser.parseProgram();
    EXPECT_FALSE(program.hadError());

    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    EXPECT_FALSE(sema.hadError());

    TypeChecker        tc(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = tc.typeCheck(program);
    EXPECT_FALSE(types.hadError());

    LoweringContext lowerer{/* currentFunction = */ nullptr,
                            /* program = */ nullptr,
                            /* localScopes = */ {},
                            /* typeTable = */ types.typeTable,
                            /* resolutionTable = */ sema.resolutionTable};

    return lowerer.lowerProgram(&program);
}