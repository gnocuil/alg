all: lex.o parse.o
	@echo http
lex.o: lex.cc HTTPScanner.h HTTPScannerbase.h input.h HTTPParserbase.h
	g++ -c lex.cc  --std=c++11
parse.o: parse.cc HTTPParser.h HTTPParserbase.h input.hpp
	g++ -c parse.cc  --std=c++11
HTTPScanner.h HTTPScannerbase.h lex.cc: lexer
	flexc++ lexer
HTTPParser.h HTTPParserbase.h parse.cc: grammar
	bisonc++ grammar
	
	
clean:
	rm -f a.out lex.cc parse.cc HTTPScannerbase.h HTTPParserbase.h
