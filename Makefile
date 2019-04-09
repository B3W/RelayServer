CC = gcc
RM = rm -f

CFLAGS = -Wall -Werror -ggdb

relayServer: relayServer.c
	@$(CC) relayServer.c $(CFLAGS) -o relayServer

clean:
	@$(RM) relayServer
