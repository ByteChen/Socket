.PHOINY:clean all
CC=gcc
CFLAGS=-Wall -g
BIN=p2pcli p2pser
all:$(BIN)
%.o:%.c
		$(CC) $(CFLAGS) -c $< -o $@
clean:
		rm -f *.o $(BIN)
