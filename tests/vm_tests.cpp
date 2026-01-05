#include "parser/parser.h"
#include "sema/analyzer.h"
#include "vm/vm.h"

#include <gtest/gtest.h>
#include <sstream>
#include <string>

/*
 * Helper: full pipeline + VM execution with captured stdout
 */
static std::string runProgram(const std::string& source)
{
    // Parse
    Lexer              lexer(source);
    std::vector<Token> tokens = lexer.scanTokens();
    Parser             parser(tokens);
    Program            program = parser.parseProgram();
    EXPECT_FALSE(program.hadError());

    // Resolve
    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    EXPECT_FALSE(sema.hadError());

    // Typecheck
    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);
    EXPECT_FALSE(types.hadError());

    // Lower
    LoweringContext lowerer{nullptr, nullptr, {}, types.typeTable, sema.resolutionTable};

    IrProgram ir = lowerer.lowerProgram(&program);
    EXPECT_FALSE(lowerer.hadError);

    // Validate IR
    IrValidator        validator{ir, ir.main};
    IrValidatorResults vres = validator.validate();
    EXPECT_FALSE(vres.hadError());

    // Capture stdout
    std::stringstream buffer;
    std::streambuf*   old = std::cout.rdbuf(buffer.rdbuf());

    // Execute
    VM vm{ir, ir.main};
    vm.execute();

    // Restore stdout
    std::cout.rdbuf(old);

    return buffer.str();
}


TEST(VM_Basics, PrintInt)
{
    std::string out = runProgram("say 5;");
    EXPECT_EQ(out, "5");
}

TEST(VM_Basics, PrintBool)
{
    std::string out = runProgram("say affirmative;");
    EXPECT_EQ(out, "affirmative");
}

TEST(VM_Basics, PrintString)
{
    std::string out = runProgram(R"(say "hello";)");
    EXPECT_EQ(out, "hello");
}