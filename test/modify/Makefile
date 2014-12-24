CC        := g++
CFLAGS    := -lpthread --std=c++11 -O2

all: modify

modify: modify.cc
	$(CC) -o modify modify.cc  $(CFLAGS)
	
clean:
	rm -f modify
