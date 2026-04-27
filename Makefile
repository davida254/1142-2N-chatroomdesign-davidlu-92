CC=gcc
CFLAGS=-Wall -Wextra -std=c11

all: server client

server: multi_server.c
	$(CC) $(CFLAGS) multi_server.c -o server

client: multi_client.c
	$(CC) $(CFLAGS) multi_client.c -o client

clean:
	rm -f server client chat_history.txt snapshot_*.txt
