CC=g++
FLAGS=-g -std=gnu++11 -Wall -Wextra
LIBS=-lpthread
RM = rm -f

all: driver1 driver3 

driver1: driver1.cc threadpool.o
	$(CC) $(FLAGS) -o $@ $^ $(LIBS)

driver3: driver3.cc message.o
	$(CC) $(FLAGS) -o $@ $^ $(LIBS)

threadpool.o: threadpool.cpp
	$(CC) $(FLAGS) -o $@ -c $^

message.o: message.cpp
	$(CC) $(FLAGS) -o $@ -c $^

clean:
	$(RM) threadpool.o message.o driver1 driver3
