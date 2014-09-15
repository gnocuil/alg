CC        := g++
CFLAGS    := --std=c++11 -O2

all: a.out lex.o parse.o

a.out: main.o lex.o parse.o
	g++ *.o

main.o: main.cc tokentype.h
	$(CC) -c main.cc  $(CFLAGS)

lex.o: lex.cc FTPScanner.h FTPScannerbase.h FTPParserbase.h tokentype.h
	$(CC) -c lex.cc  $(CFLAGS)
parse.o: parse.cc FTPParser.h FTPParserbase.h tokentype.h
	$(CC) -c parse.cc  $(CFLAGS)
tokentype.h:
	cp ../common/tokentype.h .
FTPScanner.h FTPScannerbase.h lex.cc: lexer
	flexc++ lexer
FTPParser.h FTPParserbase.h parse.cc: grammar
	bisonc++ grammar
	
	
clean:
	rm -f *.o a.out lex.cc parse.cc SIPScannerbase.h SIPParserbase.h tokentype.h
