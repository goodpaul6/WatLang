#include <iostream>

#include "lexer.cc"
#include "ast.cc"
#include "parser.cc"
#include "compiler.cc"

int main(int argc, char** argv)
{
    using namespace std;

    Parser parser;

    auto asts = parser.parseUntilEof(cin);

    Compiler compiler;

    compiler.compile(asts, cout);

    return 0;
}
