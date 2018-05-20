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

    void compileStatement(SymbolTable& table, const AST& ast, std::ostream& out)
    {
        assert(ast.getType() == AST::BIN);

        auto& bst = static_cast<const BinAST&>(ast);

        int reg = compileTerm(table, bst.getRhs(), out);

        auto& name = static_cast<const IdAST&>(bst.getLhs()).getName();

        auto var = table.getVar(name, nullptr);
 
        out << "; storing into " << var->name << "\n";
        out << "sw $" << reg << ", " << var->loc << "($0)\n";
        
        // We're done with this register now that it's stored
        curReg = reg;
    }
};
