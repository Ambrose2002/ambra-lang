#include "lowering.h"

#include <unordered_map>
#include <vector>

struct IrDiagnostic
{
    std::string message;
    size_t         ip;
    // SourceLoc   loc;
};

struct IrValidatorResults
{
    std::vector<IrDiagnostic> diagnostics;

    bool hadError() const
    {
        return diagnostics.size() > 0;
    }
};

struct IrValidator
{
    const IrProgram&  program;
    const IrFunction& function;

    std::vector<IrDiagnostic> diagnostics;

    IrValidatorResults validate()
    {
        diagnostics.clear();
        validateFunction();

        return IrValidatorResults{diagnostics};
    };

  private:
    void validateFunction()
    {
        std::vector<IrType> stack;

        std::unordered_map<LabelId, std::vector<IrType>> labelStacks;

        const auto& instructions = function.instructions;

        for (size_t ip = 0; ip < instructions.size(); ++ip)
        {
            const Instruction& inst = instructions[ip];

            // Validate instruction + update stack
            validateInstruction(ip, stack);

            // Handle control-flow edges
            if (inst.opcode == Jump || inst.opcode == JumpIfFalse)
            {
                LabelId target = std::get<LabelId>(inst.operand);

                auto it = labelStacks.find(target);
                if (it == labelStacks.end())
                {
                    labelStacks[target] = stack;
                }
                else if (it->second != stack)
                {
                    diagnostics.push_back({"Stack mismatch at jump target", ip});
                }
            }

            // Handle label definition
            if (inst.opcode == JLabel)
            {
                LabelId id = std::get<LabelId>(inst.operand);

                auto it = labelStacks.find(id);
                if (it == labelStacks.end())
                {
                    labelStacks[id] = stack;
                }
                else if (it->second != stack)
                {
                    diagnostics.push_back({"Stack mismatch at label", ip});
                }
            }
        }
    }

    void validateInstruction(size_t ip, std::vector<IrType> stack);
    void validateStack();
    void validateControlFlow();
};