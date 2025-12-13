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

class SummonStmt : public Stmt
{

  public:
    SummonStmt(std::string name, std::unique_ptr<Expr> initializer, SourceLoc loc)
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
    BlockStmt(std::vector<std::unique_ptr<Stmt>> statements, SourceLoc loc)
        : statements(std::move(statements)), loc(loc)
    {
        kind = Block;
    };

  private:
    std::vector<std::unique_ptr<Stmt>> statements;
    SourceLoc                          loc;
};

class IfChainStmt : public Stmt
{
  public:
    IfChainStmt(std::vector<std::tuple<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> branches,
                std::unique_ptr<BlockStmt> elseBranch, SourceLoc loc)
        : branches(std::move(branches)), elseBranch(std::move(elseBranch)), loc(loc)
    {
        kind = IfChain;
    };

  private:
    std::vector<std::tuple<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> branches;
    std::optional<std::unique_ptr<BlockStmt>>                                                 elseBranch;
    SourceLoc                                                                  loc;
};

class WhileStmt : public Stmt
{
  public:
    WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<BlockStmt> body, SourceLoc loc)
        : condition(std::move(condition)), body(std::move(body)), loc(loc)
    {
        kind = While;
    };

  private:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<BlockStmt>             body;
    SourceLoc             loc;
};