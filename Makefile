CC        := g++
CFLAGS    := -lnfnetlink -lnetfilter_queue -lpthread --std=c++11 -O2
TARGET    := alg
OBJS      := main.o ip.o state.o packet.o socket.o nat.o communicator.o parser.o nfqueue.o
protocols := http ftp

.PHONY: all
all: $(TARGET)

$(TARGET) : proto $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(foreach p,$(protocols),$(p)/lex.o) $(foreach p,$(protocols),$(p)/parse.o)  $(CFLAGS)

%.o: %.cc
	$(CC) -c -o $@ $<  $(CFLAGS)

.PHONY: proto
proto: 
	$(foreach dir,$(protocols),make -C $(dir);) 

.PHONY: clean
clean :
	rm -f $(TARGET)
	rm -f *.o
	rm -rf obj
	$(foreach dir,$(protocols),make clean -C $(dir);) 
