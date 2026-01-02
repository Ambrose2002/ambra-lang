#include "instructions.h"

#include <vector>

struct LocalTable
{
    std::vector<LocalInfo> locals;
};

struct LabelTable
{
    std::vector<Label> labels;
};

struct IrFunction
{
    std::vector<Instruction> instructions;
    LocalTable               locals;
    LabelTable               labels;

    LocalId nextLocalId{0};
    LabelId nextLabelId{0};
};