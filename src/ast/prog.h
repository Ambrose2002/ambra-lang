#include "ast/expr.h"
#include "ast/stmt.h"

#include <memory>
#include <vector>
class Program
{
  public:
    Program(std::vector<std::unique_ptr<Stmt>> statements, bool hasError, SourceLoc startLoc,
            SourceLoc endLoc)
        : statements(statements), hasError(hasError), startLoc(startLoc), endLoc(endLoc) {};

    bool hadError() const
    {
        return hasError;
    }

    const std::vector<std::unique_ptr<Stmt>>& getStatements() const
    {
        return statements;
    }

    bool isEmpty() const
    {
        return statements.size() == 0;
    }

    int size() const
    {
        return statements.size();
    }

    bool operator==(const Program& other) const
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
        return hasError == other.hasError && startLoc == other.startLoc && endLoc == other.endLoc;
    }

    std::string toString() const
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

    using StmtIterator = std::vector<std::unique_ptr<Stmt>>::const_iterator;

    StmtIterator begin() const
    {
        return statements.begin();
    }

    StmtIterator end() const
    {
        return statements.end();
    }

    StmtIterator cbegin() const
    {
        return statements.cbegin();
    }

    StmtIterator cend() const
    {
        return statements.cend();
    }

  private:
    std::vector<std::unique_ptr<Stmt>> statements;
    SourceLoc                          startLoc;
    SourceLoc                          endLoc;
    bool                               hasError;
};