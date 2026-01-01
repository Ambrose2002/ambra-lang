#include "ast/expr.h"

#include <cstdint>
#include <string>
#include <variant>

enum IrType
{
    I32,
    Bool32,
    String32,
    Void32
};

using LocalId = uint32_t;
using LabelId = uint32_t;
using ConstId = uint32_t;

struct LocalInfo
{
    LocalId     id;
    IrType      type;
    std::string debugName;
    SourceLoc   declLoc;
};

struct Constant
{
    IrType                               type;
    ConstId                              constId;
    std::variant<std::string, int, bool> value;
};