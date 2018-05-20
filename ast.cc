#include <memory>

struct AST
{
    enum Type
    {
        INT,
        ID,
        BIN,
        BLOCK,
        IF
    };

    AST(Type type, Pos pos) : type{type}, pos{pos} {}
    virtual ~AST() {}

    Pos getPos() const { return pos; }
    Type getType() const { return type; }

private:
    Type type;
    Pos pos;
};

struct IntAST : public AST
{
    IntAST(Pos pos, int value) : AST{INT, pos}, value{value} {}

    int getValue() const { return value; }

private:
    int value;
};

struct IdAST : public AST
{
    IdAST(Pos pos, std::string name) : AST{ID, pos}, name{name} {}

    const std::string& getName() const { return name; }

private:
    std::string name;
};

struct BinAST : public AST
{
    BinAST(Pos pos, std::unique_ptr<AST> lhs, std::unique_ptr<AST> rhs, int op) : AST{BIN, pos}, lhs{std::move(lhs)}, rhs{std::move(rhs)}, op{op} {}

    const AST& getLhs() const { return *lhs; }
    const AST& getRhs() const { return *rhs; }
    int getOp() const { return op; }

private:
    std::unique_ptr<AST> lhs, rhs;
    int op;
};

struct BlockAST : public AST
{
    BlockAST(Pos pos, std::vector<std::unique_ptr<AST>> asts) : AST{BLOCK, pos}, asts{std::move(asts)} {}

    const std::vector<std::unique_ptr<AST>>& getAsts() const { return asts; }

private:
    std::vector<std::unique_ptr<AST>> asts;
};

struct IfAST : public AST
{
    IfAST(Pos pos, std::unique_ptr<AST> cond, std::unique_ptr<AST> body, std::unique_ptr<AST> alt) : AST{IF, pos}, cond{std::move(cond)}, body{std::move(body)}, alt{std::move(alt)} {}

    const AST& getCond() const { return *cond; }
    const AST& getBody() const { return *body; }
    const AST* getAlt() const { return alt.get(); }

private:
    std::unique_ptr<AST> cond, body, alt;
};
