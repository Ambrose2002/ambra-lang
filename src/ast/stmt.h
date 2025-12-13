/**
 * @file stmt.h
 * @brief Abstract syntax tree (AST) node definitions for statements.
 *
 * This file defines the statement node hierarchy for the Ambra parser.
 * All statement types inherit from the base Stmt class and are tagged
 * with a StmtKind enum value.
 */

#include "expr.h"

#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

/**
 * @brief Enumeration of all statement node types.
 */
enum StmtKind
{
    Summon,    ///< Variable declaration statement
    Say,       ///< Print statement
    Block,     ///< Code block statement
    IfChain,   ///< Conditional chain statement
    While      ///< Loop statement
};

/**
 * @brief Base class for all statement AST nodes.
 *
 * All concrete statement types (SummonStmt, SayStmt, etc.)
 * inherit from this class. The kind member indicates the concrete type.
 */
class Stmt
{
  public:
    StmtKind kind;  ///< The concrete type of this statement
    
    /// Virtual destructor for polymorphic deletion
    virtual ~Stmt() {};
};

/**
 * @brief Represents a variable declaration statement.
 *
 * Example: `summon x = 5;`
 */
class SummonStmt : public Stmt
{

  public:
    /**
     * @brief Constructs a variable declaration.
     * @param name The variable name
     * @param initializer The initialization expression
     * @param loc Source location
     */
    SummonStmt(std::string name, std::unique_ptr<Expr> initializer, SourceLoc loc)
        : name(name), initializer(std::move(initializer)), loc(loc)
    {
        kind = Summon;
    }

  private:
    std::string           name;           ///< The variable name
    std::unique_ptr<Expr> initializer;    ///< The initialization expression
    SourceLoc             loc;            ///< Source location
};

/**
 * @brief Represents a print statement.
 *
 * Example: `say "Hello, " + name;`
 */
class SayStmt : public Stmt
{
  public:
    /**
     * @brief Constructs a print statement.
     * @param expression The expression to print
     * @param loc Source location
     */
    SayStmt(std::unique_ptr<Expr> expression, SourceLoc loc)
        : expression(std::move(expression)), loc(loc)
    {
        kind = Say;
    };

  private:
    std::unique_ptr<Expr> expression;    ///< The expression to print
    SourceLoc             loc;           ///< Source location
};

/**
 * @brief Represents a code block statement.
 *
 * Contains a sequence of statements. Blocks can be nested arbitrarily.
 * Example: `{ summon x = 5; say x; }`
 */
class BlockStmt : public Stmt
{
  public:
    /**
     * @brief Constructs a block statement.
     * @param statements The sequence of statements in the block
     * @param loc Source location
     */
    BlockStmt(std::vector<std::unique_ptr<Stmt>> statements, SourceLoc loc)
        : statements(std::move(statements)), loc(loc)
    {
        kind = Block;
    };

  private:
    std::vector<std::unique_ptr<Stmt>> statements;    ///< The statements in the block
    SourceLoc                          loc;           ///< Source location
};

/**
 * @brief Represents a conditional chain statement.
 *
 * Supports multiple `should` branches, optional `otherwise should` branches,
 * and an optional final `otherwise` branch. All branches are stored in a
 * single node for simplicity.
 * Example:
 * ```
 * should (x > 5) { say "big"; }
 * otherwise should (x > 0) { say "small"; }
 * otherwise { say "negative"; }
 * ```
 */
class IfChainStmt : public Stmt
{
  public:
    /**
     * @brief Constructs a conditional chain.
     * @param branches Ordered list of (condition, block) pairs
     * @param elseBranch Optional fallback block (no condition)
     * @param loc Source location
     */
    IfChainStmt(std::vector<std::tuple<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> branches,
                std::unique_ptr<BlockStmt> elseBranch, SourceLoc loc)
        : branches(std::move(branches)), elseBranch(std::move(elseBranch)), loc(loc)
    {
        kind = IfChain;
    };

  private:
    /// List of (condition, block) pairs, evaluated in order
    std::vector<std::tuple<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> branches;
    
    /// Optional fallback block executed if all conditions are false
    std::optional<std::unique_ptr<BlockStmt>> elseBranch;
    
    SourceLoc loc;    ///< Source location
};

/**
 * @brief Represents a loop statement.
 *
 * Repeatedly executes the body while the condition is true.
 * Example: `aslongas (x > 0) { say x; x = x - 1; }`
 */
class WhileStmt : public Stmt
{
  public:
    /**
     * @brief Constructs a loop statement.
     * @param condition The loop condition expression
     * @param body The loop body block
     * @param loc Source location
     */
    WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<BlockStmt> body, SourceLoc loc)
        : condition(std::move(condition)), body(std::move(body)), loc(loc)
    {
        kind = While;
    };

  private:
    std::unique_ptr<Expr> condition;      ///< The loop condition
    std::unique_ptr<BlockStmt> body;      ///< The loop body
    SourceLoc             loc;            ///< Source location
};