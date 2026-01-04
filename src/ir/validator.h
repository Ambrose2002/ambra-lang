#include "lowering.h"
#include <vector>

struct IrDiagnostic {
    std::string message;
    int instructionIndex;
    SourceLoc loc;
};

struct IrValidatorResults
{
    std::vector<IrDiagnostic> diagnostics;

    bool hadError() const {
        return diagnostics.size() > 0;
    }
};

struct IrValidator
{
    const IrProgram&  program;
    const IrFunction& function;

    std::vector<IrDiagnostic> diagnostics;

    IrValidatorResults validate() {
        diagnostics.clear();
        validateFunction();

        return IrValidatorResults{diagnostics};
    };

    private: 
    void validateFunction();
    void validateInstruction(Instruction instruction, int ip);
    void validateStack();
    void validateControlFlow();
};