#include <iostream>

#include "lexer.cc"
#include "error.cc"
#include "symbol.cc"
#include "ast.cc"
#include "parser.cc"
#include "compiler.cc"

int main(int argc, char** argv)
{
    using namespace std;

    try {
        SymbolTable table;

        Parser parser;

        auto asts = parser.parseUntilEof(table, cin);

        Compiler compiler;

        compiler.compile(table, asts, cout);
    } catch(PosError& e) {
        cerr << e.getPos().line << ": " << e.getMessage() << "\n";
    }

    return 0;
}