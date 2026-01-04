#include "ir/validator.h"

#include <gtest/gtest.h>

/** Helper to run validation on a function within a program. */
static IrValidatorResults validate(const IrFunction& fn, IrProgram& pg)
{

    pg.main = fn;

    IrValidator validator{pg, pg.main};
    return validator.validate();
}

TEST(IRValidator_Stack, DetectsUnderflow)
{
    IrFunction fn;
    fn.instructions.push_back({AddI32, {}}); // needs 2 operands

    IrProgram pg;
    auto      res = validate(fn, pg);
    EXPECT_TRUE(res.hadError());
}

TEST(IRValidator_Types, DetectsTypeMismatch)
{
    IrFunction fn;

    fn.instructions.push_back({PushConst, ConstId{0}});
    fn.instructions.push_back({PushConst, ConstId{1}});
    fn.instructions.push_back({AddI32, {}});

    IrProgram pg;

    // constants: string + string
    pg.constants.push_back({String32, 0, std::string("a")});
    pg.constants.push_back({String32, 1, std::string("b")});

    auto res = validate(fn, pg);
    EXPECT_TRUE(res.hadError());
}

TEST(IRValidator_ControlFlow, JumpToUndefinedLabel)
{
    IrFunction fn;
    IrProgram  pg;

    fn.instructions.push_back({Jump, LabelId{42}});

    auto res = validate(fn, pg);
    EXPECT_TRUE(res.hadError());
}

/** Jump operand must be a label id. */
TEST(IRValidator_ControlFlow, JumpMissingLabelOperand)
{
    IrFunction fn;
    IrProgram  pg;

    fn.instructions.push_back({Jump, ConstId{0}});

    auto res = validate(fn, pg);
    EXPECT_TRUE(res.hadError());
}

TEST(IRValidator_ControlFlow, StackMismatchAtMerge)
{
    IrFunction fn;
    IrProgram  pg;

    LabelId elseLabel{1};
    LabelId endLabel{2};

    fn.instructions.push_back({PushConst, ConstId{0}}); // int
    fn.instructions.push_back({JumpIfFalse, elseLabel});

    fn.instructions.push_back({PushConst, ConstId{1}}); // int
    fn.instructions.push_back({Jump, endLabel});

    fn.instructions.push_back({JLabel, elseLabel});
    fn.instructions.push_back({PushConst, ConstId{2}}); // string

    fn.instructions.push_back({JLabel, endLabel});

    pg.constants.push_back({I32, 0, 1});
    pg.constants.push_back({I32, 1, 2});
    pg.constants.push_back({String32, 2, std::string("x")});

    fn.labelTable.position[elseLabel] = 4;
    fn.labelTable.position[endLabel] = 6;

    auto res = validate(fn, pg);
    EXPECT_TRUE(res.hadError());
}

/** Label table entry must point to a JLabel instruction. */
TEST(IRValidator_ControlFlow, LabelTableNonLabelInstruction)
{
    IrFunction fn;
    IrProgram  pg;

    LabelId badLabel{7};

    fn.instructions.push_back({Jump, badLabel});
    fn.instructions.push_back({Nop, {}});

    fn.labelTable.position[badLabel] = 1; // points at Nop instead of JLabel

    auto res = validate(fn, pg);
    EXPECT_TRUE(res.hadError());
}

/** Label operand must match the label table id. */
TEST(IRValidator_ControlFlow, LabelTableIdMismatch)
{
    IrFunction fn;
    IrProgram  pg;

    LabelId tableId{3};
    LabelId instrId{4};

    fn.instructions.push_back({JLabel, instrId});

    fn.labelTable.position[tableId] = 0; // different id than operand

    auto res = validate(fn, pg);
    EXPECT_TRUE(res.hadError());
}

/** Jump target outside instruction bounds is rejected. */
TEST(IRValidator_ControlFlow, JumpTargetOutOfRange)
{
    IrFunction fn;
    IrProgram  pg;

    LabelId target{1};

    fn.instructions.push_back({Jump, target});

    fn.labelTable.position[target] = 10; // beyond instr size

    auto res = validate(fn, pg);
    EXPECT_TRUE(res.hadError());
}

TEST(IRValidator_Valid, SimpleValidProgram)
{
    IrFunction fn;
    IrProgram  pg;

    fn.instructions.push_back({PushConst, ConstId{0}});
    fn.instructions.push_back({ToString, {}});
    fn.instructions.push_back({PrintString, {}});

    pg.constants.push_back({I32, 0, 42});

    auto res = validate(fn, pg);
    EXPECT_FALSE(res.hadError());
}

/** Branches merge with balanced stacks and valid labels. */
TEST(IRValidator_Valid, BranchMergeBalancedStack)
{
    IrFunction fn;
    IrProgram  pg;

    LabelId elseLabel{1};
    LabelId endLabel{2};

    fn.instructions.push_back({PushConst, ConstId{0}}); // bool
    fn.instructions.push_back({JumpIfFalse, elseLabel});

    fn.instructions.push_back({PushConst, ConstId{1}}); // int
    fn.instructions.push_back({Jump, endLabel});

    fn.instructions.push_back({JLabel, elseLabel});
    fn.instructions.push_back({PushConst, ConstId{2}}); // int

    fn.instructions.push_back({JLabel, endLabel});
    fn.instructions.push_back({ToString, {}});
    fn.instructions.push_back({PrintString, {}});

    pg.constants.push_back({Bool32, 0, true});
    pg.constants.push_back({I32, 1, 7});
    pg.constants.push_back({I32, 2, 9});

    fn.labelTable.position[elseLabel] = 4;
    fn.labelTable.position[endLabel] = 6;

    auto res = validate(fn, pg);
    EXPECT_FALSE(res.hadError());
}

/** Local store/load round-trip with printing. */
TEST(IRValidator_Valid, LocalStoreLoadRoundTrip)
{
    IrFunction fn;
    IrProgram  pg;

    fn.instructions.push_back({PushConst, ConstId{0}});
    fn.instructions.push_back({StoreLocal, LocalId{0}});
    fn.instructions.push_back({LoadLocal, LocalId{0}});
    fn.instructions.push_back({ToString, {}});
    fn.instructions.push_back({PrintString, {}});

    pg.constants.push_back({I32, 0, 123});
    fn.localTable.locals.push_back(LocalInfo{LocalId{0}, I32, "x", {}});

    auto res = validate(fn, pg);
    EXPECT_FALSE(res.hadError());
}

/** String concat path that stays type-correct. */
TEST(IRValidator_Valid, ConcatAndPrint)
{
    IrFunction fn;
    IrProgram  pg;

    fn.instructions.push_back({PushConst, ConstId{0}});
    fn.instructions.push_back({PushConst, ConstId{1}});
    fn.instructions.push_back({ConcatString, {}});
    fn.instructions.push_back({PrintString, {}});

    pg.constants.push_back({String32, 0, std::string("hello ")});
    pg.constants.push_back({String32, 1, std::string("world")});

    auto res = validate(fn, pg);
    EXPECT_FALSE(res.hadError());
}

/** Detects invalid const ids. */
TEST(IRValidator_Types, InvalidConstId)
{
    IrFunction fn;
    IrProgram  pg;

    fn.instructions.push_back({PushConst, ConstId{5}});

    auto res = validate(fn, pg);
    EXPECT_TRUE(res.hadError());
}

/** LoadLocal rejects out-of-range locals. */
TEST(IRValidator_Types, InvalidLocalLoad)
{
    IrFunction fn;
    IrProgram  pg;

    fn.instructions.push_back({LoadLocal, LocalId{0}});

    auto res = validate(fn, pg);
    EXPECT_TRUE(res.hadError());
}

/** StoreLocal enforces type matching. */
TEST(IRValidator_Types, StoreLocalTypeMismatch)
{
    IrFunction fn;
    IrProgram  pg;

    fn.instructions.push_back({PushConst, ConstId{0}}); // bool
    fn.instructions.push_back({StoreLocal, LocalId{0}});

    pg.constants.push_back({Bool32, 0, true});
    fn.localTable.locals.push_back(LocalInfo{0, I32});

    auto res = validate(fn, pg);
    EXPECT_TRUE(res.hadError());
}

/** ToString requires an operand on the stack. */
TEST(IRValidator_Types, ToStringUnderflow)
{
    IrFunction fn;
    IrProgram  pg;

    fn.instructions.push_back({ToString, {}});

    auto res = validate(fn, pg);
    EXPECT_TRUE(res.hadError());
}