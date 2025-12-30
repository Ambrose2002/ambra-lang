/**
 * @file expr.h
 * @brief Abstract syntax tree (AST) node definitions for expressions.
 *
 * This file defines the expression node hierarchy for the Ambra parser.
 * All expression types inherit from the base Expr class and are tagged
 * with an ExprKind enum value.
 */
#pragma once
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Enumeration of all expression node types.
 */
enum ExprKind
{
    IntLiteral,         ///< Integer literal expression
    BoolLiteral,        ///< Boolean literal expression
    InterpolatedString, ///< Interpolated string expression
    Identifier,         ///< Identifier reference expression
    Unary,              ///< Unary operator expression
    Binary,             ///< Binary operator expression
    Grouping            ///< Parenthesized grouping expression
};

/**
 * @brief Enumeration of unary operator types.
 */
enum UnaryOpKind
{
    LogicalNot,      ///< Logical negation (not)
    ArithmeticNegate ///< Arithmetic negation (-)
};

/**
 * @brief Enumeration of binary operator types.
 */
enum BinaryOpKind
{
    EqualEqual,   ///< Equality comparison (==)
    NotEqual,     ///< Inequality comparison (!=)
    Less,         ///< Less than (<)
    LessEqual,    ///< Less than or equal (<=)
    Greater,      ///< Greater than (>)
    GreaterEqual, ///< Greater than or equal (>=)
    Add,          ///< Addition (+)
    Subtract,     ///< Subtraction (-)
    Multiply,     ///< Multiplication (*)
    Divide        ///< Division (/)
};

/**
 * @brief Represents a position in the source code.
 *
 * Used to track the source location of AST nodes for error reporting.
 */
struct SourceLoc
{
    int line; ///< Line number
    int col;  ///< Column number

    bool operator==(const SourceLoc& other) const
    {
        return line == other.line && col == other.col;
    }
};

/**
 * @brief Base class for all expression AST nodes.
 *
 * All concrete expression types (IntLiteralExpr, BinaryExpr, etc.)
 * inherit from this class. The kind member indicates the concrete type.
 */
class Expr
{
  public:
    ExprKind  kind; ///< The concrete type of this expression
    SourceLoc loc;

    /// Virtual destructor for polymorphic deletion
    virtual ~Expr() = default;

    /**
     * @brief Compare two expressions for structural equality.
     *
     * Derived classes must implement this to perform a deep, structural
     * comparison of the expression subtree. Implementations should first
     * check the concrete `kind` and the source location when relevant,
     * then compare any fields and recursively compare child expressions.
     *
     * @param other Expression to compare against
     * @return true if expressions are structurally equal, false otherwise
     */
    virtual bool operator==(const Expr& other) const = 0;
    /**
     * @brief Return a human-readable representation of this expression.
     *
     * Used by tests and debugging output. Derived classes must implement a
     * concise serialization suitable for debugging (not necessarily reversible).
     */
    virtual std::string toString() const = 0;
};

/**
 * @brief Represents a part of an interpolated string.
 *
 * Parts can be either text chunks or embedded expressions.
 */
struct StringPart
{
    /// Distinguishes between text and expression parts
    enum Kind
    {
        TEXT, ///< Text chunk
        EXPR  ///< Embedded expression
    } kind;

    std::string           text; ///< Text content (for TEXT parts)
    std::unique_ptr<Expr> expr; ///< Expression content (for EXPR parts)

    /**
     * @brief Deep-compare two StringPart instances.
     *
     * For TEXT parts this compares the contained string. For EXPR parts
     * this performs a deep structural comparison of the embedded expression
     * (handling null pointers appropriately).
     */
    bool operator==(const StringPart& other) const
    {
        if (kind != other.kind)
        {
            return false;
        }
        if (kind == TEXT)
        {
            return text == other.text;
        }
        if (!expr && !other.expr)
            return true;
        if (!expr || !other.expr)
            return false;
        return *expr == *other.expr;
    }
};

/**
 * @brief Represents an integer literal expression.
 */
class IntLiteralExpr : public Expr
{

  public:
    /**
     * @brief Constructs an integer literal.
     * @param value The integer value
     * @param loc Source location
     */
    IntLiteralExpr(int value, int line, int col) : value(value)
    {
        loc = {line, col};
        kind = IntLiteral;
    };

    int getValue() const
    {
        return value;
    }

    bool operator==(const Expr& other) const override
    {
        if (other.kind != kind)
            return false;
        auto& o = static_cast<const IntLiteralExpr&>(other);
        return value == o.value && loc == o.loc;
    }

    std::string toString() const override
    {
        return std::string("Int(") + std::to_string(value) + ")";
    }

  private:
    int value; ///< The integer value
};

/**
 * @brief Represents a boolean literal expression.
 */
class BoolLiteralExpr : public Expr
{

  public:
    /**
     * @brief Constructs a boolean literal.
     * @param value The boolean value
     * @param loc Source location
     */
    BoolLiteralExpr(bool value, int line, int col) : value(value)
    {
        kind = BoolLiteral;
        loc = {line, col};
    };

    bool operator==(const Expr& other) const override
    {
        if (other.kind != kind)
            return false;
        auto& o = static_cast<const BoolLiteralExpr&>(other);
        return value == o.value && loc == o.loc;
    }

    std::string toString() const override
    {
        return std::string("Bool(") + (value ? "true" : "false") + ")";
    }

  private:
    bool value; ///< The boolean value
};
/**
 * @brief Represents a identifier reference expression.
 */
class IdentifierExpr : public Expr
{

  public:
    /**
     * @brief Constructs a identifier reference.
     * @param name The identifier name
     * @param loc Source location
     */
    IdentifierExpr(std::string name, int line, int col) : name(name)
    {
        kind = Identifier;
        loc = {line, col};
    };

    bool operator==(const Expr& other) const override
    {
        if (other.kind != kind)
            return false;
        auto& o = static_cast<const IdentifierExpr&>(other);
        return name == o.name && loc == o.loc;
    }

    std::string toString() const override
    {
        return std::string("Ident(") + name + ")";
    }

    std::string getName() const
    {
        return name;
    }

  private:
    std::string name; ///< The identifier name
};

/**
 * @brief Represents a unary operator expression.
 *
 * Supports logical negation (not) and arithmetic negation (-).
 */
class UnaryExpr : public Expr
{
  public:
    /**
     * @brief Constructs a unary expression.
     * @param op The unary operator
     * @param operand The operand expression
     * @param loc Source location
     */
    UnaryExpr(UnaryOpKind op, std::unique_ptr<Expr> operand, int line, int col)
        : op(op), operand(std::move(operand))
    {
        kind = Unary;
        loc = {line, col};
    };

    bool operator==(const Expr& other) const override
    {
        if (other.kind != kind)
            return false;
        auto& o = static_cast<const UnaryExpr&>(other);
        if (op != o.op || !(loc == o.loc))
        {
            return false;
        };
        if (!operand && !o.operand)
            return true;
        if (!operand || !o.operand)
            return false;
        return *operand == *o.operand;
    }

    std::string toString() const override
    {
        std::string opName = (op == LogicalNot) ? "Not" : "Neg";
        return std::string("Unary(") + opName + ", " + (operand ? operand->toString() : "null") +
               ")";
    }

    /**
     * @brief Retrieves the operand of a unary expression.
     * @return A const reference to the operand.
     */
    const Expr& getOperand() const
    {
        return *operand;
    }

    const UnaryOpKind getOperator() const
    {
        return op;
    }

  private:
    UnaryOpKind           op;      ///< The unary operator
    std::unique_ptr<Expr> operand; ///< The operand expression
};

/**
 * @brief Represents a binary operator expression.
 *
 * Supports arithmetic, comparison, and equality operations.
 */
class BinaryExpr : public Expr
{
  public:
    /**
     * @brief Constructs a binary expression.
     * @param left The left operand expression
     * @param op The binary operator
     * @param right The right operand expression
     * @param loc Source location
     */
    BinaryExpr(std::unique_ptr<Expr> left, BinaryOpKind op, std::unique_ptr<Expr> right, int line,
               int col)
        : left(std::move(left)), op(op), right(std::move(right))
    {
        kind = Binary;
        loc = {line, col};
    };

    bool operator==(const Expr& other) const override
    {
        if (other.kind != kind)
            return false;
        auto& o = static_cast<const BinaryExpr&>(other);
        if (op != o.op || !(loc == o.loc))
        {
            return false;
        };
        if (!left && !o.left && !right && !o.right)
            return true;
        if (!left || !o.left || !right || !o.right)
            return false;
        return *left == *o.left && *right == *o.right;
    }

    std::string toString() const override
    {
        std::string opName;
        switch (op)
        {
        case EqualEqual:
            opName = "==";
            break;
        case NotEqual:
            opName = "!=";
            break;
        case Less:
            opName = "<";
            break;
        case LessEqual:
            opName = "<=";
            break;
        case Greater:
            opName = ">";
            break;
        case GreaterEqual:
            opName = ">=";
            break;
        case Add:
            opName = "+";
            break;
        case Subtract:
            opName = "-";
            break;
        case Multiply:
            opName = "*";
            break;
        case Divide:
            opName = "/";
            break;
        default:
            opName = "?";
        }
        return std::string("Binary(") + (left ? left->toString() : "null") + " " + opName + " " +
               (right ? right->toString() : "null") + ")";
    }

    /**
     * @brief Retrieves the left operand of a binary expression.
     * @return A const reference to the left-hand side expression.
     */
    const Expr& getLeft() const
    {
        return *left;
    }

    /**
     * @brief Retrieves the right operand of a binary expression.
     * @return A const reference to the right-hand side expression.
     */
    const Expr& getRight() const
    {
        return *right;
    }

    const BinaryOpKind getOperator() const
    {
        return op;
    }

  private:
    std::unique_ptr<Expr> left;  ///< The left operand
    BinaryOpKind          op;    ///< The binary operator
    std::unique_ptr<Expr> right; ///< The right operand
};

/**
 * @brief Represents a parenthesized grouping expression.
 *
 * Used to override operator precedence via explicit parentheses.
 */
class GroupingExpr : public Expr
{
  public:
    /**
     * @brief Constructs a grouping expression.
     * @param expression The inner expression
     * @param loc Source location
     */
    GroupingExpr(std::unique_ptr<Expr> expression, int line, int col)
        : expression(std::move(expression))
    {
        kind = Grouping;
        loc = {line, col};
    };

    bool operator==(const Expr& other) const override
    {
        if (other.kind != kind)
            return false;
        auto& o = static_cast<const GroupingExpr&>(other);
        if (!(loc == o.loc))
            return false;
        if (!expression && !o.expression)
            return true;
        if (!expression || !o.expression)
            return false;
        return *expression == *o.expression;
    }

    std::string toString() const override
    {
        return std::string("Group(") + (expression ? expression->toString() : "null") + ")";
    }

    /**
     * @brief Retrieves the nested expression.
     * @return A const reference to the nested expression.
     */
    const Expr& getExpression() const
    {
        return *expression;
    }

  private:
    std::unique_ptr<Expr> expression; ///< The inner expression
};

/**
 * @brief Represents a string expression.
 *
 * Contains alternating text chunks and embedded expressions.
 * Example: `"x={x} y={y}"` is parsed as:
 * - TextChunk "x="
 * - ExprPart (identifier x)
 * - TextChunk " y="
 * - ExprPart (identifier y)
 */
class StringExpr : public Expr
{
  public:
    /**
     * @brief Constructs an interpolated string.
     * @param parts The sequence of text chunks and expressions
     * @param loc Source location
     */
    StringExpr(std::vector<StringPart>&& parts, int line, int col) : parts(std::move(parts))
    {
        kind = InterpolatedString;
        loc = {line, col};
    };

    bool operator==(const Expr& other) const override
    {
        if (other.kind != kind)
            return false;
        auto& o = static_cast<const StringExpr&>(other);
        return parts == o.parts && loc == o.loc;
    }

    std::string toString() const override
    {
        std::string out = "String(\"";
        for (const auto& p : parts)
        {
            if (p.kind == StringPart::TEXT)
            {
                out += p.text;
            }
            else
            {
                out += "${" + (p.expr ? p.expr->toString() : std::string("null")) + "}";
            }
        }
        out += "\")";
        return out;
    }

    using StringPartIterator = std::vector<StringPart>::const_iterator;

    StringPartIterator begin() const
    {
        return parts.begin();
    }

    StringPartIterator end() const
    {
        return parts.end();
    }

    StringPartIterator cbegin() const
    {
        return parts.cbegin();
    }

    StringPartIterator cend() const
    {
        return parts.cend();
    }

    const std::vector<StringPart>& getParts() const
    {
        return parts;
    }

  private:
    std::vector<StringPart> parts; ///< Sequence of string parts
};