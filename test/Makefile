CC        := g++
CFLAGS    := -lpthread --std=c++11 -O2

all: ftp

ftp: ftp.cc ftp.h ftp_srv.h
	$(CC) -o ftp ftp.cc ../ip.cc $(CFLAGS)
	
clean:
	rm -f ftp
