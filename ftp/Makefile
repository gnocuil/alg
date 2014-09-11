CC        := g++
CFLAGS    := --std=c++11 -O2

all: lex.o parse.o

lex.o: lex.cc FTPScanner.h FTPScannerbase.h FTPParserbase.h
	$(CC) -c lex.cc  $(CFLAGS)
parse.o: parse.cc FTPParser.h FTPParserbase.h 
	$(CC) -c parse.cc  $(CFLAGS)
FTPScanner.h FTPScannerbase.h lex.cc: lexer
	flexc++ lexer
FTPParser.h FTPParserbase.h parse.cc: grammar
	bisonc++ grammar
	
	
clean:
	rm -f *.o a.out lex.cc parse.cc FTPScannerbase.h FTPParserbase.h
