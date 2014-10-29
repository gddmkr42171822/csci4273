CC = gcc
CFLAGS = -Wall -Wextra -Wno-deprecated
SSL = -lcrypto -lssl
RM = rm -f

all: echoClient echoServer

echoClient: client/echoClient.c
	$(CC) $(CFLAGS) -o client/echoClient client/echoClient.c $(SSL)

echoServer: server/echoServer.c
	$(CC) $(CFLAGS) -o server/echoServer server/echoServer.c $(SSL)

clean:
	$(RM) client/echoClient
	$(RM) server/echoServer

	 
