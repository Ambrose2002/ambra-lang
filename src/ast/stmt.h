enum StmtKind {
    Summon,
    Say,
    Block,
    IfChain,
    While
};

class Stmt {
    public:
    StmtKind kind;
    virtual ~Stmt() {};
};


class SummonSmt: public Stmt {

};

class SayStmt: public Stmt {

};

class BlockStmt: public Stmt {

};

class IfChainStmt: public Stmt {

};

class WhileStmt: public Stmt {

};