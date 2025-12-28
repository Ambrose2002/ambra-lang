/**
 * @file stmt.h
 * @brief Abstract syntax tree (AST) node definitions for statements.
 *
 * This file defines the statement node hierarchy for the Ambra parser.
 * All statement types inherit from the base Stmt class and are tagged
 * with a StmtKind enum value.
 */
#pragma once

#include "expr.h"

#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

/**
 * @brief Enumeration of all statement node types.
 */
enum StmtKind
{
    Summon,  ///< Variable declaration statement
    Say,     ///< Print statement
    Block,   ///< Code block statement
    IfChain, ///< Conditional chain statement
    While    ///< Loop statement
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
    StmtKind  kind; ///< The concrete type of this statement
    SourceLoc loc;  ///< The source location of this statement

    /// Virtual destructor for polymorphic deletion
    virtual ~Stmt() {};

    /**
     * @brief Compares two Stmt nodes for equality.
     * @param other The other statement to compare with
     * @return True if both statements are equivalent
     */
    virtual bool operator==(const Stmt& other) const = 0;

    /**
     * @brief Return a human-readable representation of this statement.
     *
     * Used by tests and debugging output. Derived classes must implement a
     * concise serialization suitable for debugging (not necessarily reversible).
     *
     * @return String representation of this statement
     */
    virtual std::string toString() const = 0;
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
     * @param line Source line number
     * @param col Source column number
     */
    SummonStmt(std::string name, std::unique_ptr<Expr> initializer, int line, int col)
        : name(name), initializer(std::move(initializer))
    {
        kind = Summon;
        loc = {line, col};
    }

    /**
     * @brief Compares two SummonStmt nodes for equality.
     * @param other The other statement to compare with
     * @return True if both have the same name, initializer, and source location
     */
    bool operator==(const Stmt& other) const override
    {

        if (other.kind != kind)
            return false;
        auto& o = static_cast<const SummonStmt&>(other);
        if (o.name != name || !(o.loc == loc))
        {
            return false;
        }

        if (!initializer && !o.initializer)
            return true;
        if (!initializer || !o.initializer)
            return false;
        return *initializer == *o.initializer;
    }

    /**
     * @brief Return a human-readable representation of this statement.
     * @return String representation of this statement
     */
    std::string toString() const override
    {
        std::string out = "Summon(";
        out += name;
        out += ", ";
        out += initializer ? initializer->toString() : std::string("null");
        out += ")";
        return out;
    }

    const Expr& getInitializer() const
    {
        return *initializer;
    }

    const std::string& getName() const
    {
        return name;
    }

  private:
    std::string           name;        ///< The variable name
    std::unique_ptr<Expr> initializer; ///< The initialization expression
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
     * @param line Source line number
     * @param col Source column number
     */
    SayStmt(std::unique_ptr<Expr> expression, int line, int col) : expression(std::move(expression))
    {
        kind = Say;
        loc = {line, col};
    };

    /**
     * @brief Compares two SayStmt nodes for equality.
     * @param other The other statement to compare with
     * @return True if both have the same expression and source location
     */
    bool operator==(const Stmt& other) const override
    {

        if (other.kind != kind)
            return false;
        auto& o = static_cast<const SayStmt&>(other);
        if (!(o.loc == loc))
        {
            return false;
        }

        if (!expression && !o.expression)
            return true;
        if (!expression || !o.expression)
            return false;
        return *expression == *o.expression;
    }

    /**
     * @brief Retrieves the expression.
     * @return A const reference to the expression.
     */
    const Expr& getExpression() const {
        return *expression;
    }

  private:
    std::unique_ptr<Expr> expression; ///< The expression to print

  public:
    /**
     * @brief Return a human-readable representation of this statement.
     * @return String representation of this statement
     */
    std::string toString() const override
    {
        return std::string("Say(") + (expression ? expression->toString() : std::string("null")) +
               ")";
    }
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
     * @param line Source line number
     * @param col Source column number
     */
    BlockStmt(std::vector<std::unique_ptr<Stmt>> statements, int line, int col)
        : statements(std::move(statements))
    {
        kind = Block;
        loc = {line, col};
    };

    /**
     * @brief Compares two BlockStmt nodes for equality.
     * @param other The other statement to compare with
     * @return True if both have the same statements in the same order and source location
     */
    bool operator==(const Stmt& other) const override
    {

        if (other.kind != kind)
            return false;
        auto& o = static_cast<const BlockStmt&>(other);
        if (!(o.loc == loc))
        {
            return false;
        }

        if (statements.size() != o.statements.size())
            return false;

        auto s1 = statements.begin();
        auto s2 = o.statements.begin();
        for (; s1 != statements.end() && s2 != o.statements.end(); ++s1, ++s2)
        {
            if (!*s1 && !*s2)
                continue;
            if (!*s1 || !*s2)
                return false;
            if (!(**s1 == **s2))
            {
                return false;
            }
        }
        return true;
    }

    using StmtIterator = std::vector<std::unique_ptr<Stmt>>::const_iterator;

    /**
     * @brief Returns an iterator to the beginning of the statements.
     * @return Iterator to the first statement
     */
    StmtIterator begin() const
    {
        return statements.begin();
    }

    /**
     * @brief Returns an iterator to the end of the statements.
     * @return Iterator past the last statement
     */
    StmtIterator end() const
    {
        return statements.end();
    }

    /**
     * @brief Returns a const iterator to the beginning of the statements.
     * @return Const iterator to the first statement
     */
    StmtIterator cbegin() const
    {
        return statements.cbegin();
    }

    /**
     * @brief Returns a const iterator to the end of the statements.
     * @return Const iterator past the last statement
     */
    StmtIterator cend() const
    {
        return statements.cend();
    }

    /**
     * @brief Return a human-readable representation of this statement.
     * @return String representation of this statement
     */
    std::string toString() const override
    {
        std::string out = "Block([";
        bool        first = true;
        for (const auto& s : statements)
        {
            if (!first)
                out += ", ";
            first = false;
            out += (s ? s->toString() : std::string("null"));
        }
        out += "])";
        return out;
    }

  private:
    std::vector<std::unique_ptr<Stmt>> statements; ///< The statements in the block
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
     * @param line Source line number
     * @param col Source column number
     */
    IfChainStmt(std::vector<std::tuple<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> branches,
                std::unique_ptr<BlockStmt> elseBranch, int line, int col)
        : branches(std::move(branches)), elseBranch(std::move(elseBranch))
    {
        kind = IfChain;
        loc = {line, col};
    };

    /**
     * @brief Compares two IfChainStmt nodes for equality.
     * @param other The other statement to compare with
     * @return True if both have the same branches and else branch in the same order and source
     * location
     */
    bool operator==(const Stmt& other) const override
    {

        if (other.kind != kind)
            return false;
        auto& o = static_cast<const IfChainStmt&>(other);
        if (!(o.loc == loc))
        {
            return false;
        }

        if (branches.size() != o.branches.size())
            return false;

        for (size_t i = 0; i < branches.size(); ++i)
        {
            const auto& lhs = branches[i];
            const auto& rhs = o.branches[i];

            const auto& lc = std::get<0>(lhs);
            const auto& rc = std::get<0>(rhs);
            if (!lc && !rc)
                ;
            else if (!lc || !rc)
                return false;
            else if (!(*lc == *rc))
                return false;

            const auto& lb = std::get<1>(lhs);
            const auto& rb = std::get<1>(rhs);
            if (!lb && !rb)
                continue;
            if (!lb || !rb)
                return false;
            if (!(*lb == *rb))
                return false;
        }

        if (!elseBranch && !o.elseBranch)
            return true;
        if (!elseBranch || !o.elseBranch)
            return false;

        return *elseBranch == *o.elseBranch;
    }

    /**
     * @brief Return a human-readable representation of this statement.
     * @return String representation of this statement
     */
    std::string toString() const override
    {
        std::string out = "IfChain([";
        for (size_t i = 0; i < branches.size(); ++i)
        {
            if (i > 0)
                out += ", ";
            const auto& cond = std::get<0>(branches[i]);
            const auto& blk = std::get<1>(branches[i]);
            out += "(";
            out += (cond ? cond->toString() : std::string("null"));
            out += ", ";
            out += (blk ? blk->toString() : std::string("null"));
            out += ")";
        }
        out += "]";
        if (elseBranch)
        {
            out += ", else=";
            out += elseBranch->toString();
        }
        out += ")";
        return out;
    }

    using StmtIterator =
        std::vector<std::tuple<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>>::const_iterator;

    /**
     * @brief Returns an iterator to the beginning of the statements.
     * @return Iterator to the first statement
     */
    StmtIterator begin() const
    {
        return branches.begin();
    }

    /**
     * @brief Returns an iterator to the end of the statements.
     * @return Iterator past the last statement
     */
    StmtIterator end() const
    {
        return branches.end();
    }

    /**
     * @brief Returns a const iterator to the beginning of the statements.
     * @return Const iterator to the first statement
     */
    StmtIterator cbegin() const
    {
        return branches.cbegin();
    }

    /**
     * @brief Returns a const iterator to the end of the statements.
     * @return Const iterator past the last statement
     */
    StmtIterator cend() const
    {
        return branches.cend();
    }

    const std::vector<std::tuple<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>>&
    getBranches() const
    {
        return branches;
    }

    const std::unique_ptr<BlockStmt>& getElseBranch() const
    {
        return elseBranch;
    }

  private:
    /// List of (condition, block) pairs, evaluated in order
    std::vector<std::tuple<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> branches;

    /// Optional fallback block executed if all conditions are false
    std::unique_ptr<BlockStmt> elseBranch;
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
     * @param line Source line number
     * @param col Source column number
     */
    WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<BlockStmt> body, int line, int col)
        : condition(std::move(condition)), body(std::move(body))
    {
        kind = While;
        loc = {line, col};
    };

    /**
     * @brief Return a human-readable representation of this statement.
     * @return String representation of this statement
     */
    std::string toString() const override
    {
        return std::string("While(") + (condition ? condition->toString() : std::string("null")) +
               ", " + (body ? body->toString() : std::string("null")) + ")";
    }

    const Expr& getCondition() const
    {
        return *condition;
    }

    const BlockStmt& getBody() const
    {
        return *body;
    }

  private:
    std::unique_ptr<Expr>      condition; ///< The loop condition
    std::unique_ptr<BlockStmt> body;      ///< The loop body

    /**
     * @brief Compares two WhileStmt nodes for equality.
     * @param other The other statement to compare with
     * @return True if both have the same condition and body and source location
     */
    bool operator==(const Stmt& other) const override
    {
        if (kind != other.kind)
            return false;

        const auto& o = static_cast<const WhileStmt&>(other);

        if (!(loc == o.loc))
            return false;

        if (!condition && !o.condition && !body && !o.body)
            return true;

        if (!condition || !o.condition || !body || !o.body)
            return false;

        return *condition == *o.condition && *body == *o.body;
    }
};