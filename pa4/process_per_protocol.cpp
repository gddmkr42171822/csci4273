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
void udp_out(int port_number, char *buffer, int s);
char *udp_read(int port_number, int s); 
void udp_readwrite_test(); 

sem_t sem;
static int serv_in_udp_port;
static int in_socket;
static int out_socket;
static bool udp_ports_ready;

using namespace std;

int main() {
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
	while(!udp_ports_ready) {

	}
	udp_readwrite_test();
	sem_destroy(&sem);
	return 0;
}

void create_udp_socket(void *socket_type) {
	struct sockaddr_in clientaddr;
	int s, errno;
	memset(&clientaddr, 0, sizeof(clientaddr));
	clientaddr.sin_family = AF_INET;
	clientaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
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
			sem_wait(&sem);
			printf("New OUT udp socket port number is %d\n", ntohs(clientaddr.sin_port));
			out_socket = s;
			sem_post(&sem);
		}
		else {
			sem_wait(&sem);
			printf("New IN udp socket port number is %d\n", ntohs(clientaddr.sin_port));
			cout << "Enter IN udp port number of server: ";
			cin >> serv_in_udp_port;
			in_socket = s;
			sem_post(&sem);
			udp_ports_ready = true;
		}
	}
}

void udp_out(int port_number, char *buffer, int s) {
	int sendto_error;
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr.sin_port = htons(port_number);
	sendto_error = sendto(s, buffer, sizeof(buffer), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if(sendto_error == -1) {
		fprintf(stderr, "error sending udp message: %s\n", strerror(errno));
	}
}

char *udp_read(int s) {
	char *buf = new char[BUFSIZE+1];
	bzero(&buf, sizeof(buf));
	int recvfrom_error;
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	socklen_t addrlen = sizeof(servaddr);
	recvfrom_error = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&servaddr, &addrlen);
	if(recvfrom_error == -1) {
		fprintf(stderr, "error receiving udp message: %s\n", strerror(errno));
	}
	buf[BUFSIZE+1] = '\0';
	return buf;
}

void udp_readwrite_test() {
	char *buf = new char[16];
	strncpy(buf, "This is a test!", 15);
	buf[16] = '\0';
	cout << out_socket << endl;
	cout << in_socket << endl;
	udp_out(serv_in_udp_port, buf, out_socket);
	buf = udp_read(in_socket);
	cout << buf << endl;
}
