#include "expr.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

enum StmtKind
{
    Summon,
    Say,
    Block,
    IfChain,
    While
};

class Stmt
{
  public:
    StmtKind kind;
    virtual ~Stmt() {};
};

class SummonSmt : public Stmt
{

  public:
    SummonSmt(std::string name, std::unique_ptr<Expr> initializer, SourceLoc loc)
        : name(name), initializer(std::move(initializer)), loc(loc)
    {
    }

  private:
    std::string           name;
    std::unique_ptr<Expr> initializer;
    SourceLoc             loc;
};

class SayStmt : public Stmt
{
  private:
    std::unique_ptr<Expr> expression;
    SourceLoc             loc;
};

class BlockStmt : public Stmt
{
  private:
    std::vector<Stmt> statements;
    SourceLoc         loc;
};

class IfChainStmt : public Stmt
{
};

class WhileStmt : public Stmt
{
};