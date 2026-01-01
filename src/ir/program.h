#include "functions.h"

#include <vector>
struct IrProgram
{
    std::vector<Constant> constants;
    IrFunction            main;
};