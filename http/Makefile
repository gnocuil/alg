all: parse.o
	@echo http
parse.o: lex.cc parse.cc HTTPScanner.h input.hpp
	g++ -c lex.cc  --std=c++11
	g++ -c parse.cc  --std=c++11
lex.cc: lexer
	flexc++ lexer
parse.cc: grammar
	bisonc++ grammar
	
	
clean:
	rm -f a.out lex.cc parse.cc HTTPScannerbase.h HTTPParserbase.h
