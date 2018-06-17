#include <ostream>
#include <cassert>
#include <unordered_map>
#include <algorithm>

struct Compiler
{
    void compile(SymbolTable& table, std::vector<std::unique_ptr<AST>>& asts, Codegen& gen)
    {
        if(!table.getFunc("main")) {
            throw std::runtime_error{"Missing main function."};
        }

        // Make sure that all structs are defined
        for(auto& tag : table.tags) {
            if(tag->tag == Typetag::STRUCT && tag->structFields.empty()) {
                throw PosError{tag->structDeclPos, "Referenced undefined struct " + tag->structName};
            }
        }

        resolveSymbolLocations(table, gen);

        for(auto& ast : asts) {
            compileStatement(table, *ast, gen);
        }

        // This is used by the default allocator in the runtime
        // to determine where it can start allocating memory
        gen.labelHere("memStartXXXX");
    }

private:
    struct TempStorage
    {
        enum Type { REG, STACK } type;

        union
        {
            int reg;
            struct
            {
                int ptrReg;
                int sizeInBytes;
            } stack;
        };

        TempStorage() = default;

        explicit TempStorage(int reg) : type{REG} { this->reg = reg; }
        explicit TempStorage(int ptrReg, int sizeInBytes) :
            type{STACK}
        {
            stack.ptrReg = ptrReg;
            stack.sizeInBytes = sizeInBytes;
        }
    };

    int curReg = 1;
    int labelIndex = 0;

    // The function we are compiling rn
    Func* curFunc = nullptr;

    // This register stores the PC of the calling function
    const int LINK_REG = 31;

    // Stack pointer
    const int STACK_REG = 30;

    // This is the base pointer register
    const int BASE_REG = 29;

    // This register is set to a memory location on the stack large enough
    // to accomodate the return value
    const int RETVAL_REG = 28;

    std::string uniqueLabel()
    {
        return "L" + std::to_string(labelIndex++);
    }

    // Makes room for symbols and sets their location values
    void resolveSymbolLocations(SymbolTable& table, Codegen& gen)
    {
        // We keep track of return-to-os address
        gen.lis(29, "exitAddrGlobalXXXX");
        gen.sw(LINK_REG, 0, 29);

        gen.lis(29, "main");
        gen.jr(29);

        for(auto& v : table.globals) {
            v.loc = gen.getPos();
            gen.word(0);
        }

        for(auto& s : table.strings) {
            s.loc = gen.getPos();
            for(auto ch : s.str) {
                gen.word(ch);
            }

            // null-terminator
            gen.word(0);
        }

        gen.labelHere("exitAddrGlobalXXXX");
        gen.word(0);

        for(auto& f : table.funcs) {
            // Since the stack grows downwards these values are located positively relative to the base pointer
            auto spaceUsed = 0;
            auto cur = 0;

            for(auto& v : f.locals) {
                spaceUsed += v.typetag->getSizeInBytes();
            }

            cur = spaceUsed;

            for(auto& v : f.locals) {
                cur -= v.typetag->getSizeInBytes();
                v.loc = cur;
            }

            // Args are higher than locals in the stack because
            // they're pushed before the function is entered
            for(auto& v : f.args) {
                spaceUsed += v.typetag->getSizeInBytes();
            }

            cur = spaceUsed;

            for(auto& v : f.args) { 
                cur -= v.typetag->getSizeInBytes();
                v.loc = cur;
            }
        }
    }

    bool compileCall(SymbolTable& table, CallAST& ast, Codegen& gen, TempStorage& ret)
    {
        auto func = table.getFunc(ast.getFuncName());

        if(!func) {
            throw PosError{ast.getPos(), "Attempted to call undeclared function " + ast.getFuncName()};
        }

        if(ast.getArgs().size() != func->args.size()) {
            throw PosError{ast.getPos(), "Incorrect amount of arguments supplied to " + ast.getFuncName() + "; expected " + std::to_string(func->args.size())};
        }

        bool hasReturn = func->returnType->tag != Typetag::VOID;

        int temp = curReg++;

        // TODO(Apaar): If the return value is 4 bytes wide, just store it in the retval
        // register; don't bother allocating stack space

        if(hasReturn) {
            // Make room for the return value
            gen.lis(temp, func->returnType->getSizeInBytes());
            gen.sub(STACK_REG, STACK_REG, temp);
            gen.add(RETVAL_REG, STACK_REG, 0);
        }

        curReg = temp;

        // Store all current registers (required for correctness of expressions e.g. 1 + call())

        int numRegsStored = curReg;

        for(int i = 1; i < numRegsStored; ++i) {
            gen.sw(i, -i * 4, STACK_REG);
        }

        curReg = temp + 1;

        gen.lis(temp, numRegsStored * 4);
        gen.sub(STACK_REG, STACK_REG, temp);
    
        auto sizeSoFar = 0;
        
        for(auto& arg : ast.getArgs()) {
            auto val = compileTerm(table, *arg, gen);

            sizeSoFar += arg->getTag()->getSizeInBytes();

            // We only need to push the arg on the stack if its not already there
            if(val.type == TempStorage::REG) {
                gen.sw(val.reg, -sizeSoFar, STACK_REG); 

                // Since we push the arg to the stack, we can stomp the reg
                curReg = val.reg;
            } else {
                curReg = val.stack.ptrReg;
            }
        }

        gen.lis(temp, sizeSoFar);
        gen.sub(STACK_REG, STACK_REG, temp);

        // Args are loaded onto the stack, do the call
        gen.lis(temp, func->name);
        gen.jalr(temp);

        // If it returned anything, the pointer to it is in RETVAL_REG
        if(hasReturn) {
            ret.stack.ptrReg = RETVAL_REG;
            ret.stack.sizeInBytes = func->returnType->getSizeInBytes();
        }

        // Pop args
        gen.lis(temp, sizeSoFar);
        gen.add(STACK_REG, STACK_REG, temp);

        // Restore regs
        gen.lis(temp, numRegsStored * 4);
        gen.add(STACK_REG, STACK_REG, temp); 

        for(int i = 1; i < numRegsStored; ++i) {
            gen.lw(i, -i * 4, STACK_REG);
        }

        curReg = temp;

        return hasReturn;
    }

    TempStorage compileTerm(SymbolTable& table, AST& ast, Codegen& gen)
    {
        if(ast.getType() == AST::INT || ast.getType() == AST::BOOL || ast.getType() == AST::CHAR) {
            gen.lis(curReg++, static_cast<int32_t>(static_cast<const IntAST&>(ast).getValue()));

            return TempStorage{curReg - 1};
        } else if(ast.getType() == AST::ARRAY || ast.getType() == AST::ARRAY_STRING) {
            auto startLabel = uniqueLabel();
            auto endLabel = uniqueLabel();

            gen.lis(curReg++, endLabel);
            gen.jr(curReg - 1);

            gen.labelHere(startLabel);

            auto& a = static_cast<const ArrayAST&>(ast);

            if(a.getLength() == 0) {
                throw PosError{ast.getPos(), "Size of array literal must be > 0."};
            }

            auto i = 0;
            for(auto value : a.getValues()) {
                gen.word(value);
                ++i;
            }

            for(auto j = i; j < a.getLength(); ++j) {
                gen.word(0);
            }

            gen.labelHere(endLabel);
            gen.lis(curReg - 1, startLabel);

            return TempStorage{curReg - 1};
        } else if(ast.getType() == AST::PAREN) {
            return compileTerm(table, static_cast<ParenAST&>(ast).getInner(), gen);
        } else if(ast.getType() == AST::ID) {
            auto idAst = static_cast<IdAST&>(ast);
            auto var = table.getVar(idAst.getName(), curFunc);

            if(!var) {
                throw PosError{ast.getPos(), "Referencing undeclared identifier " + idAst.getName()};
            }

            int baseReg = var->func ? BASE_REG : 0;
            int sizeInBytes = var->typetag->getSizeInBytes();

            if(sizeInBytes == 4) {
                gen.lw(curReg++, var->loc, baseReg);
                return TempStorage{curReg - 1};
            }

            int temp = curReg++;

            // Copy the struct onto the stack
            for(int i = 0; i < sizeInBytes; sizeInBytes += 4) {
                gen.lw(temp, var->loc + i, baseReg);
                gen.sw(temp, -i - 4, STACK_REG);
            }

            gen.lis(temp, sizeInBytes);
            gen.sub(STACK_REG, STACK_REG, temp);

            // This reg will be the pointer to the value
            gen.add(temp, STACK_REG, 0);

            return TempStorage{temp, sizeInBytes};
        } else if(ast.getType() == AST::CALL) {
            auto& cst = static_cast<CallAST&>(ast);

            TempStorage val;

            auto result = compileCall(table, cst, gen, val);

            // Typer should've made sure that this wasn't returning void
            assert(result);

            return val;
        } else if(ast.getType() == AST::STR) {
            gen.lis(curReg++, table.getString(static_cast<StrAST&>(ast).getId()).loc);
            return TempStorage{curReg - 1};
        } else if(ast.getType() == AST::UNARY) {
            auto val = compileTerm(table, static_cast<UnaryAST&>(ast).getRhs(), gen);

            // Typer should make sure this is something that can be negated (an int, which
            // fits in a reg) or dereferenced (a pointer, also fits in a reg)
            assert(val.type == TempStorage::REG);

            switch(static_cast<UnaryAST&>(ast).getOp()) {
                case '-': {
                    gen.sub(curReg++, 0, val.reg);
                    return TempStorage{curReg - 1};
                } break;

                case '*': {
                    gen.lw(curReg++, 0, val.reg);
                    return TempStorage{curReg - 1};
                } break;
            }
        } else if(ast.getType() == AST::CAST) {
            return compileTerm(table, static_cast<CastAST&>(ast).getValue(), gen);
        }

        assert(ast.getType() == AST::BIN);

        auto& bst = static_cast<BinAST&>(ast);

        int dest = curReg++;

        auto as = compileTerm(table, bst.getLhs(), gen);
        auto bs = compileTerm(table, bst.getRhs(), gen);

        // Similar to above, Typer should make sure lhs and rhs fit into regs
        assert(as.type == TempStorage::REG);
        assert(bs.type == TempStorage::REG);

        auto a = as.reg;
        auto b = bs.reg;

        switch(bst.getOp()) {
            case '+': gen.add(dest, a, b); break;
            case '-': gen.sub(dest, a, b); break;
            case '*': {
                gen.mult(a, b);
                gen.mflo(dest);
            } break;

            case '/': {
                gen.div(a, b);
                gen.mflo(dest);
            } break;

            case '%': {
                gen.div(a, b);
                gen.mfhi(dest);
            } break;

            case TOK_EQUALS: {
                gen.lis(dest, 1);

                // Set the result to 0 if they're not equal
                gen.beq(a, b, 1);
                gen.add(dest, 0, 0);
            } break;

            case TOK_NOTEQUALS: {
                gen.lis(dest, 1);

                // Set the result to 0 if they're equal
                gen.bne(a, b, 1);
                gen.add(dest, 0, 0);
            } break;

            case '<': {
                gen.slt(dest, a, b);
            } break;

            case '>': {
                gen.slt(dest, a, b);

                int temp = curReg++;
                gen.lis(temp, 1);

                gen.sub(dest, temp, dest);

                // dest reg=1 if greater than or equal to b

                // we skip setting the dest to 0 if they're not equal
                gen.bne(a, b, 1);
                gen.add(dest, 0, 0);
            } break;

            case TOK_LTE: {
                gen.slt(dest, a, b);

                // Set to 1 if they're equal
                gen.bne(a, b, 2);
                gen.lis(dest, 1);
            } break;

            case TOK_GTE: {
                gen.slt(dest, a, b);

                int temp = curReg++;
                gen.lis(temp, 1);

                gen.sub(dest, temp, dest);

                // dest reg=1 if greater than or equal to b
            } break;

            case TOK_LOGICAL_AND: {
                gen.lis(dest, 0);

                gen.beq(a, 0, 3);
                gen.beq(b, 0, 2);
                gen.lis(dest, 1);
            } break;

            case TOK_LOGICAL_OR: {
                gen.lis(dest, 1);

                gen.bne(a, 0, 3);
                gen.bne(b, 0, 2);
                gen.lis(dest, 0);
            } break;
        }

        curReg = dest + 1;

        return TempStorage{dest};
    }

    void compileRestoreLinkAndSp(Codegen& gen)
    {
        // Reset back to base pointer
        gen.add(STACK_REG, BASE_REG, 0);

        // The link reg and the previous base reg are stored after the base pointer, so we can retrieve them now
        gen.lw(LINK_REG, -4, STACK_REG);
        gen.lw(BASE_REG, -8, STACK_REG);
    }

    void compileStatement(SymbolTable& table, AST& ast, Codegen& gen)
    {
        if(ast.getType() == AST::BIN) {
            auto& bst = static_cast<BinAST&>(ast);

            auto val = compileTerm(table, bst.getRhs(), gen);

            auto& lhs = bst.getLhs();

            if(lhs.getType() == AST::ID) {
                auto& name = static_cast<IdAST&>(lhs).getName();

                auto var = table.getVar(name, curFunc);

                if(!var) {
                    throw PosError{ast.getPos(), "Attempted to reference undeclared identifier " + name};
                }

                int baseReg = var->func ? BASE_REG : 0;
                int sizeInBytes = var->typetag->getSizeInBytes();
  
                if(val.type == TempStorage::REG) {
                    gen.sw(val.reg, var->loc, baseReg);
                } else {
                    // Typer should make sure rhs has same size as lhs
                    assert(val.stack.sizeInBytes == sizeInBytes);

                    int temp = curReg++;

                    for(int i = 0; i < sizeInBytes; i += 4) {
                        gen.lw(temp, i, val.stack.ptrReg);
                        gen.sw(temp, var->loc + i, BASE_REG);
                    }

                    curReg = temp;
                }
            } else {
                assert(lhs.getType() == AST::UNARY);

                auto& ust = static_cast<UnaryAST&>(lhs);

                assert(ust.getOp() == '*');

                auto lval = compileTerm(table, ust.getRhs(), gen);

                // Pointers ought to fit into registers
                assert(lval.type == TempStorage::REG);

                int lreg = lval.reg;
                int sizeInBytes = ust.getRhs().getTag()->getSizeInBytes();

                if(val.type == TempStorage::REG) {
                    gen.sw(val.reg, 0, lreg);
                } else {
                    // Typer should make sure rhs has same size as lhs
                    assert(val.stack.sizeInBytes == sizeInBytes);

                    int temp = curReg++;

                    for(int i = 0; i < sizeInBytes; i += 4) {
                        gen.lw(temp, i, val.stack.ptrReg);
                        gen.sw(temp, i, lreg);
                    }

                    curReg = temp;
                }
            }

            // We're done with this register now that it's stored
            if(val.type == TempStorage::REG) {
                curReg = val.reg;
            } else {
                curReg = val.stack.ptrReg;
            }
        } else if(ast.getType() == AST::BLOCK) {
            for(auto& a : static_cast<BlockAST&>(ast).getAsts()) {
                compileStatement(table, *a, gen);
            }
        } else if(ast.getType() == AST::IF) {
            auto& ist = static_cast<IfAST&>(ast);

            int prevReg = curReg;

            auto condVal = compileTerm(table, ist.getCond(), gen);

            // Booleans must fit in a register
            assert(condVal.type == TempStorage::REG);

            auto cond = condVal.reg;

            auto altLabel = uniqueLabel();
            auto endLabel = uniqueLabel();

            gen.beq(cond, 0, altLabel);

            compileStatement(table, ist.getBody(), gen);

            int temp = curReg++;

            gen.lis(temp, endLabel);
            gen.jr(temp);

            curReg = prevReg;

            gen.labelHere(altLabel);
            if(ist.getAlt()) {
                compileStatement(table, *ist.getAlt(), gen);
            }

            gen.labelHere(endLabel);
        } else if(ast.getType() == AST::WHILE) {
            auto& ist = static_cast<WhileAST&>(ast);

            int prevReg = curReg;

            auto condLabel = uniqueLabel();

            gen.labelHere(condLabel);

            auto condVal = compileTerm(table, ist.getCond(), gen);
            
            // Booleans must fit in regs
            assert(condVal.type == TempStorage::REG);

            auto cond = condVal.reg;

            auto endLabel = uniqueLabel();

            gen.beq(cond, 0, endLabel);

            compileStatement(table, ist.getBody(), gen);

            int temp = curReg++;

            gen.lis(temp, condLabel);
            gen.jr(temp);

            curReg = prevReg;

            gen.labelHere(endLabel);
        } else if(ast.getType() == AST::FUNC) {
            auto& fst = static_cast<FuncAST&>(ast);

            curFunc = table.getFunc(fst.getName());

            assert(curFunc);

            gen.labelHere(curFunc->name);

            // Make sure that we've reset curReg
            assert(curReg == 1);

            // We can use all registers from 1 onwards
            curReg = 1;

            int temp = curReg++;

            auto localsSize = 0;

            for(auto& v : curFunc->locals) {
                localsSize += v.typetag->getSizeInBytes();
            }

            if(localsSize > 0) {
                // Make room for local variables
                gen.lis(temp, localsSize);
                gen.sub(STACK_REG, STACK_REG, temp);
            }
            
            // Push the link and base pointer regs (for nested func calls)
            gen.sw(LINK_REG, -4, STACK_REG);
            gen.sw(BASE_REG, -8, STACK_REG);
            
            // Set the base pointer to the stack pointer
            gen.add(BASE_REG, STACK_REG, 0);

            gen.lis(temp, 8);
            gen.sub(STACK_REG, STACK_REG, temp);

            compileStatement(table, fst.getBody(), gen);

			curReg = temp;

            assert(curReg == 1);

            compileRestoreLinkAndSp(gen);
            gen.jr(LINK_REG);

            curFunc = nullptr;
        } else if(ast.getType() == AST::CALL) {
            auto& cst = static_cast<CallAST&>(ast);

            TempStorage val;

            auto hasReturn = compileCall(table, cst, gen, val);

            if(hasReturn) {
                // Discard return value
                if(val.type == TempStorage::REG) {
                    curReg = curReg > val.reg ? val.reg : curReg;
                } else {
                    curReg = curReg > val.stack.ptrReg ? val.stack.ptrReg : curReg;
                }
            }
        } else if(ast.getType() == AST::RETURN) {
            if(!curFunc) {
                throw PosError{ast.getPos(), "You're trying to return but you're not inside a function. Think about that for a second."};
            }

            auto value = static_cast<ReturnAST&>(ast).getValue();

            if(value) {
                auto val = compileTerm(table, *value, gen);    

                // Copy the return value into the stack memory pointed at by RETVAL_REG
                if(val.type == TempStorage::REG) {
                    gen.sw(val.reg, 0, RETVAL_REG);
                } else {
                    int temp = curReg++;

                    for(int i = 0; i < val.stack.sizeInBytes; i += 4) {
                        gen.lw(temp, i, val.stack.ptrReg);
                        gen.sw(temp, i, RETVAL_REG);
                    }

                    curReg = temp;
                }
            }

            compileRestoreLinkAndSp(gen);
            gen.jr(LINK_REG);
        } else if(ast.getType() == AST::ASM) {
            gen.parse(ast.getPos(), static_cast<AsmAST&>(ast).getCode());
        } else {
            throw PosError{ast.getPos(), "Expected statement."};
        }
    }
};
