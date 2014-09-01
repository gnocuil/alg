CC     := g++
CFLAGS := -lnfnetlink -lnetfilter_queue
TARGET := test
OBJS   := main.o ip.o state.o

all: $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

%.o: %.cc
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	rm -f $(TARGET)
	rm -f *.o
