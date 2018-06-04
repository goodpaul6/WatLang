#include <cassert>

#include "typer.h"

// To Apaar in the morning or whenever
// I was setting this up so that it caches
// types in the AST directly for later reference.
// While it's doing that, it also does all the type checking
// it needs to. This way we can use type information
// easily in the compiler.
struct Typer
{
    void resolveTypes(SymbolTable& table, AST& ast)
    {
        switch(ast.getType()) {
            case AST::BOOL: {
                ast.tag = table.getPrimTag(Typetag::BOOL);
            } break;

            case AST::CHAR: {
                ast.tag = table.getPrimTag(Typetag::CHAR);
            } break;

            case AST::INT: {
                ast.tag = table.getPrimTag(Typetag::INT);
            } break;

            case AST::STR: {
                ast.tag = table.getPtrTag(table.getPrimTag(Typetag::CHAR));
            } break;

            case AST::ID: {
                auto var = table.getVar(static_cast<IdAST&>(ast).getName(), curFunc);

                if(!var) {
                    throw PosError{ast.getPos(), "Referenced non-existent variable " + static_cast<IdAST&>(ast).getName()};
                }

                ast.tag = var->typetag;
            } break;

            case AST::CAST: {
                ast.tag = static_cast<CastAST&>(ast).getTargetType();
            } break;

            case AST::ARRAY: {
                ast.tag = table.getPtrTag(table.getPrimTag(Typetag::INT));
            } break;

            case AST::ARRAY_STRING: {
                ast.tag = table.getPtrTag(table.getPrimTag(Typetag::CHAR));
            } break;

            case AST::BIN: {
                auto& bst = static_cast<BinAST&>(ast);

                resolveTypes(table, bst.getLhs());
                resolveTypes(table, bst.getRhs());

                auto lhsType = bst.getLhs().tag;
                auto rhsType = bst.getRhs().tag;

                switch(bst.getOp()) {
                    case '+': case '-': case '*': case '/': {
                        if(lhsType->tag == Typetag::PTR) {
                            if(rhsType->tag != Typetag::INT && rhsType->tag != Typetag::PTR) {
                        throw PosError{ast.getPos(), "Attempted to perform binary operation on pointer with a " + static_cast<std::string>(*rhsType)};
                            }
                        } else if(lhsType->tag == Typetag::INT || lhsType->tag == Typetag::CHAR) {
                            if(rhsType->tag != Typetag::INT && rhsType->tag != Typetag::CHAR) {
                                throw PosError{ast.getPos(), "Attempted to perform binary operation between a " + static_cast<std::string>(*lhsType) + " and a " + static_cast<std::string>(*rhsType)};
                            }
                        }

                        bst.tag = lhsType;
                        return;
                    } break;
                    
                    case '=': {
                        // We allow assigning *void to any type of pointer
                        if(lhsType != rhsType && !(lhsType->tag == Typetag::PTR && rhsType->tag == Typetag::PTR && rhsType->inner->tag == Typetag::VOID)) {
                            throw PosError{bst.getPos(), "Mismatched types in assignment statement: " + static_cast<std::string>(*lhsType) + " " + static_cast<std::string>(*rhsType)};
                        }

                        return;
                    } break;

                    default:
                        // Relational operators
                        // TODO(Apaar): Typecheck these
                        ast.tag = table.getPrimTag(Typetag::BOOL);
                        return;
                }

                throw PosError{ast.getPos(), "Invalid types in binary operation: " + static_cast<std::string>(*lhsType) + " " + static_cast<std::string>(*rhsType)};
            } break;

            case AST::PAREN: {
                resolveTypes(table, static_cast<ParenAST&>(ast).getInner());

                ast.tag = static_cast<ParenAST&>(ast).getInner().tag;
            } break;

            case AST::UNARY: {
                auto& ust = static_cast<UnaryAST&>(ast);

                resolveTypes(table, ust.getRhs());
                auto rhsType = ust.getRhs().tag;

                if(ust.getOp() == '*') {
                    if(rhsType->tag != Typetag::PTR) {
                        throw PosError{ast.getPos(),
                            "Attempted to dereference a " + static_cast<std::string>(*rhsType)};
                    }

                    ast.tag = rhsType->inner;
                    return;
                }

                if(ust.getOp() == '-') {
                    if(rhsType->tag != Typetag::INT) {
                        throw PosError{ast.getPos(),
                            "Attempted to negate a " + static_cast<std::string>(*rhsType)};
                    }

                    ast.tag = rhsType; 
                    return;
                }

                if(ust.getOp() == '!') {
                    if(rhsType->tag != Typetag::BOOL) {
                        throw PosError{ast.getPos(),
                            "Attempted to not a " + static_cast<std::string>(*rhsType)};
                    }

                    ast.tag = table.getPrimTag(Typetag::BOOL);
                    return;
                }

                assert(0);
            } break;

            case AST::BLOCK: {
                for(auto& a : static_cast<BlockAST&>(ast).getAsts()) {
                    resolveTypes(table, *a);
                }
            } break;

            case AST::FUNC: {
                auto& fst = static_cast<FuncAST&>(ast);

                curFunc = table.getFunc(fst.getName());
                resolveTypes(table, fst.getBody());
                curFunc = nullptr;
            } break;

            case AST::RETURN: {
                auto& rst = static_cast<ReturnAST&>(ast);

                if(!curFunc) {
                    throw PosError{ast.getPos(), "What are you returning from???"};
                }

                if(rst.getValue()) {
                    resolveTypes(table, *rst.getValue());

                    auto valueType = rst.getValue()->tag;

                    if(rst.getValue()->tag != curFunc->returnType) {
                        throw PosError{ast.getPos(), "Returned value is of type " + static_cast<std::string>(*valueType) + " but the surrounding function returns " +
                            static_cast<std::string>(*curFunc->returnType)};
                    }
                } else {
                    if(curFunc->returnType->tag != Typetag::VOID) {
                        throw PosError{ast.getPos(), "Attempted to return nothing from a function which returns " + static_cast<std::string>(*curFunc->returnType)};
                    }
                }
            } break;

            case AST::IF: {
                auto& ist = static_cast<IfAST&>(ast);
                
                resolveTypes(table, ist.getCond());

                auto condType = ist.getCond().tag;
                
                if(condType->tag != Typetag::BOOL) {
                    throw PosError{ist.getCond().getPos(), "If condition is not boolean."};
                }

                resolveTypes(table, ist.getBody());
                
                if(ist.getAlt()) {
                    resolveTypes(table, *ist.getAlt());
                }
            } break;

            case AST::WHILE: {
                auto& wst = static_cast<WhileAST&>(ast);

                resolveTypes(table, wst.getCond());

                auto condType = wst.getCond().tag;

                if(condType->tag != Typetag::BOOL) {
                    throw PosError{wst.getCond().getPos(), "While condition is not boolean."};
                }

                resolveTypes(table, wst.getBody());
            } break;

            case AST::CALL: {
                auto& cst = static_cast<CallAST&>(ast);

                Func* func = table.getFunc(cst.getFuncName());

                if(!func) {
                    throw PosError{cst.getPos(), "Attempted to call non-existent function " + cst.getFuncName()};
                }

                if(cst.getArgs().size() != func->args.size()) {
                    throw PosError{cst.getPos(), cst.getFuncName() + " expected " + std::to_string(func->args.size()) + " arguments but you supplied " + std::to_string(cst.getArgs().size())};
                }

                for(auto i = 0u; i < cst.getArgs().size(); ++i) {
                    resolveTypes(table, *cst.getArgs()[i]);
                    auto suppliedType = cst.getArgs()[i]->tag;

                    if(suppliedType != func->args[i].typetag &&
                       (suppliedType->tag != Typetag::PTR && func->args[i].typetag->tag != Typetag::PTR && func->args[i].typetag->inner->tag != Typetag::VOID)) {
                        throw PosError{cst.getArgs()[i]->getPos(), "Argument " + std::to_string(i + 1) + " to " + func->name + " is supposed to be a " + static_cast<std::string>(*func->args[i].typetag) + " but you supplied a " + static_cast<std::string>(*suppliedType)};
                    }
                }

                ast.tag = func->returnType;
            } break;

            default: break;
        }
    }

private:
    Func* curFunc = nullptr;
};
