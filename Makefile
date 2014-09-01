CC     := g++
CFLAGS := -lnfnetlink -lnetfilter_queue
TARGET := alg
OBJS   := main.o ip.o state.o

all: $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(CFLAGS)

%.o: %.cc
	$(CC) -c -o $@ $<  $(CFLAGS)

clean :
	rm -f $(TARGET)
	rm -f *.o
