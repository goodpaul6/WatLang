wat: lexer.cc ast.cc error.cc parser.cc compiler.cc symbol.cc main.cc
	g++ -std=c++14 main.cc -o wat -g

bin:
	mkdir bin

test: tests/fact.wat tests/hello.wat wat bin
	./wat < tests/fact.wat > bin/fact.wat.asm
	./wat < tests/hello.wat > bin/hello.wat.asm
	cs241.binasm < bin/fact.wat.asm > bin/fact.wat.mips
	cs241.binasm < bin/hello.wat.asm > bin/hello.wat.mips
	mips.twoints bin/fact.wat.mips
	mips.twoints bin/hello.wat.mips
