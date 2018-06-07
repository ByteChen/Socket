.PHOINY:clean all
CC=gcc
CFLAGS=-Wall -g
BIN=server client p2pser p2pcli
all:$(BIN)
%.o:%.c
		$(CC) $(CFLAGS) -c $< -o $@
clean:
		rm -f *.o $(BIN)
