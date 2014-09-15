CC        := g++
CFLAGS    := --std=c++11 -O2

all: lex.o parse.o

lex.o: lex.cc HTTPScanner.h HTTPScannerbase.h input.hpp input.h HTTPParserbase.h tokentype.h
	$(CC) -c lex.cc  $(CFLAGS)
parse.o: parse.cc HTTPParser.h HTTPParserbase.h tokentype.h
	$(CC) -c parse.cc  $(CFLAGS)
tokentype.h:
	cp ../common/tokentype.h .
HTTPScanner.h HTTPScannerbase.h lex.cc: lexer
	flexc++ lexer
HTTPParser.h HTTPParserbase.h parse.cc: grammar
	bisonc++ grammar
	
	
clean:
	rm -f *.o a.out lex.cc parse.cc HTTPScannerbase.h HTTPParserbase.h tokentype.h
