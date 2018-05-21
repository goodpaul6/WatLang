#include <ostream>
#include <cassert>
#include <unordered_map>

struct Compiler
{
    void compile(SymbolTable& table, const std::vector<std::unique_ptr<AST>>& asts, std::ostream& out)
    {
        if(!table.getFunc("main")) {
            throw std::runtime_error{"Missing main function."};
        }

        resolveSymbolLocations(table, out);

        out << "; code\n";

        for(auto& ast : asts) {
            compileStatement(table, *ast, out);
        }
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
    void resolveSymbolLocations(SymbolTable& table, std::ostream& out)
    {
        out << "lis $29\n";
        out << ".word main\n";
        out << "jr $29\n";

        out << "; globals\n";

        auto i = 12u;

        for(auto& v : table.globals) {
            out << ".word 0\n";

            v.loc = i;
            i += 4;
        }

        out << "; strings\n";

        for(auto& s : table.strings) {
            s.loc = i;
            for(auto ch : s.str) {
                out << ".word " << (int)ch << "\n";
                i += 1;
            }

            // null-terminator
            out << ".word 0\n";
            i += 1;
        }

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

    int compileCall(SymbolTable& table, const CallAST& ast, std::ostream& out)
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
            out << "sw $" << i << ", -" << spaceUsed << "($30)\n";
        }
 
        int prev = curReg;

        // We'll use registers which are not in use by the function
        // to store our temp args (because even if we stored them, when
        // this call is being compiled, temporary values are being stored
        // in them)
        curReg = func->firstReg;

        int temp = curReg++;

        out << "lis $" << temp << "\n";
        out << ".word " << spaceUsed << "\n";
        out << "sub $30, $30, $" << temp << "\n";

        curReg = temp;

        // compile and assign arguments to the correct registers
        
        auto i = 0u;
        for(auto& arg : ast.getArgs()) {
            int reg = compileTerm(table, *arg, out);
            
            out << "add $" << func->args[i].loc << ", $" << reg << ", $0\n";

            // We are free to stomp that register now
            curReg = reg;
        }

        // Jump into the function  
        temp = func->firstReg;

        out << "lis $" << temp << "\n";
        out << ".word " << func->name << "\n";
        out << "jalr $" << temp << "\n";

        curReg = prev;

        // Move return value (could be garbage if there is none) into temp reg
        out << "add $" << curReg++ << ", $" << (func->firstReg - 1) << ", $0\n";

        temp = curReg++;

        // Restore regs
        out << "lis $" << temp << "\n";
        out << ".word " << spaceUsed << "\n";
        out << "add $30, $30, $" << temp << "\n";

        curReg = temp;

        // -2 because we used an extra register to store the return value (but we didn't save that register)
        for(int i = curReg - 2; i >= 1; --i) {
            out << "lw $" << i << ", -" << spaceUsed << "($30)\n";
            spaceUsed -= 4;
        }

        return curReg - 1;
    }
    
    // Returns the register index into which the term's result is stored
    int compileTerm(SymbolTable& table, const AST& ast, std::ostream& out)
    {
        if(ast.getType() == AST::INT) {
            out << "lis $" << curReg++ << "\n";
            out << ".word " << static_cast<const IntAST&>(ast).getValue() << "\n";

            return curReg - 1;
        } else if(ast.getType() == AST::ID) {
            auto idAst = static_cast<const IdAST&>(ast);

            auto var = table.getVar(idAst.getName(), curFunc);
            
            if(!var) {
                throw PosError{ast.getPos(), "Referencing undeclared identifier " + idAst.getName()};
            }

            if(var->func) {
                return var->loc;
            } else {
                out << "lw $" << curReg++ << ", " << var->loc << "($0)\n";
            
                return curReg - 1;
            }
        } else if(ast.getType() == AST::CALL) {
            auto& cst = static_cast<const CallAST&>(ast);

            return compileCall(table, cst, out);
        } else if(ast.getType() == AST::STR) {
            out << "lis $" << curReg++ << "\n";
            out << ".word " << table.getString(static_cast<const StrAST&>(ast).getId()).loc << "\n";

            return curReg - 1;
        }

        assert(ast.getType() == AST::BIN);

        auto& bst = static_cast<const BinAST&>(ast);

        int dest = curReg++;

        int a = compileTerm(table, bst.getLhs(), out);
        int b = compileTerm(table, bst.getRhs(), out);

        switch(bst.getOp()) {
            case '+': out << "add $" << dest << ", $" << a << ", $" << b << "\n"; break;
            case '-': out << "sub $" << dest << ", $" << a << ", $" << b << "\n"; break;
            case '*': {
                out << "mult $" << a << ", $" << b << "\n";
                out << "mflo $" << dest << "\n";
            } break;

            case '/': {
                out << "div $" << a << ", $" << b << "\n";
                out << "mflo $" << dest << "\n";
            } break;

            case TOK_EQUALS: {
                out << "lis $" << dest << "\n";
                out << ".word 1\n";

                // Set the result to 0 if they're not equal
                out << "beq $" << a << ", $" << b << ", 1\n";
                out << "add $" << dest << ", $0, $0\n";
            } break;

            case '<': {
                out << "slt $" << dest << ", $" << a << ", $" << b << "\n";
            } break;

            case '>': {
                out << "slt $" << dest << ", $" << a << ", $" << b << "\n";

                int temp = curReg++;
                out << "lis $" << temp << "\n";
                out << ".word 1\n";

                out << "sub $" << dest << ", $" << temp << ", $" << dest << "\n";

                // dest reg=1 if greater than or equal to b

                // we skip setting the dest to 0 if they're not equal
                out << "bne $" << a << ", $" << b << ", 1\n";
                out << "add $" << dest << ", $0, $0\n";
            } break;

            case TOK_LTE: {
                out << "slt $" << dest << ", $" << a << ", $" << b << "\n";

                // Set to 1 if they're equal
                out << "bne $" << a << ", $" << b << ", 2\n";
                out << "lis $" << dest << "\n";
                out << ".word 1\n";
            } break;

            case TOK_GTE: {
                out << "slt $" << dest << ", $" << a << ", $" << b << "\n";

                int temp = curReg++;
                out << "lis $" << temp << "\n";
                out << ".word 1\n";

                out << "sub $" << dest << ", $" << temp << ", $" << dest << "\n";

                // dest reg=1 if greater than or equal to b
            } break;
        }

        curReg = dest + 1;

        return dest;
    }

    void compileRestoreLinkAndSp(std::ostream& out)
    {
        int temp = curReg++;

        out << "lis $" << temp << "\n";
        out << ".word 4\n";
        out << "add $30, $30, $" << temp << "\n";
        out << "lw $31, -4($30)\n";
    }

    void compileStatement(SymbolTable& table, const AST& ast, std::ostream& out)
    {
        if(ast.getType() == AST::BIN) {
            auto& bst = static_cast<const BinAST&>(ast);

            int reg = compileTerm(table, bst.getRhs(), out);

            auto& name = static_cast<const IdAST&>(bst.getLhs()).getName();

            auto var = table.getVar(name, curFunc);
     
            if(!var) {
                throw PosError{ast.getPos(), "Attempted to reference undeclared identifier " + name};
            }

            if(!var->func) {
                out << "; storing into " << var->name << "\n";
                out << "sw $" << reg << ", " << var->loc << "($0)\n"; 
            } else {
                out << "add $" << var->loc << ", $" << reg << ", $0\n";
            }
            
            // We're done with this register now that it's stored
            curReg = reg;
        } else if(ast.getType() == AST::BLOCK) {
            for(auto& a : static_cast<const BlockAST&>(ast).getAsts()) {
                compileStatement(table, *a, out);
            }
        } else if(ast.getType() == AST::IF) {
            auto& ist = static_cast<const IfAST&>(ast);

            int prevReg = curReg;

            int cond = compileTerm(table, ist.getCond(), out);

            auto altLabel = uniqueLabel();
            auto endLabel = uniqueLabel();

            out << "beq $" << cond << ", $0, " << altLabel << "\n";

            compileStatement(table, ist.getBody(), out);

            int temp = curReg++;

            out << "lis $" << temp << "\n";
            out << ".word " << endLabel << "\n";
            out << "jr $" << temp << "\n";

            curReg = prevReg;

            out << altLabel << ":\n";
            if(ist.getAlt()) {
                compileStatement(table, *ist.getAlt(), out);
            }

            out << endLabel << ":\n";
        } else if(ast.getType() == AST::WHILE) {
            auto& ist = static_cast<const WhileAST&>(ast);

            int prevReg = curReg;

            auto condLabel = uniqueLabel();

            out << condLabel << ":\n";

            int cond = compileTerm(table, ist.getCond(), out);

            auto endLabel = uniqueLabel();

            out << "beq $" << cond << ", $0, " << endLabel << "\n";

            compileStatement(table, ist.getBody(), out);

            int temp = curReg++;

            out << "lis $" << temp << "\n";
            out << ".word " << condLabel << "\n";
            out << "jr $" << temp << "\n";

            curReg = prevReg;

            out << endLabel << ":\n";
        } else if(ast.getType() == AST::FUNC) {
            auto& fst = static_cast<const FuncAST&>(ast);
        
            curFunc = table.getFunc(fst.getName());
            
            assert(curFunc);

            int temp = curReg++;

            // Some info for debugging asm
            for(auto& arg : curFunc->args) {
                out << "; $" << arg.loc << " = " << arg.name << "\n";
            }

            for(auto& local : curFunc->locals) {
                out << "; $" << local.loc << " = " << local.name << "\n";
            }

            out << curFunc->name << ":\n";

            // We can only use registers from firstReg onwards
            int prev = curReg;

            curReg = curFunc->firstReg;

            temp = curReg++;

            out << "sw $31, -4($30)\n";
            out << "lis $" << temp << "\n";
            out << ".word 4\n";
            out << "sub $30, $30, $" << temp << "\n";

            compileStatement(table, fst.getBody(), out);

            // We are free to stop these regs now that the body of the function has been passed
            curReg = prev;

            compileRestoreLinkAndSp(out);
            out << "jr $31\n";

            curFunc = nullptr;
        } else if(ast.getType() == AST::CALL) {
            auto& cst = static_cast<const CallAST&>(ast);

            // Ignore return value
            curReg = compileCall(table, cst, out);
        } else if(ast.getType() == AST::RETURN) {
            if(!curFunc) {
                throw PosError{ast.getPos(), "You're trying to return but you're not inside a function. Think about that for a second."};
            }

            auto value = static_cast<const ReturnAST&>(ast).getValue();
        
            if(value) {
                int res = compileTerm(table, *value, out);

                // Move the result into return value register
                out << "add $" << (curFunc->firstReg - 1) << ", $" << res << ", $0\n";
            }

            compileRestoreLinkAndSp(out);
            out << "jr $31\n";
        } else if(ast.getType() == AST::ASM) {
            out << static_cast<const AsmAST&>(ast).getCode() << "\n";
        } else {
            throw PosError{ast.getPos(), "Expected statement."};
        }
    }
};
