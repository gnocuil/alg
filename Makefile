CC        := g++
CFLAGS    := -lnfnetlink -lnetfilter_queue
TARGET    := alg
OBJS      := main.o ip.o state.o packet.o socket.o nat.o
protocols := http

.PHONY: all
all: $(TARGET)

$(TARGET) : $(OBJS) proto
	$(CC) -o $(TARGET) $(OBJS) $(foreach p,$(protocols),$(p)/lex.o) $(foreach p,$(protocols),$(p)/parse.o)  $(CFLAGS)

%.o: %.cc
	$(CC) -c -o $@ $<  $(CFLAGS)

.PHONY: proto
proto: 
	$(foreach dir,$(protocols),make -C $(dir)) 

.PHONY: clean
clean :
	rm -f $(TARGET)
	rm -f *.o
	rm -rf obj
	$(foreach dir,$(protocols),make clean -C $(dir)) 
