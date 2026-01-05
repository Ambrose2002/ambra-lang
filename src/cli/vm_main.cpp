#include "parser/parser.h"
#include "sema/analyzer.h"
#include "vm/vm.h"

#include <fstream>
#include <iostream>
#include <sstream>

/*
 * Run full compiler pipeline and execute program.
 * Returns execution output if successful.
 */
std::string runProgram(const std::string& source)
{
    // Lex the source code
    Lexer              lexer(source);
    std::vector<Token> tokens = lexer.scanTokens();

    // Parse tokens into ast
    Parser  parser(tokens);
    Program program = parser.parseProgram();
    if (parser.hadError())
        throw std::runtime_error("Parsing failed");

    // Resolve program
    Resolver       resolver;
    SemanticResult sema = resolver.resolve(program);
    if (sema.hadError())
    {
        for (auto& e : sema.diagnostics)
        {
            std::cerr << e.toString() << "\n";
        }
        throw std::runtime_error("Semantic analysis failed");
    }

    // Typecheck program
    TypeChecker        checker(sema.resolutionTable, sema.rootScope.get());
    TypeCheckerResults types = checker.typeCheck(program);
    if (types.hadError())
    {
        for (const auto& e : types.diagnostics)
        {
            std::cerr << e.toString() << "\n";
        }
        throw std::runtime_error("Typechecking failed");
    }

    LoweringContext lowerer{nullptr, nullptr, {}, types.typeTable, sema.resolutionTable};

    IrProgram ir = lowerer.lowerProgram(&program);

    // validate IR
    IrValidator        validator{ir, ir.main};
    IrValidatorResults vres = validator.validate();
    if (vres.hadError())
    {
        for (const auto& e : vres.diagnostics)
        {
            std::cerr << e.message << "\n";
        }
        throw std::runtime_error("IR validation failed");
    }

    // ---- Execute VM (capture stdout) ----
    std::stringstream buffer;
    std::streambuf*   oldOut = std::cout.rdbuf(buffer.rdbuf());

    VM vm{ir, ir.main};

    vm.execute();

    std::cout.rdbuf(oldOut);
    return buffer.str();
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: ambra_lang <file.ara>\n";
        return 1;
    }

    std::string fileName = argv[1];
    if (fileName.size() < 4 || fileName.substr(fileName.size() - 4) != ".ara")
    {
        std::cerr << "Error: source file must have .ara extension\n";
        return 1;
    }

    std::ifstream input(fileName);
    if (!input)
    {
        std::cerr << "Error: could not open file\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << input.rdbuf();
    std::string source = buffer.str();

    try
    {
        std::string output = runProgram(source);
        std::cout << output;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Compilation failed: " << e.what() << "\n";
        return 1;
    }

    return 0;
}