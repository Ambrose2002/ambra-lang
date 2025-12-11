
#include <string>
#include <vector>

enum ExprKind
{
    IntLiteral,
    BoolLiteral,
    StringLiteral,
    InterpolatedString,
    Variable,
    Unary,
    Binary,
    Grouping
};

enum UnaryOpKind
{
    LogicalNot,
    ArithmeticNegate
};

enum BinaryOpKind
{
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Add,
    Subtract,
    Multiply,
    Divide
};

class SourceLoc
{

  public:
    SourceLoc(int line, int col) : line(line), col(col) {};
    int line;
    int col;
};

class Expr
{
  public:
    ExprKind kind;
    virtual ~Expr() {};
};

struct StringPart
{
    enum Kind
    {
        TEXT,
        EXPR
    } kind;
    std::string           text;
    std::unique_ptr<Expr> expr;
};

class IntLiteralExpr : public Expr
{

  public:
    IntLiteralExpr(int value, SourceLoc loc) : value(value), loc(loc)
    {
        kind = IntLiteral;
    };

  private:
    int       value;
    SourceLoc loc;
};

class BoolLiteralExpr : public Expr
{

  public:
    BoolLiteralExpr(bool value, SourceLoc loc) : value(value), loc(loc)
    {
        kind = BoolLiteral;
    };

  private:
    bool      value;
    SourceLoc loc;
};

class StringLiteralExpr : public Expr
{

  public:
    StringLiteralExpr(std::string value, SourceLoc loc) : value(value), loc(loc)
    {
        kind = StringLiteral;
    };

  private:
    std::string value;
    SourceLoc   loc;
};

class VariableExpr : public Expr
{

  public:
    VariableExpr(std::string name, SourceLoc loc) : name(name), loc(loc)
    {
        kind = Variable;
    };

  private:
    std::string name;
    SourceLoc   loc;
};

class UnaryExpr : public Expr
{
  public:
    UnaryExpr(UnaryOpKind op, std::unique_ptr<Expr> operand, SourceLoc loc)
        : op(op), operand(std::move(operand)), loc(loc)
    {
        kind = Unary;
    };

  private:
    UnaryOpKind           op;
    std::unique_ptr<Expr> operand;
    SourceLoc             loc;
};

class BinaryExpr : public Expr
{
  public:
    BinaryExpr(std::unique_ptr<Expr> left, BinaryOpKind op, std::unique_ptr<Expr> right,
               SourceLoc loc)
        : left(std::move(left)), op(op), right(std::move(right)), loc(loc)
    {
        kind = Binary;
    };

  private:
    std::unique_ptr<Expr> left;
    BinaryOpKind          op;
    std::unique_ptr<Expr> right;
    SourceLoc             loc;
};

class GroupingExpr : public Expr
{
  public:
    GroupingExpr(std::unique_ptr<Expr> expression, SourceLoc loc)
        : expression(std::move(expression)), loc(loc)
    {
        kind = Grouping;
    };

  private:
    std::unique_ptr<Expr> expression;
    SourceLoc             loc;
};

class InterpolatedStringExpr : public Expr
{
  public:
    InterpolatedStringExpr(std::vector<StringPart> parts, SourceLoc loc) : parts(parts), loc(loc)
    {
        kind = InterpolatedString;
    };

  private:
    std::vector<StringPart> parts;
    SourceLoc               loc;
};