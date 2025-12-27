#include "ast/expr.h"

#include <memory>
#include <string>
#include <unordered_map>

struct Scope;
struct Diagnostic
{
    std::string message;
    SourceLoc   loc;

    std::string toString() const;
};

struct Symbol
{
    std::string name;
    enum Kind
    {
        VARIABLE
    } kind;

    SourceLoc declLoc;
};

struct Scope
{

    std::unique_ptr<Scope>                                   parent;
    std::unordered_map<std::string, std::unique_ptr<Symbol>> table;

    std::vector<std::unique_ptr<Scope>> children;

    bool declare(std::string& name, std::unique_ptr<Symbol>);

    const Symbol* lookup(std::string& name) const;

    const Symbol* lookupLocal(std::string& name) const;
};

struct ResolutionTable
{
    std::unordered_map<IdentifierExpr*, Symbol*> mapping;
};

struct SemanticResult
{
    std::unique_ptr<Scope>  rootScope;
    std::vector<Diagnostic> diagnostic;
    ResolutionTable         resolutionTable;
    bool                    hadError() const;
};