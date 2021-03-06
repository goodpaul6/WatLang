#include <memory>

struct AST
{
    enum Type
    {
        INT,
        BOOL,
        CHAR,
        STR,
        ID,
        BIN,
        BLOCK,
        IF,
        WHILE,
        FUNC,
        CALL,
        RETURN,
        ASM,
        UNARY,
        PAREN,
        CAST,
        ARRAY,
        ARRAY_STRING
    };

    AST(Type type, Pos pos) : type{type}, pos{pos} {}
    virtual ~AST() {}

    Pos getPos() const { return pos; }
    Type getType() const { return type; }

private:
    Type type;
    Pos pos;
};

// Used for all of INT, BOOL, CHAR
struct IntAST : public AST
{
    IntAST(Pos pos, int64_t value, Type type) : AST{type, pos}, value{value} {}

    int64_t getValue() const { return value; }

private:
    int64_t value;
};

struct StrAST : public AST
{
    StrAST(Pos pos, int id) : AST{STR, pos}, id{id} {}

    int getId() const { return id; }

private:
    int id;
    size_t length;
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

struct AsmAST : public AST
{
    AsmAST(Pos pos, std::string code) : AST{ASM, pos}, code{std::move(code)} {}

    const std::string& getCode() const { return code; }
private:
    std::string code;
};

struct UnaryAST : public AST
{
    UnaryAST(Pos pos, std::unique_ptr<AST> rhs, int op) : AST{UNARY, pos}, rhs{std::move(rhs)}, op{op} {}

    const AST& getRhs() const { return *rhs; }
    int getOp() const { return op; }

private:
    int op;
    std::unique_ptr<AST> rhs;
};

struct ParenAST : public AST
{
    ParenAST(Pos pos, std::unique_ptr<AST> inner) : AST{PAREN, pos}, inner{std::move(inner)} {}

    const AST& getInner() const { return *inner; }

private:
    std::unique_ptr<AST> inner;
};

struct CastAST : public AST
{
    CastAST(Pos pos, std::unique_ptr<AST> value, std::unique_ptr<Typetag> targetType) : AST{CAST, pos}, value{std::move(value)}, targetType{std::move(targetType)} {}

    const Typetag& getTargetType() const { return *targetType; }
    const AST& getValue() const { return *value; }

private:
    std::unique_ptr<AST> value;
    std::unique_ptr<Typetag> targetType;
};

// If the length value for the array ast is -1, it is determined based
// on the values
struct ArrayAST : public AST
{
    ArrayAST(Pos pos, int length, std::vector<int> values, Type type) : AST{type, pos}, length{length}, values{std::move(values)} {}

    const std::vector<int>& getValues() const { return values; }
    int getLength() const { return length; }

private:
    int length;
    std::vector<int> values;
};
