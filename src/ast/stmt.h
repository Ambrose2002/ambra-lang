#include "expr.h"

#include <memory>
#include <string>
#include <tuple>
#include <utility>
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
        kind = Summon;
    }

  private:
    std::string           name;
    std::unique_ptr<Expr> initializer;
    SourceLoc             loc;
};

class SayStmt : public Stmt
{
  public:
    SayStmt(std::unique_ptr<Expr> expression, SourceLoc loc)
        : expression(std::move(expression)), loc(loc)
    {
        kind = Say;
    };

  private:
    std::unique_ptr<Expr> expression;
    SourceLoc             loc;
};

class BlockStmt : public Stmt
{
  public:
    BlockStmt(std::vector<Stmt> statements, SourceLoc loc) : statements(statements), loc(loc)
    {
        kind = Block;
    };

  private:
    std::vector<Stmt> statements;
    SourceLoc         loc;
};

class IfChainStmt : public Stmt
{
    public:
    IfChainStmt(std::vector<std::tuple<Expr, BlockStmt>> branches, BlockStmt elseBranch, SourceLoc loc): branches(branches), elseBranch(elseBranch), loc(loc){
        kind = IfChain;
    };
    private:
    std::vector<std::tuple<Expr, BlockStmt>> branches;
    BlockStmt elseBranch;
    SourceLoc loc;
};

class WhileStmt : public Stmt
{
  public:
    WhileStmt(std::unique_ptr<Expr> condition, BlockStmt body, SourceLoc loc)
        : condition(std::move(condition)), body(body), loc(loc)
    {
        kind = While;
    };

  private:
    std::unique_ptr<Expr> condition;
    BlockStmt             body;
    SourceLoc             loc;
};