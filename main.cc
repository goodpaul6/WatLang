#include <iostream>

#include "lexer.cc"
#include "error.cc"
#include "ast.cc"
#include "parser.cc"
#include "compiler.cc"

int main(int argc, char** argv)
{
    using namespace std;

    try {
        Parser parser;

        auto asts = parser.parseUntilEof(cin);

        Compiler compiler;

        compiler.compile(asts, cout);
    } catch(PosError& e) {
        cerr << e.getPos().line << ": " << e.getMessage() << "\n";
    }

    return 0;
}
