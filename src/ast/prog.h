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

    bool operator==(Program other)
    {
        if (statements.size() != other.statements.size())
        {
            return false;
        }
        for (size_t i = 0; i < statements.size(); ++i)
        {
            if (!(*statements[i] == *other.statements[i]))
            {
                return false;
            }
        }
        return hasError == other.hasError;
    }

    std::string toString()
    {
        std::string result = "Program(\n";
        for (size_t i = 0; i < statements.size(); ++i)
        {
            result += "  " + statements[i]->toString();
            if (i < statements.size() - 1)
            {
                result += ",";
            }
            result += "\n";
        }
        result += ")";
        return result;
    }

  private:
    std::vector<std::unique_ptr<Stmt>> statements;
    SourceLoc                          startLoc;
    SourceLoc                          endLoc;
    bool                               hasError;
};