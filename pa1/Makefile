CC=gcc
CFLAGS = -g -Wall

all: chat_client chat_coordinator session_server
session_server: chat_server.o
	$(CC) $(CFLAGS) -o chat_server chat_server.o
chat_client: chat_client.o
	$(CC) $(CFLAGS) -o chat_client chat_client.o
chat_coordinator: chat_coordinator.o 
	$(CC) $(CFLAGS) -o chat_coordinator chat_coordinator.o
chat_client.o: chat_client.c
	$(CC) $(CFLAGS) -c chat_client.c
chat_coordinator.o: chat_coordinator.c 
	$(CC) $(CFLAGS) -c chat_coordinator.c
chat_server.o: chat_server.c
	$(CC) $(CFLAGS) -c chat_server.c
clean:
	rm -f *.o
	rm -f *~
	rm -f chat_client
	rm -f chat_coordinator
	rm -f chat_server
