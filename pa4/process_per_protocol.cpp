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

#include "threadpool.h"

#define BUFSIZE 4096

void create_udp_socket(void *socket_type);

using namespace std;

int main(int argc, char *argv[]) {
	int in_udp_socket = 2;
	int out_udp_socket = 1;
	ThreadPool th(20);
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
	struct sockaddr_in udpaddr;
	struct hostent *phe;
	int s, errno;
	memset(&udpaddr, 0, sizeof(udpaddr));
	udpaddr.sin_family = AF_INET;
	udpaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	udpaddr.sin_port = htons(0);
	if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		fprintf(stderr, "cannot creat UDP socket: %s\n", strerror(errno));
	}
	if(bind(s, (struct sockaddr *)&udpaddr, sizeof(udpaddr)) < 0) {
		fprintf(stderr, "can't bind to port: %s\n", strerror(errno));
		exit(1);
	}
	else {
		socklen_t socklen = sizeof(udpaddr);
		if((getsockname(s, (struct sockaddr *)&udpaddr, &socklen)) < 0) {
			fprintf(stderr, "getsockname error: %s\n", strerror(errno));
		}
		if((*(int *)socket_type) == 1) {
			printf("New out udp socket port number is %d\n", ntohs(udpaddr.sin_port));
		}
		else {
			printf("New in udp socket port number is %d\n", ntohs(udpaddr.sin_port));
		}
	}
}
