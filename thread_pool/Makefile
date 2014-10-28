.PHONY : all clean

CC = gcc
CFLAGS = -Wall -g

BIN = main
OBJS = main.o queue.o thread_pool.o

all : $(BIN)

$(BIN) : $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -lpthread
%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(BIN)
