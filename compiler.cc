#include <ostream>
#include <cassert>
#include <unordered_map>
#include <algorithm>

struct Compiler
{
    void compile(SymbolTable& table, const std::vector<std::unique_ptr<AST>>& asts, Codegen& gen)
    {
        if(!table.getFunc("main")) {
            throw std::runtime_error{"Missing main function."};
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
    int curReg = 1;
    int labelIndex = 0;

    // The function we are compiling rn
    Func* curFunc = nullptr;

    // TODO(Apaar): Actually, we probably don't always want to put retval in a single register
    // because you could have multiple calls in a single statement. We'll return the register
    // into which the result of the function call is returned. Get rid of this once that's set up
    const int RETVAL_REG = 29;
 
    std::string uniqueLabel()
    {
        return "L" + std::to_string(labelIndex++);
    }

    // Makes room for symbols and sets their location values
    void resolveSymbolLocations(SymbolTable& table, Codegen& gen)
    {
        // We keep track of return-to-os address
        gen.lis(29, "exitAddrGlobalXXXX");
        gen.sw(31, 0, 29);

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
            auto reg = 1;

            for(auto& v : f.args) {
                if(reg >= RETVAL_REG) {
                    throw PosError{v.pos, "Function " + f.name + " takes too many arguments."};
                }

                v.loc = reg++;
            }

            for(auto& v : f.locals) {
                if(reg >= RETVAL_REG) {
                    throw PosError{v.pos, "Function " + f.name + " has too many locals."};
                }

                v.loc = reg++;
            }

            // NOTE(Apaar): We use firstReg - 1 to store return value
            f.firstReg = reg + 1;
        }
    }

    int compileCall(SymbolTable& table, const CallAST& ast, Codegen& gen)
    {
        auto func = table.getFunc(ast.getFuncName());

        if(!func) {
            throw PosError{ast.getPos(), "Attempted to call undeclared function " + ast.getFuncName()};
        }

        if(ast.getArgs().size() != func->args.size()) {
            throw PosError{ast.getPos(), "Incorrect amount of arguments supplied to " + ast.getFuncName() + "; expected " + std::to_string(func->args.size())};
        }
        
        int spaceUsed = 0;

        // Store all the registers in use so far
        for(int i = 1; i < curReg; ++i) {
            spaceUsed += 4;
            gen.sw(i, -spaceUsed, 30);
        }
 
        int prev = curReg;

        // We'll use registers which are not in use by the function
        // to store our temp args (because even if we stored them, when
        // this call is being compiled, temporary values are being stored
        // in them)
        curReg = std::max(func->firstReg, curFunc ? curFunc->firstReg : 0);

        int temp = curReg++;

        gen.lis(temp, spaceUsed);
        gen.sub(30, 30, temp);

        curReg = temp;

        // compile and assign arguments to the correct registers
        
        auto i = 0u;
        for(auto& arg : ast.getArgs()) {
            int reg = compileTerm(table, *arg, gen);
            
            gen.add(func->args[i].loc, reg, 0);

            // We are free to stomp that register now
            curReg = reg;
            i += 1;
        }

        // Jump into the function  
        temp = std::max(func->firstReg, curFunc ? curFunc->firstReg : 0);

        gen.lis(temp, func->name);
        gen.jalr(temp);

        curReg = prev;

        // Move return value (could be garbage if there is none) into temp reg
        gen.add(curReg++, func->firstReg - 1, 0);

        temp = curReg++;

        // Restore regs
        gen.lis(temp, spaceUsed);
        gen.add(30, 30, temp);

        curReg = temp;

        // -2 because we used an extra register to store the return value (but we didn't save that register)
        for(int i = curReg - 2; i >= 1; --i) {
            gen.lw(i, -spaceUsed, 30);
            spaceUsed -= 4;
        }

        return curReg - 1;
    }
    
    // Returns the register index into which the term's result is stored
    int compileTerm(SymbolTable& table, const AST& ast, Codegen& gen)
    {
        if(ast.getType() == AST::INT || ast.getType() == AST::BOOL || ast.getType() == AST::CHAR) {
			gen.lis(curReg++, static_cast<int32_t>(static_cast<const IntAST&>(ast).getValue()));

            return curReg - 1;
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

            return curReg - 1;
        } else if(ast.getType() == AST::PAREN) {
            return compileTerm(table, static_cast<const ParenAST&>(ast).getInner(), gen);
        } else if(ast.getType() == AST::ID) {
            auto idAst = static_cast<const IdAST&>(ast);
            auto var = table.getVar(idAst.getName(), curFunc);
            
            if(!var) {
                throw PosError{ast.getPos(), "Referencing undeclared identifier " + idAst.getName()};
            }

            if(var->func) {
                // HAHA no can't depend on this being saved properly when you're calling a function for example
                // return var->loc;

                // We move it into a temp reg we know works and then use that
                gen.add(curReg++, var->loc, 0);
                return curReg - 1;
            } else {
                gen.lw(curReg++, var->loc, 0); 
                return curReg - 1;
            }
        } else if(ast.getType() == AST::CALL) {
            auto& cst = static_cast<const CallAST&>(ast);

            return compileCall(table, cst, gen);
        } else if(ast.getType() == AST::STR) {
            gen.lis(curReg++, table.getString(static_cast<const StrAST&>(ast).getId()).loc);
            return curReg - 1;
        } else if(ast.getType() == AST::UNARY) {
            int reg = compileTerm(table, static_cast<const UnaryAST&>(ast).getRhs(), gen);

            switch(static_cast<const UnaryAST&>(ast).getOp()) {
                case '-': {
                    gen.sub(curReg++, 0, reg);
                    return curReg - 1;
                } break;

                case '*': {
                    gen.lw(curReg++, 0, reg);
                    return curReg - 1;
                } break;
            }
        } else if(ast.getType() == AST::CAST) {
            return compileTerm(table, static_cast<const CastAST&>(ast).getValue(), gen);
        }

        assert(ast.getType() == AST::BIN);

        auto& bst = static_cast<const BinAST&>(ast);

        int dest = curReg++;

        int a = compileTerm(table, bst.getLhs(), gen);
        int b = compileTerm(table, bst.getRhs(), gen);

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

        return dest;
    }

    void compileRestoreLinkAndSp(Codegen& gen)
    {
        int temp = curReg++;

        gen.lis(temp, 4);
        gen.add(30, 30, temp);
        gen.lw(31, -4, 30);

        curReg = temp;
    }

    void compileStatement(SymbolTable& table, const AST& ast, Codegen& gen)
    {
        if(ast.getType() == AST::BIN) {
            auto& bst = static_cast<const BinAST&>(ast);

            int reg = compileTerm(table, bst.getRhs(), gen);

            auto& lhs = bst.getLhs();

            if(lhs.getType() == AST::ID) {
                auto& name = static_cast<const IdAST&>(lhs).getName();

                auto var = table.getVar(name, curFunc);

                if(!var) {
                    throw PosError{ast.getPos(), "Attempted to reference undeclared identifier " + name};
                }

                if(!var->func) {
                    gen.sw(reg, var->loc, 0);
                } else {
                    gen.add(var->loc, reg, 0);
                }
            } else {
                assert(lhs.getType() == AST::UNARY);
                
                auto& ust = static_cast<const UnaryAST&>(lhs);

                assert(ust.getOp() == '*');

                int lreg = compileTerm(table, ust.getRhs(), gen);

                gen.sw(reg, 0, lreg);
            }

            // We're done with this register now that it's stored
            curReg = reg;
        } else if(ast.getType() == AST::BLOCK) {
            for(auto& a : static_cast<const BlockAST&>(ast).getAsts()) {
                compileStatement(table, *a, gen);
            }
        } else if(ast.getType() == AST::IF) {
            auto& ist = static_cast<const IfAST&>(ast);

            int prevReg = curReg;

            int cond = compileTerm(table, ist.getCond(), gen);

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
            auto& ist = static_cast<const WhileAST&>(ast);

            int prevReg = curReg;

            auto condLabel = uniqueLabel();

            gen.labelHere(condLabel);

            int cond = compileTerm(table, ist.getCond(), gen);

            auto endLabel = uniqueLabel();

            gen.beq(cond, 0, endLabel);

            compileStatement(table, ist.getBody(), gen);

            int temp = curReg++;

            gen.lis(temp, condLabel);
            gen.jr(temp);

            curReg = prevReg;

            gen.labelHere(endLabel);
        } else if(ast.getType() == AST::FUNC) {
            auto& fst = static_cast<const FuncAST&>(ast);
        
            curFunc = table.getFunc(fst.getName());
            
            assert(curFunc);

            int temp = curReg++;

            gen.labelHere(curFunc->name);

            // We can only use registers from firstReg onwards
            int prev = curReg;

            curReg = curFunc->firstReg;

            temp = curReg++;

            gen.sw(31, -4, 30);
            gen.lis(temp, 4);
            gen.sub(30, 30, temp);

            compileStatement(table, fst.getBody(), gen);

            // We are free to stop these regs now that the body of the function has been passed
            curReg = prev;

            compileRestoreLinkAndSp(gen);

            gen.jr(31);

            curFunc = nullptr;
        } else if(ast.getType() == AST::CALL) {
            auto& cst = static_cast<const CallAST&>(ast);

            // Ignore return value
            curReg = compileCall(table, cst, gen);
        } else if(ast.getType() == AST::RETURN) {
            if(!curFunc) {
                throw PosError{ast.getPos(), "You're trying to return but you're not inside a function. Think about that for a second."};
            }

            auto value = static_cast<const ReturnAST&>(ast).getValue();
        
            if(value) {
                int res = compileTerm(table, *value, gen);

                // Move the result into return value register
                gen.add(curFunc->firstReg - 1, res, 0);
            }

            compileRestoreLinkAndSp(gen);
            gen.jr(31);
        } else if(ast.getType() == AST::ASM) {
            gen.parse(ast.getPos(), static_cast<const AsmAST&>(ast).getCode());
        } else {
            throw PosError{ast.getPos(), "Expected statement."};
        }
    }
};
