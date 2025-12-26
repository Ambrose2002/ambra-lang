#include "ast/expr.h"
#include "ast/stmt.h"

#include <memory>
#include <vector>
class Program
{
  public:
    Program(std::vector<std::unique_ptr<Stmt>> statements, bool hasError)
        : statements(statements), hasError(hasError) {};

    bool hadError()
    {
        return hasError;
    }

    std::vector<std::unique_ptr<Stmt>> getStatements() const
    {
        return statements;
    }

    bool isEmpty() const
    {
        return statements.size() == 0;
    }

    bool operator==(Program other) {

    }

    std::string toString() {
        
    }

  private:
    std::vector<std::unique_ptr<Stmt>> statements;
    SourceLoc                          startLoc;
    SourceLoc                          endLoc;
    bool                               hasError;
};