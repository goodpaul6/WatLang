#include <memory>
#include <cassert>

struct Typetag
{
    enum Tag
    {
        VOID,
        BOOL,
        CHAR,
        INT,
        PTR
    } tag;

    std::unique_ptr<Typetag> inner;

    Typetag() = default;
    Typetag(Tag tag) : tag{tag} {}
    Typetag(const Typetag& other) :
        tag{other.tag},
        inner{other.inner ? new Typetag{*other.inner} : nullptr}
    {
    }

    operator std::string() const
    {
        switch(tag) {
            case VOID: return "void";
            case BOOL: return "bool";
            case CHAR: return "char";
            case INT: return "int";
            case PTR: return "*" + static_cast<std::string>(*inner);
            default: assert(0); return "ERROR"; break;
        }
    }
};

bool operator==(const Typetag& a, const Typetag& b)
{
    if(a.tag != b.tag) {
        return false;
    }

    if(a.tag == Typetag::PTR) {
        return *a.inner == *b.inner;
    }

    return true;
}

bool operator!=(const Typetag& a, const Typetag& b)
{
    return !(a == b);
}

struct Typer
{
    void checkTypes(SymbolTable& table, const AST& ast)
    {
        switch(ast.getType()) {
            case AST::BLOCK: {
                for(auto& a : static_cast<const BlockAST&>(ast).getAsts()) {
                    checkTypes(table, *a);
                }
            } break;

            case AST::FUNC: {
                auto& fst = static_cast<const FuncAST&>(ast);

                curFunc = table.getFunc(fst.getName());
                checkTypes(table, fst.getBody());
                curFunc = nullptr;
            } break;

            case AST::RETURN: {
                auto& rst = static_cast<const ReturnAST&>(ast);

                if(!curFunc) {
                    throw PosError{ast.getPos(), "What are you returning from???"};
                }

                if(rst.getValue()) {
                    auto valueType = inferType(table, *rst.getValue());

                    if(*valueType != *curFunc->returnType) {
                        throw PosError{ast.getPos(), "Returned value is of type " + static_cast<std::string>(*valueType) + " but the surrounding function returns " +
                            static_cast<std::string>(*curFunc->returnType)};
                    }
                } else {
                    if(curFunc->returnType->tag != Typetag::VOID) {
                        throw PosError{ast.getPos(), "Attempted to return nothing from a function which returns " + static_cast<std::string>(*curFunc->returnType)};
                    }
                }
            } break;

            case AST::BIN: {
                auto& bst = static_cast<const BinAST&>(ast);
                
                assert(bst.getOp() == '=');

                auto lhsType = inferType(table, bst.getLhs());
                auto rhsType = inferType(table, bst.getRhs());

                if(*lhsType != *rhsType) {
                    throw PosError{bst.getPos(), "Mismatched types in assignment statement: " + static_cast<std::string>(*lhsType) + " " + static_cast<std::string>(*rhsType)};
                }
            } break;
            
            case AST::IF: {
                auto& ist = static_cast<const IfAST&>(ast);

                auto condType = inferType(table, ist.getCond());
                
                if(condType->tag != Typetag::BOOL) {
                    throw PosError{ist.getCond().getPos(), "If condition is not boolean."};
                }

                checkTypes(table, ist.getBody());
                
                if(ist.getAlt()) {
                    checkTypes(table, *ist.getAlt());
                }
            } break;

            case AST::WHILE: {
                auto& wst = static_cast<const WhileAST&>(ast);

                auto condType = inferType(table, wst.getCond());

                if(condType->tag != Typetag::BOOL) {
                    throw PosError{wst.getCond().getPos(), "While condition is not boolean."};
                }

                checkTypes(table, wst.getBody());
            } break;

            case AST::CALL: {
                auto& cst = static_cast<const CallAST&>(ast);

                Func* func = table.getFunc(cst.getFuncName());

                if(!func) {
                    throw PosError{cst.getPos(), "Attempted to call non-existent function " + cst.getFuncName()};
                }

                if(cst.getArgs().size() != func->args.size()) {
                    throw PosError{cst.getPos(), cst.getFuncName() + " expected " + std::to_string(func->args.size()) + " arguments but you supplied " + std::to_string(cst.getArgs().size())};
                }

                for(auto i = 0u; i < cst.getArgs().size(); ++i) {
                    auto suppliedType = inferType(table, *cst.getArgs()[i]);

                    if(*suppliedType != *func->args[i].typetag) {
                        throw PosError{cst.getArgs()[i]->getPos(), "Argument " + std::to_string(i + 1) + " to " + func->name + " is supposed to be a " + static_cast<std::string>(*func->args[i].typetag) + " but you supplied a " + static_cast<std::string>(*suppliedType)};
                    }
                }
            } break;

            default: break;
        }
    }

private:
    Func* curFunc = nullptr;

    std::unique_ptr<Typetag> inferType(SymbolTable& table, const AST& ast)
    {
        switch(ast.getType()) {
            case AST::INT: return std::unique_ptr<Typetag>{new Typetag{Typetag::INT}};
            case AST::BOOL: return std::unique_ptr<Typetag>{new Typetag{Typetag::BOOL}};
            case AST::CHAR: return std::unique_ptr<Typetag>{new Typetag{Typetag::CHAR}};
            case AST::STR: { 
                auto type = std::make_unique<Typetag>(Typetag::PTR);
                type->inner = std::make_unique<Typetag>(Typetag::CHAR);

                return type;
            } break;

            case AST::ID: {
                auto var = table.getVar(static_cast<const IdAST&>(ast).getName(), curFunc);

                if(!var) {
                    throw PosError{ast.getPos(), "Referenced non-existent variable " + static_cast<const IdAST&>(ast).getName()};
                }

                return std::unique_ptr<Typetag>{new Typetag{*var->typetag}};
            } break;

            case AST::CAST: {
                return std::unique_ptr<Typetag>{new Typetag{static_cast<const CastAST&>(ast).getTargetType()}};
            } break;
            
            case AST::BIN: {
                auto& bst = static_cast<const BinAST&>(ast);

                auto lhsType = inferType(table, bst.getLhs());
                auto rhsType = inferType(table, bst.getRhs());

                if(lhsType->tag == Typetag::PTR) {
                    if(rhsType->tag != Typetag::INT && rhsType->tag != Typetag::PTR) {
                        throw PosError{ast.getPos(), "Attempted to perform binary operation on pointer with a " + static_cast<std::string>(*rhsType)};
                    }

                    return std::unique_ptr<Typetag>{new Typetag{*lhsType}};
                }

                if(lhsType->tag == Typetag::INT || lhsType->tag == Typetag::CHAR) {
                    if(bst.getOp() == '+' || bst.getOp() == '-' || bst.getOp() == '*' || bst.getOp() == '/') {
                        if(rhsType->tag != Typetag::INT && rhsType->tag != Typetag::PTR) {
                            throw PosError{ast.getPos(), "Attempted to apply binary operation to " + static_cast<std::string>(*rhsType) + " and an integer."};
                        }

                        return rhsType->tag == Typetag::PTR ? 
                            std::unique_ptr<Typetag>{new Typetag{*rhsType}} : 
                            std::unique_ptr<Typetag>{new Typetag{*lhsType}};
                    } else {
                        // Relational/Logical operators
                        return std::unique_ptr<Typetag>{new Typetag{Typetag::BOOL}};
                    }
                }

                throw PosError{ast.getPos(), "Invalid types in binary operation: " + static_cast<std::string>(*lhsType) + " " + static_cast<std::string>(*rhsType)};
            } break;

            case AST::PAREN: {
                return inferType(table, static_cast<const ParenAST&>(ast).getInner());
            } break;

            case AST::UNARY: {
                auto& ust = static_cast<const UnaryAST&>(ast);

                auto rhsType = inferType(table, ust.getRhs());

                if(ust.getOp() == '*') {
                    if(rhsType->tag != Typetag::PTR) {
                        throw PosError{ast.getPos(),
                            "Attempted to dereference a " + static_cast<std::string>(*rhsType)};
                    }

                    return std::unique_ptr<Typetag>{new Typetag{*rhsType->inner}};
                }

                if(ust.getOp() == '-') {
                    if(rhsType->tag != Typetag::INT) {
                        throw PosError{ast.getPos(),
                            "Attempted to negate a " + static_cast<std::string>(*rhsType)};
                    }

                    return std::unique_ptr<Typetag>{new Typetag{*rhsType}};
                }

                if(ust.getOp() == '!') {
                    if(rhsType->tag != Typetag::BOOL) {
                        throw PosError{ast.getPos(),
                            "Attempted to not a " + static_cast<std::string>(*rhsType)};
                    }
                }

                assert(0);
            } break;

            case AST::CALL: {
                checkTypes(table, ast);

                auto func = table.getFunc(static_cast<const CallAST&>(ast).getFuncName());

                // checkTypes should make sure this exists
                assert(func);

                return std::unique_ptr<Typetag>{new Typetag{*func->returnType}};
            } break;

            default: assert(0); break;
        }

        return nullptr;
    }
};
