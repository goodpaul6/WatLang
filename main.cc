#include <iostream>

#include "error.cc"
#include "emulator.cc"
#include "codegen.cc"
#include "lexer.cc"
#include "symbol.cc"
#include "ast.cc"
#include "typer.cc"
#include "parser.cc"
#include "compiler.cc"

int main(int argc, char** argv)
{
    using namespace std;

    try {
        if(argc != 2) {
            std::cerr << "Usage: " << argv[0] << " [file.wat]\n";
            return 1;
        }

        std::ifstream file{argv[1]};

        SymbolTable table;

        Parser parser;

        parser.includes.insert(argv[1]);

        auto asts = parser.parseUntilEof(table, file);

        Typer typer;

        for(auto& a : asts) {
            typer.checkTypes(table, *a);
        }

        Codegen gen;

        Compiler compiler;

        compiler.compile(table, asts, gen);

        auto code = gen.getPatchedCode();

        run(&code[0], code.size() * sizeof(Instruction));
    } catch(const PosError& e) {
        if(e.getPos().filename.empty()) {
            cerr << e.getPos().line << ": " << e.getMessage() << "\n";
        } else {
            cerr << e.getPos().filename << '(' << e.getPos().line << "): " << e.getMessage() << "\n";
        }

        return 1;
    } catch(const std::exception& e) {
        cerr << e.what() << "\n";
        return 1;
    } catch(...) {
        throw;
    }

    return 0;
}
