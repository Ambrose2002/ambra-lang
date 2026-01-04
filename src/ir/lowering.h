/**
 * @file lowering.h
 * @brief AST-to-IR lowering phase for the Ambra compiler
 *
 * This file defines the LoweringContext structure which transforms a type-checked
 * Abstract Syntax Tree (AST) into a low-level Intermediate Representation (IR).
 * The IR uses a stack-based instruction set suitable for interpretation or
 * further compilation.
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

    /**
     * @brief Register a label in the label table
     * @param id The label identifier to define
     *
     * Allocates a new label entry without emitting a JLabel instruction yet.
     * Used to reserve label IDs before their actual position is known.
     */
    void defineLabel(LabelId id);

    /**
     * @brief Emit a label instruction at the current position
     * @param id The label identifier to emit
     */
    void emitLabel(LabelId id);

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
     */
    void lowerIdentifierExpr(const IdentifierExpr* expr, Type expectedType);

    /**
     * @brief Lower unary expression (negation, logical NOT)
     * @param expr Unary expression AST node
     * @param expectedType Expected result type
     */
    void lowerUnaryExpr(const UnaryExpr* expr, Type expectedType);

    /**
     * @brief Lower binary expression (arithmetic, comparison)
     * @param expr Binary expression AST node
     * @param expectedType Expected result type
     */
    void lowerBinaryExpr(const BinaryExpr* expr, Type expectedType);

    /**
     * @brief Lower grouping (parenthesized) expression
     * @param expr Grouping expression AST node
     * @param expectedType Expected result type
     */
    void lowerGroupingExpr(const GroupingExpr* expr, Type expectedType);

    /**
     * @brief Lower a statement to IR instructions
     * @param stmt The statement AST node to lower
     */
    void lowerStatement(const Stmt* stmt);

    /**
     * @brief Lower variable declaration statement (summon)
     * @param stmt Variable declaration AST node
     */
    void lowerSummonStatement(const SummonStmt* stmt);

    /**
     * @brief Lower print statement (say)
     * @param stmt Print statement AST node
     */
    void lowerSayStatement(const SayStmt* stmt);

    /**
     * @brief Lower block statement (scope with multiple statements)
     * @param stmt Block statement AST node
     * Variables declared in the block are out of scope after block exit
     */
    void lowerBlockStatement(const BlockStmt* stmt);

    /**
     * @brief Lower if/else statement chain (should/otherwise)
     * @param stmt If chain statement AST node
     */
    void lowerIfChainStatement(const IfChainStmt* stmt);

    /**
     * @brief Lower while loop statement (aslongas)
     * @param stmt While statement AST node
     */
    void lowerWhileStatement(const WhileStmt* stmt);

    /**
     * @brief Lower entire program from AST to IR
     * @param program The root AST node representing the complete program
     * @return Complete IR program ready for execution or further compilation
     */
    IrProgram lowerProgram(const Program* program);
};
