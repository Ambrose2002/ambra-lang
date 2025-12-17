/**
 * @file expr.h
 * @brief Abstract syntax tree (AST) node definitions for expressions.
 *
 * This file defines the expression node hierarchy for the Ambra parser.
 * All expression types inherit from the base Expr class and are tagged
 * with an ExprKind enum value.
 */
#pragma once
#include <string>
#include <vector>

/**
 * @brief Enumeration of all expression node types.
 */
enum ExprKind
{
    IntLiteral,         ///< Integer literal expression
    BoolLiteral,        ///< Boolean literal expression
    StringLiteral,      ///< String literal expression
    InterpolatedString, ///< Interpolated string expression
    Variable,           ///< Variable reference expression
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
    Equal,        ///< Equality comparison (==)
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
    virtual ~Expr() {};

    virtual bool operator==(const Expr& other) const;
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
        if (!expr && !other.expr) return true;
        if (!expr && !other.expr) return false;
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

    bool operator==(const IntLiteralExpr& other) const
    {
        return value == other.value && loc == other.loc;
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

    bool operator==(const BoolLiteralExpr& other) const
    {
        return value == other.value && loc == other.loc;
    }

  private:
    bool value; ///< The boolean value
};

/**
 * @brief Represents a string literal expression (no interpolation).
 */
class StringLiteralExpr : public Expr
{

  public:
    /**
     * @brief Constructs a string literal.
     * @param value The string value
     * @param loc Source location
     */
    StringLiteralExpr(std::string value, int line, int col) : value(value)
    {
        kind = StringLiteral;
        loc = {line, col};
    };

    bool operator==(const StringLiteralExpr& other) const
    {
        return value == other.value && loc == other.loc;
    }

  private:
    std::string value; ///< The string value
};

/**
 * @brief Represents a variable reference expression.
 */
class VariableExpr : public Expr
{

  public:
    /**
     * @brief Constructs a variable reference.
     * @param name The variable name
     * @param loc Source location
     */
    VariableExpr(std::string name, int line, int col) : name(name)
    {
        kind = Variable;
        loc = {line, col};
    };

    bool operator==(const VariableExpr& other) const
    {
        return name == other.name && loc == other.loc;
    }

  private:
    std::string name; ///< The variable name
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

    bool operator==(const UnaryExpr& other) const
    {
        return op == other.op && operand == other.operand && loc == other.loc;
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

    bool operator==(const BinaryExpr& other) const
    {
        return op == other.op && left == other.left && right == other.right && loc == other.loc;
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

    bool operator==(const GroupingExpr& other) const
    {
        return expression == other.expression && loc == other.loc;
    }

  private:
    std::unique_ptr<Expr> expression; ///< The inner expression
};

/**
 * @brief Represents an interpolated string expression.
 *
 * Contains alternating text chunks and embedded expressions.
 * Example: `"x={x} y={y}"` is parsed as:
 * - TextChunk "x="
 * - ExprPart (variable x)
 * - TextChunk " y="
 * - ExprPart (variable y)
 */
class InterpolatedStringExpr : public Expr
{
  public:
    /**
     * @brief Constructs an interpolated string.
     * @param parts The sequence of text chunks and expressions
     * @param loc Source location
     */
    InterpolatedStringExpr(std::vector<StringPart> parts, int line, int col)
        : parts(std::move(parts))
    {
        kind = InterpolatedString;
        loc = {line, col};
    };

    bool operator==(const InterpolatedStringExpr& other) const
    {
        return parts == other.parts && loc == other.loc;
    }

  private:
    std::vector<StringPart> parts; ///< Sequence of string parts
};