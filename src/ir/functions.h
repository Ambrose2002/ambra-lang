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
    LocalTable               localTable;
    LabelTable               labelTable;

    LocalId nextLocalId{0};
    LabelId nextLabelId{0};
};