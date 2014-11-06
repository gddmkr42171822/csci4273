CC = g++
FLAGS = -g -Wall -Wextra
PTHREAD = -pthread
RM = rm -f

all: threadpool message eventscheduler

threadpool: driver_threadpool.cc threadpool.h
	$(CC) $(FLAGS) -o threadpool $(PTHREAD) driver_threadpool.cc

message: driver_message.cc message.h
	$(CC) $(FLAGS) -o message $(PTHREAD) driver_message.cc

eventscheduler: driver_eventscheduler.cc eventscheduler.h threadpool.h
	$(CC) $(FLAGS) -o eventscheduler $(PTHREAD) driver_eventscheduler.cc

clean:
	$(RM) threadpool message eventscheduler

