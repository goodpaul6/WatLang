#include <iostream>

#include "error.cc"
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
        SymbolTable table;

        Parser parser;

        auto asts = parser.parseUntilEof(table, cin);

        Typer typer;

        for(auto& a : asts) {
            typer.checkTypes(table, *a);
        }

        Compiler compiler;

        compiler.compile(table, asts, cout);
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
