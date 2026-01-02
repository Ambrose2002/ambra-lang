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

struct LocalId
{
    uint32_t value;
};

struct LabelId
{
    uint32_t value;
};

struct ConstId
{
    uint32_t value;
};

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