basic: lexer.cc ast.cc error.cc parser.cc compiler.cc symbol.cc main.cc
	g++ -std=c++14 main.cc -o wat -g
