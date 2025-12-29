/**
 * @file prog.h
 * @brief Program representation for the parsed AST.
 *
 * The Program class represents a complete parsed program, containing
 * the root-level statements and metadata about the parse result.
 */
#pragma once

#include "ast/expr.h"
#include "ast/stmt.h"

#include <memory>
#include <vector>

/**
 * @brief Represents a complete parsed program.
 *
 * A Program consists of a sequence of statements at the top level,
 * along with error tracking and source location information. It serves
 * as the root of the entire AST.
 */
class Program
{
  public:
    /**
     * @brief Constructs a program from a sequence of statements.
     * @param statements The top-level statements in the program
     * @param hasError Whether a parse error was encountered
     * @param startLoc Source location of the program start
     * @param endLoc Source location of the program end
     */
    Program(std::vector<std::unique_ptr<Stmt>> statements, bool hasError, SourceLoc startLoc,
            SourceLoc endLoc)
        : statements(std::move(statements)), hasError(hasError), startLoc(startLoc),
          endLoc(endLoc) {};

    bool hadError() const
    {
        return hasError;
    }

    /**
     * @brief Returns the vector of top-level statements.
     * @return Const reference to the statements vector
     */
    const std::vector<std::unique_ptr<Stmt>>& getStatements() const
    {
        return statements;
    }

    /**
     * @brief Check whether the program contains any statements.
     * @return true if the program is empty, false otherwise
     */
    bool isEmpty() const
    {
        return statements.size() == 0;
    }

    /**
     * @brief Returns the number of top-level statements.
     * @return The count of statements in the program
     */
    int size() const
    {
        return statements.size();
    }

    /**
     * @brief Compares two Program nodes for equality.
     * @param other The other program to compare with
     * @return True if both have the same statements, error state, and source locations
     */
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

    /**
     * @brief Return a human-readable representation of this program.
     * @return String representation of this program with all statements
     */
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

    /// Const iterator type for iterating over statements
    using StmtIterator = std::vector<std::unique_ptr<Stmt>>::const_iterator;

    /**
     * @brief Returns an iterator to the beginning of the statements.
     * @return Iterator to the first statement
     */
    StmtIterator begin() const
    {
        return statements.begin();
    }

    /**
     * @brief Returns an iterator to the end of the statements.
     * @return Iterator past the last statement
     */
    StmtIterator end() const
    {
        return statements.end();
    }

    /**
     * @brief Returns a const iterator to the beginning of the statements.
     * @return Const iterator to the first statement
     */
    StmtIterator cbegin() const
    {
        return statements.cbegin();
    }

    /**
     * @brief Returns a const iterator to the end of the statements.
     * @return Const iterator past the last statement
     */
    StmtIterator cend() const
    {
        return statements.cend();
    }

  private:
    std::vector<std::unique_ptr<Stmt>> statements; ///< The top-level statements
    SourceLoc                          startLoc;   ///< Source location of program start
    SourceLoc                          endLoc;     ///< Source location of program end
    bool                               hasError;   ///< Whether a parse error occurred
};