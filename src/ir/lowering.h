/**
 * @file lowering.h
 * @brief AST-to-IR lowering phase for the Ambra compiler
 *
 * This file defines the LoweringContext structure which transforms a type-checked
 * Abstract Syntax Tree (AST) into a low-level Intermediate Representation (IR).
 * The IR uses a stack-based instruction set suitable for interpretation or
 * further compilation.
 *
 * The lowering phase:
 * - Converts high-level expressions into stack operations
 * - Allocates local variable slots and tracks scoping
 * - Generates control flow instructions (jumps, labels)
 * - Creates constant pool entries for literals
 * - Inserts type conversions (e.g., ToString for printing)
 *
 * The IR generated is structured as:
 * - Constants: Pool of literal values referenced by instructions
 * - Instructions: Sequence of stack-based operations
 * - Local table: Variable metadata (name, type, slot)
 * - Control flow: Labels and jumps for conditionals/loops
 */

#include "ast/expr.h"
#include "ast/stmt.h"
#include "program.h"
#include "sema/analyzer.h"

#include <unordered_map>

/**
 * @brief Context for lowering AST nodes to IR instructions
 *
 * The LoweringContext maintains state during the lowering process including:
 * - Current function being lowered (for nested functions in the future)
 * - The target IR program being constructed
 * - Scope stack for variable resolution
 * - Type information from semantic analysis
 *
 * The context is stateful and mutates the IrProgram as lowering proceeds.
 * Lowering is a single-pass traversal of the AST.
 */
struct LoweringContext
{
    /** @brief Current function being lowered (nullptr for top-level code) */
    IrFunction* currentFunction;

    /** @brief Target IR program being constructed */
    IrProgram* program;

    /**
     * @brief Stack of local variable scopes
     *
     * Each scope maps symbols to their corresponding local IDs.
     * Inner scopes are pushed/popped as blocks are entered/exited.
     * This enables proper variable shadowing and scope isolation.
     */
    std::vector<std::unordered_map<const Symbol*, LocalId>> localScopes;

    /** @brief Type information from the type checking phase */
    const TypeTable& typeTable;

    /** @brief Symbol resolution information from semantic analysis */
    const ResolutionTable& resolutionTable;

    /** @brief Error flag set if lowering encounters an unrecoverable issue */
    bool hadError = false;

    // ==================================================================================
    // EXPRESSION LOWERING
    // ==================================================================================
    // Expression lowering generates instructions that evaluate expressions and leave
    // their result on the stack. The expectedType parameter enables type conversions
    // when the expression result needs to match a specific type context.

    /**
     * @brief Lower an expression to IR instructions
     * @param expr The expression AST node to lower
     * @param expectedType The type context where this expression is used
     *
     * Dispatches to specific lowering methods based on expression kind.
     * After lowering, the expression result is on top of the stack.
     */
    void lowerExpression(const Expr* expr, Type expectedType);

    /**
     * @brief Lower integer literal expression
     * @param expr Integer literal AST node
     * @param expectedType Expected type (typically I32 or String32 if printed)
     *
     * Generates: PushConst (integer constant)
     * Optionally followed by ToString if expectedType is String32
     */
    void lowerIntExpr(const IntLiteralExpr* expr, Type expectedType);

    /**
     * @brief Lower string literal or interpolated string expression
     * @param expr String expression AST node
     * @param expectedType Expected type (typically String32)
     *
     * For simple strings: PushConst (string constant)
     * For interpolated strings: Multiple PushConst/expression evaluations + ConcatString
     */
    void lowerStringExpr(const StringExpr* expr, Type expectedType);

    /**
     * @brief Lower boolean literal expression
     * @param expr Boolean literal AST node (affirmative/negative)
     * @param expectedType Expected type (typically Bool32 or String32 if printed)
     *
     * Generates: PushConst (boolean constant)
     * Optionally followed by ToString if expectedType is String32
     */
    void lowerBoolExpr(const BoolLiteralExpr* expr, Type expectedType);

    /**
     * @brief Lower identifier (variable reference) expression
     * @param expr Identifier AST node
     * @param expectedType Expected type in current context
     *
     * Generates: LoadLocal (variable's local ID)
     * Optionally followed by ToString if type conversion needed
     */
    void lowerIdentifierExpr(const IdentifierExpr* expr, Type expectedType);

    /**
     * @brief Lower unary expression (negation, logical NOT)
     * @param expr Unary expression AST node
     * @param expectedType Expected result type
     *
     * Generates:
     * 1. Lower operand expression
     * 2. Unary operation instruction (NegI32, NotBool)
     * 3. Optional ToString for printing
     */
    void lowerUnaryExpr(const UnaryExpr* expr, Type expectedType);

    /**
     * @brief Lower binary expression (arithmetic, comparison)
     * @param expr Binary expression AST node
     * @param expectedType Expected result type
     *
     * Generates:
     * 1. Lower left operand
     * 2. Lower right operand
     * 3. Binary operation instruction (AddI32, CmpEqI32, etc.)
     * 4. Optional ToString for result
     */
    void lowerBinaryExpr(const BinaryExpr* expr, Type expectedType);

    /**
     * @brief Lower grouping (parenthesized) expression
     * @param expr Grouping expression AST node
     * @param expectedType Expected result type
     *
     * Simply lowers the inner expression; parentheses don't generate instructions
     */
    void lowerGroupingExpr(const GroupingExpr* expr, Type expectedType);

    // ==================================================================================
    // STATEMENT LOWERING
    // ==================================================================================
    // Statement lowering generates instructions that perform side effects but don't
    // leave values on the stack (or clean up after themselves).

    /**
     * @brief Lower a statement to IR instructions
     * @param stmt The statement AST node to lower
     *
     * Dispatches to specific lowering methods based on statement kind.
     * Statements don't leave values on the stack.
     */
    void lowerStatement(const Stmt* stmt);

    /**
     * @brief Lower variable declaration statement (summon)
     * @param stmt Variable declaration AST node
     *
     * Generates:
     * 1. Lower initializer expression
     * 2. StoreLocal (allocate local variable and store value)
     * 3. Add entry to local table and current scope
     */
    void lowerSummonStatement(const SummonStmt* stmt);

    /**
     * @brief Lower print statement (say)
     * @param stmt Print statement AST node
     *
     * Generates:
     * 1. Lower expression to print
     * 2. ToString if expression is not already a string
     * 3. PrintString instruction
     */
    void lowerSayStatement(const SayStmt* stmt);

    /**
     * @brief Lower block statement (scope with multiple statements)
     * @param stmt Block statement AST node
     *
     * Generates:
     * 1. Push new variable scope
     * 2. Lower each statement in block
     * 3. Pop variable scope
     *
     * Variables declared in the block are out of scope after block exit
     */
    void lowerBlockStatement(const BlockStmt* stmt);

    /**
     * @brief Lower if/else statement chain (should/otherwise)
     * @param stmt If chain statement AST node
     *
     * Generates:
     * 1. Lower condition expression
     * 2. JumpIfFalse to else branch or end
     * 3. Lower if body
     * 4. Jump to end (if else exists)
     * 5. JLabel for else branch
     * 6. Lower else body (if exists)
     * 7. JLabel for end
     */
    void lowerIfChainStatement(const IfChainStmt* stmt);

    /**
     * @brief Lower while loop statement (aslongas)
     * @param stmt While statement AST node
     *
     * Generates:
     * 1. JLabel for loop start
     * 2. Lower condition expression
     * 3. JumpIfFalse to loop end
     * 4. Lower loop body
     * 5. Jump back to loop start
     * 6. JLabel for loop end
     */
    void lowerWhileStatement(const WhileStmt* stmt);

    // ==================================================================================
    // PROGRAM LOWERING
    // ==================================================================================

    /**
     * @brief Lower entire program from AST to IR
     * @param program The root AST node representing the complete program
     * @return Complete IR program ready for execution or further compilation
     *
     * Main entry point for the lowering phase. Initializes the IR program,
     * processes all top-level statements, and returns the complete IR structure.
     *
     * The returned IrProgram contains:
     * - Constant pool with all literals
     * - Main function with instruction sequence
     * - Local variable table
     * - Label information for control flow
     */
    IrProgram lowerProgram(const Program* program);
};
