CC        := g++
CFLAGS    := --std=c++11 -O2

all: lex.o parse.o

lex.o: lex.cc SIPScanner.h SIPScannerbase.h SIPParserbase.h tokentype.h
	$(CC) -c lex.cc  $(CFLAGS)
parse.o: parse.cc SIPParser.h SIPParserbase.h tokentype.h
	$(CC) -c parse.cc  $(CFLAGS)
tokentype.h:
	cp ../common/tokentype.h .
SIPScanner.h SIPScannerbase.h lex.cc: lexer
	flexc++ lexer
SIPParser.h SIPParserbase.h parse.cc: grammar
	bisonc++ grammar
	
	
clean:
	rm -f *.o a.out lex.cc parse.cc SIPScannerbase.h SIPParserbase.h tokentype.h
