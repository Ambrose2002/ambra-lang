/**
 * @file program.h
 * @brief Top-level IR program structure
 *
 * This file defines the IrProgram structure which represents a complete
 * compiled Ambra program in Intermediate Representation form.
 *
 * An IR program consists of:
 * - Constant pool: All literal values used in the program
 * - Main function: The top-level code to execute
 * - ID generators: Tracking next available constant ID
 *
 * The IR is designed to be:
 * - Simple to interpret or compile further
 * - Independent of source-level constructs
 * - Stack-based for straightforward execution
 *
 * Future versions may support multiple functions for user-defined functions.
 */

#include "functions.h"

#include <vector>

/**
 * @brief Complete IR program structure
 *
 * Represents the entire program after lowering from AST to IR.
 * Contains all the information needed to execute or further compile the program.
 *
 * The program structure is designed for single-function programs (main only)
 * in the current version. Future versions will support multiple functions.
 */
struct IrProgram
{
    /**
     * @brief Pool of constant values referenced by instructions
     *
     * All literal values (integers, booleans, strings) are stored here
     * and referenced by ConstId in PushConst instructions. This avoids
     * embedding large values directly in the instruction stream.
     *
     * Constants may be deduplicated - identical literals share the same entry.
     */
    std::vector<Constant> constants;

    /**
     * @brief The main function containing top-level program code
     *
     * In the current version, all program code goes into the main function.
     * The main function contains:
     * - Instruction sequence
     * - Local variable table
     * - Label table for control flow
     */
    IrFunction main;

    /**
     * @brief Next available constant ID
     *
     * Tracks the next ConstId to assign when adding a new constant to the pool.
     * Incremented each time a constant is added.
     */
    ConstId nextConstId{0};
};
