
#include <string>
#include <vector>


enum ExprKind
{
    IntLiteral,
    BoolLiteral,
    StringLIteral,
    InterpolatedString,
    Variable,
    Unary,
    Binary,
    Grouping
};

enum UnaryOpKind {
    LogicalNot,
    ArithmeticNegate
};

enum BinaryOpKind {
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
};

struct StringPart {
    enum Kind {TEXT, EXPR} kind;
    std::string text;
    Expr* expr;
};

class IntLiteralExpr: public Expr {

    public:
    IntLiteralExpr (int value, SourceLoc loc): value(value), loc(loc) {};

    private:
    int value;
    SourceLoc loc;
};

class BoolLiteral: public Expr {

    public:
    BoolLiteral (bool value, SourceLoc loc): value(value), loc(loc) {};

    private:
    bool value;
    SourceLoc loc;
};

class StringLiteral: public Expr {

    public:
    StringLiteral (std::string value, SourceLoc loc): value(value), loc(loc) {};

    private:
    std::string value;
    SourceLoc loc;
};

class VariableExpr: public Expr {

    public:
    VariableExpr (std::string name, SourceLoc loc): name(name), loc(loc) {};

    private:
    std::string name;
    SourceLoc loc;
};

class UnaryExpr: public Expr {
    public:
    UnaryExpr ();

    private:
    UnaryOpKind op;
    Expr* operand;
    SourceLoc loc;
};

class BinaryExpr: public Expr {
    public:
    BinaryExpr();

    private:
    Expr* left;
    BinaryOpKind op;
    Expr* right;
    SourceLoc loc;
};

class GroupingExpr: public Expr {
    public:
    GroupingExpr();

    private:
    Expr* expression;
    SourceLoc loc;
};

class InterpolatedStringExpr: public Expr {
    public:
    InterpolatedStringExpr();

    private:
    std::vector<StringPart> parts;
    SourceLoc loc;
};