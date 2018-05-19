#include <ostream>
#include <cassert>

struct Compiler
{
    void compile(const std::vector<std::unique_ptr<AST>>& asts, std::ostream& out)
    {
        for(auto& ast : asts) {
            compileStatement(*ast, out);
        }
    }

private:
    int curReg = 1;
    int labelIndex = 0;
    
    std::string uniqueLabel()
    {
        return "L" + std::to_string(labelIndex++);
    }
    
    // Returns the register index into which the term's result is stored
    int compileTerm(const AST& ast, std::ostream& out)
    {
        if(ast.getType() == AST::INT) {
            out << "lis $" << curReg++ << "\n";
            out << ".word " << static_cast<const IntAST&>(ast).getValue() << "\n";

            return curReg - 1;
        }

        assert(ast.getType() == AST::BIN);

        auto& bst = static_cast<const BinAST&>(ast);

        int dest = curReg++;

        int a = compileTerm(bst.getLhs(), out);
        int b = compileTerm(bst.getRhs(), out);

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

    void compileStatement(const AST& ast, std::ostream& out)
    {
        assert(ast.getType() == AST::BIN);

        compileTerm(static_cast<const BinAST&>(ast).getRhs(), out);
        
        // TODO(Apaar): Assign to lhs
    }
};
