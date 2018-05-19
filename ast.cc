#include <memory>

struct AST
{
    enum Type
    {
        INT,
        ID,
        BIN
    };

    AST(Type type, Pos pos) : type{type}, pos{pos} {}

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
    IdAST(Pos pos, const std::string& name) : AST{ID, pos}, name{name} {}

    const std::string& getName() const { return name; }

private:
    std::string name;
};

struct BinAST : public AST
{
    BinAST(Pos pos, std::unique_ptr<AST> lhs, std::unique_ptr<AST> rhs, int op) : AST{BIN, pos}, lhs{std::move(lhs)}, rhs{std::move(rhs)}, op{op} {}

    AST& getLhs() { return *lhs; }
    AST& getRhs() { return *rhs; }
    int getOp() const { return op; }

private:
    std::unique_ptr<AST> lhs, rhs;
    int op;
};
