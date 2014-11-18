#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>

#include "threadpool.h"
#include <semaphore.h>

#define BUFSIZE 4096
#define DEFAULT_NUM_THREADS 30

void create_udp_socket(void *socket_type);
sem_t sem;

using namespace std;

int main(int argc, char *argv[]) {
	sem_init(&sem, 0, 1);
	int in_udp_socket = 2;
	int out_udp_socket = 1;
	ThreadPool th(DEFAULT_NUM_THREADS);
	do {
	/* Wait until a thread is avaible to create a udp socket */
	}
	while (!th.thread_avail());
	th.dispatch_thread(create_udp_socket, (void *)&out_udp_socket);
	do {
	/* Wait until a thread is avaible to create a udp socket */
	}
	while (!th.thread_avail());
	th.dispatch_thread(create_udp_socket, (void *)&in_udp_socket);

	return 0;
}

void create_udp_socket(void *socket_type) {
	struct sockaddr_in clientaddr;
	struct hostent *phe;
	int s, errno;
	int serv_out_udp_socket, serv_in_udp_socket;
	memset(&clientaddr, 0, sizeof(clientaddr));
	clientaddr.sin_family = AF_INET;
	clientaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	clientaddr.sin_port = htons(0);
	if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		fprintf(stderr, "cannot creat UDP socket: %s\n", strerror(errno));
	}
	if(bind(s, (struct sockaddr *)&clientaddr, sizeof(clientaddr)) < 0) {
		fprintf(stderr, "can't bind to port: %s\n", strerror(errno));
		exit(1);
	}
	else {
		socklen_t socklen = sizeof(clientaddr);
		if((getsockname(s, (struct sockaddr *)&clientaddr, &socklen)) < 0) {
			fprintf(stderr, "getsockname error: %s\n", strerror(errno));
		}
		if((*(int *)socket_type) == 1) {
			printf("New out udp socket port number is %d\n", ntohs(clientaddr.sin_port));
			sem_wait(&sem);
			cout << "Enter out udp socket port number: ";
			cin >> serv_out_udp_socket;
			sem_post(&sem);
		}
		else {
			printf("New in udp socket port number is %d\n", ntohs(clientaddr.sin_port));
			sem_wait(&sem);
			cout << "Enter in udp socket port number: ";
			cin >> serv_in_udp_socket;
			sem_post(&sem);
		}
	}
}

