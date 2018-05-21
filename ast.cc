#include <memory>

struct AST
{
    enum Type
    {
        INT,
        ID,
        BIN,
        BLOCK,
        IF,
        WHILE,
        FUNC,
        CALL,
        RETURN
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

struct WhileAST : public AST
{
    WhileAST(Pos pos, std::unique_ptr<AST> cond, std::unique_ptr<AST> body) : AST{WHILE, pos}, cond{std::move(cond)}, body{std::move(body)} {}

    const AST& getCond() const { return *cond; }
    const AST& getBody() const { return *body; }

private:
    std::unique_ptr<AST> cond, body;
};

struct FuncAST : public AST
{
    FuncAST(Pos pos, std::string name, std::unique_ptr<AST> body) : AST{FUNC, pos}, name{std::move(name)}, body{std::move(body)} {}

    const std::string& getName() const { return name; }
    const AST& getBody() const { return *body; }

private:
    std::string name;
    std::unique_ptr<AST> body;
};

struct CallAST : public AST
{
    CallAST(Pos pos, std::string funcName, std::vector<std::unique_ptr<AST>> args): AST{CALL, pos}, funcName{std::move(funcName)}, args{std::move(args)} {}

    const std::string& getFuncName() const { return funcName; }
    const std::vector<std::unique_ptr<AST>>& getArgs() const { return args; }

private:
    std::string funcName;
    std::vector<std::unique_ptr<AST>> args;
};

struct ReturnAST : public AST
{
    ReturnAST(Pos pos, std::unique_ptr<AST> value) : AST{RETURN, pos}, value{std::move(value)} {}

    const AST* getValue() const { return value.get(); }

private:
    std::unique_ptr<AST> value;
};
