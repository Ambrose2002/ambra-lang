
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

class SourceLoc
{

  public:
    SourceLoc(int line, int col) : line(line), col(col) {};
    int line;
    int col;
};


class StringPart {

};
class Expr
{
};

class IntLiteralExpr: public Expr {

    private:
    int value;
    SourceLoc loc;
};