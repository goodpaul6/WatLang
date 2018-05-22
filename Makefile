wat: lexer.cc ast.cc error.cc parser.cc compiler.cc symbol.cc main.cc
	g++ -std=c++14 main.cc -o wat -g

bin:
	mkdir bin

bin/%.mips: tests/%.wat basic.wat
	./wat < $< > bin/$(basename $(notdir ${<})).asm
	cs241.binasm < bin/$(basename $(notdir ${<})).asm > $@

test: wat bin bin/fact.mips bin/hello.mips bin/globals.mips bin/putn.mips bin/unary.mips
	mips.twoints bin/fact.mips
	mips.twoints bin/hello.mips
	mips.twoints bin/globals.mips
	mips.twoints bin/putn.mips
	mips.array bin/unary.mips
