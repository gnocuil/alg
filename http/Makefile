all: a.out
a.out: main.cc lex.cc parse.cc HTTPScanner.h input.hpp
	g++ --std=c++11 *.cc
lex.cc: lexer
	flexc++ lexer
parse.cc: grammar
	bisonc++ grammar
	
	
clean:
	rm -f a.out lex.cc parse.cc HTTPScannerbase.h HTTPParserbase.h
