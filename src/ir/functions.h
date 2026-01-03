/**
 * @file functions.h
 * @brief IR function structure and associated tables
 *
 * This file defines the IrFunction structure which represents a single
 * function in the IR, along with supporting structures for managing
 * local variables and labels.
 *
 * Key structures:
 * - LocalTable: Tracks all local variables in a function
 * - LabelTable: Tracks all labels for control flow
 * - IrFunction: Complete function including instructions and metadata
 *
 * In the current version, only one function (main) is used. Future versions
 * will support user-defined functions, which will also use this structure.
 *
 * Functions are self-contained units with:
 * - Their own instruction sequence
 * - Private local variable space
 * - Independent label namespace
 * - ID generators for locals and labels
 */

#include "instructions.h"

#include <vector>

/**
 * @brief Table of local variables in a function
 *
 * Stores metadata for all local variables declared within a function.
 * Each local has a unique LocalId and associated type/name information.
 *
 * The table is built during the lowering phase as variables are declared.
 * It's used for:
 * - Type checking during lowering
 * - Debugging (variable names)
 * - Stack frame allocation in the VM
 */
struct LocalTable
{
    /**
     * @brief Vector of local variable metadata
     *
     * Index corresponds to LocalId.value. For example, LocalId{0} refers
     * to locals[0], LocalId{1} to locals[1], etc.
     */
    std::vector<LocalInfo> locals;
};

/**
 * @brief Table of control flow labels in a function
 *
 * Stores metadata for all labels used in control flow instructions.
 * Labels mark positions in the instruction stream that can be jumped to.
 *
 * The table is built during the lowering phase as labels are generated
 * for control structures (if/else, loops).
 */
struct LabelTable
{
    /**
     * @brief Vector of label metadata
     *
     * Each label has a unique LabelId and optional debug name.
     * Labels are resolved to instruction positions during execution.
     */
    std::vector<Label> labels;
};

/**
 * @brief IR representation of a function
 *
 * Contains everything needed to execute a function:
 * - Instruction sequence (the actual code)
 * - Local variable table (function's variables)
 * - Label table (control flow targets)
 * - ID generators for allocating new locals/labels
 *
 * The function is the unit of execution in the VM. The main function
 * represents the top-level program code.
 *
 * Execution model:
 * 1. Allocate stack frame based on local table size
 * 2. Execute instructions sequentially
 * 3. Jump instructions can change instruction pointer
 * 4. Function completes when all instructions are executed
 */
struct IrFunction
{
    /**
     * @brief Sequence of instructions to execute
     *
     * Instructions are executed in order from index 0 to n-1.
     * Control flow instructions (Jump, JumpIfFalse) can change the
     * execution order by modifying the instruction pointer.
     */
    std::vector<Instruction> instructions;

    /**
     * @brief Local variable metadata table
     *
     * Contains information about all variables declared in this function.
     * Used to allocate stack frame and for debugging.
     */
    LocalTable localTable;

    /**
     * @brief Label metadata table
     *
     * Contains information about all labels used for control flow.
     * JLabel instructions emit labels, Jump/JumpIfFalse reference them.
     */
    LabelTable labelTable;

    /**
     * @brief Next available local variable ID
     *
     * Incremented each time a new local variable is declared.
     * Ensures each local gets a unique ID within the function.
     */
    LocalId nextLocalId{0};

    /**
     * @brief Next available label ID
     *
     * Incremented each time a new label is created (for if/else, loops).
     * Ensures each label gets a unique ID within the function.
     */
    LabelId nextLabelId{0};
};
