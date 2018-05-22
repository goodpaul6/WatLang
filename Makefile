wat: lexer.cc ast.cc error.cc parser.cc compiler.cc symbol.cc main.cc
	g++ -std=c++14 main.cc -o wat -g

bin:
	mkdir bin

bin/%.mips: tests/%.wat basic.wat wat
	./wat < $< > bin/$(basename $(notdir ${<})).asm
	cs241.binasm < bin/$(basename $(notdir ${<})).asm > $@

zero:
	echo 0 >> bin/zero.txt
	echo 0 >> bin/zero.txt

test: wat bin bin/fact.mips bin/hello.mips bin/globals.mips bin/putn.mips bin/unary.mips bin/unarylhs.mips bin/tree.mips zero
	mips.twoints bin/fact.mips
	mips.twoints bin/hello.mips < bin/zero.txt
	mips.twoints bin/globals.mips
	mips.twoints bin/putn.mips < bin/zero.txt
	mips.array bin/unary.mips 
	mips.twoints bin/unarylhs.mips < bin/zero.txt
	mips.twoints bin/tree.mips < bin/zero.txt
