.PHONY : all clean

CC = gcc
CFLAGS = -Wall -g

BIN = server client
OBJS = server.o client.o

all : $(BIN)

server : server.o
	$(CC) $(CFLAGS) $^ -o $@ -lpthread
client : client.o
	$(CC) $(CFLAGS) $^ -o $@
%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(BIN)
