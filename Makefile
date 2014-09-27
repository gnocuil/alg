CC        := g++
CFLAGS    := -lnfnetlink -lnetfilter_queue -lpthread --std=c++11 -O2
TARGET    := alg
OBJS      := main.o ip.o state.o packet.o socket.o nat.o communicator.o parser.o nfqueue.o flow.o
protocols := http ftp sip

.PHONY: all
all: $(TARGET)

$(TARGET) : proto parser_maker $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(foreach p,$(protocols),$(p)/lex.o) $(foreach p,$(protocols),$(p)/parse.o)  $(CFLAGS)

%.o: %.cc
	$(CC) -c -o $@ $<  $(CFLAGS)
	
.PHONY: parser_maker
parser_maker: parser_make_maker.cc
	$(CC) -o parsermaker parser_make_maker.cc
	./parsermaker $(protocols)

.PHONY: proto
proto: 
	$(foreach dir,$(protocols),make -C $(dir);) 

.PHONY: clean
clean :
	rm -f $(TARGET)
	rm -f *.o
	rm -rf obj
	rm -f parsermaker parser_make.h
	$(foreach dir,$(protocols),make clean -C $(dir);) 
