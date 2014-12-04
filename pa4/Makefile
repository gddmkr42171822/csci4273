CC=g++
FLAGS=-g -std=c++11 -Wall -Wextra 
LIBS=-lpthread
RM = rm -f

all: process_per_protocol process_per_message

process_per_message:  process_per_message.o message.o
	$(CC) $(FLAGS) -o $@ $^ $(LIBS)

process_per_protocol: process_per_protocol.o message.o
	$(CC) $(FLAGS) -o $@ $^ $(LIBS)

process_per_protocol.o: process_per_protocol.cpp 
	$(CC) $(FLAGS) -c $^

process_per_message.o: process_per_message.cpp threadpool.h
	$(CC) $(FLAGS) -c $^

message.o: message.cpp message.h
	$(CC) $(FLAGS) -c $^

clean:
	$(RM) *.gch message.o process_per_protocol.o process_per_message.o 
	$(RM) process_per_message process_per_protocol
