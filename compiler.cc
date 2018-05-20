#include <ostream>
#include <cassert>
#include <unordered_map>

struct Compiler
{
    void compile(SymbolTable& table, const std::vector<std::unique_ptr<AST>>& asts, std::ostream& out)
    {
        resolveSymbolLocations(table, out);

        out << "; code\n";

        for(auto& ast : asts) {
            compileStatement(table, *ast, out);
        }

        out << "jr $31\n";
    }

private:
    int curReg = 1;
    int labelIndex = 0;

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
        auto i = 0u;

        out << "; globals\n";

        auto startLabel = uniqueLabel();

        out << "lis $1\n";
        out << ".word " << startLabel << "\n";
        out << "jr $1\n";

        for(auto& v : table.globals) {
            out << ".word 0\n";

            v.loc = i;
            i += 4;
        }

        out << startLabel << ":\n";

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
        }
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

            auto var = table.getVar(idAst.getName(), nullptr);

            if(var->func) {
                return var->loc;
            } else {
                out << "lw $" << curReg++ << ", " << var->loc << "($0)\n";
            
                return curReg - 1;
            }
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
        }

        curReg = dest + 1;

        return dest;
    }

    // Returns the register in which the zero/non-zero result lies
    int compileRelation(SymbolTable& table, const AST& ast, std::ostream& out)
    {
        if(ast.getType() == AST::BIN) {
            auto& bst = static_cast<const BinAST&>(ast);
            
            int dest = curReg++;

            int a = compileTerm(table, bst.getLhs(), out);
            int b = compileTerm(table, bst.getRhs(), out);

            switch(bst.getOp()) {
                case TOK_EQUALS: {
                    out << "lis $" << dest << "\n";
                    out << ".word 1\n";

                    // Set the result to 0 if they're not equal
                    out << "beq $" << a << ", $" << b << ", 1\n";
                    out << "add $1, $0, $0\n";
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

                default:
                    throw PosError{ast.getPos(), "You're using an arithmetic expression in a conditional. That's probably not intentional."};
                    break;
            }

            curReg = dest + 1;

            return dest;
        } else {
            return compileTerm(table, ast, out);
        }
    }

    void compileStatement(SymbolTable& table, const AST& ast, std::ostream& out)
    {
        if(ast.getType() == AST::BIN) {
            auto& bst = static_cast<const BinAST&>(ast);

            int reg = compileTerm(table, bst.getRhs(), out);

            auto& name = static_cast<const IdAST&>(bst.getLhs()).getName();

            auto var = table.getVar(name, nullptr);
     
            out << "; storing into " << var->name << "\n";
            out << "sw $" << reg << ", " << var->loc << "($0)\n";
            
            // We're done with this register now that it's stored
            curReg = reg;
        } else if(ast.getType() == AST::BLOCK) {
            for(auto& a : static_cast<const BlockAST&>(ast).getAsts()) {
                compileStatement(table, *a, out);
            }
        } else if(ast.getType() == AST::IF) {
            auto& ist = static_cast<const IfAST&>(ast);

            int prevReg = curReg;

            int cond = compileRelation(table, ist.getCond(), out);

            auto altLabel = uniqueLabel();
            auto endLabel = uniqueLabel();

            out << "beq $" << cond << ", $0, " << altLabel << "\n";

            compileStatement(table, ist.getBody(), out);

            int temp = curReg++;

            out << "lis $" << temp << "\n";
            out << ".word " << endLabel << "\n";
            out << "jr $" << temp << "\n";

            curReg = prevReg;

            if(ist.getAlt()) {
                out << altLabel << ":\n";
                compileStatement(table, *ist.getAlt(), out);
            }

            out << endLabel << ":\n";
        } else {
            throw PosError{ast.getPos(), "Expected statement."};
        }
    }
};
