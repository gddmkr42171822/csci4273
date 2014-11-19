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
#define in_socket_type 2
#define out_socket_type 1

int create_udp_socket(int socket_type);
void udp_readwrite_test(int out_socket, int in_socket, int serv_in_udp_port);
void udp_out(int port_number, char *buffer, int s);
char *udp_read(int port_number, int s); 

using namespace std;

int main() {
	int in_udp_socket, out_udp_socket, serv_in_udp_port;
	out_udp_socket = create_udp_socket(out_socket_type);
	in_udp_socket = create_udp_socket(in_socket_type);
	cout << "Enter the port number of the server in udp socket: ";
	cin >> serv_in_udp_port;
	udp_readwrite_test(out_udp_socket, in_udp_socket, serv_in_udp_port);
	return 0;
}

int create_udp_socket(int socket_type) {
	struct sockaddr_in clientaddr;
	int s, errno;
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
		if(socket_type == 1) {
			printf("New OUT udp socket port number is %d\n", ntohs(clientaddr.sin_port));
			return s;
		}
		else {
			printf("New IN udp socket port number is %d\n", ntohs(clientaddr.sin_port));
			return s;
		}
	}
}

void udp_out(int port_number, char *buffer, int s) {
	int sendto_error;
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port_number);
	sendto_error = sendto(s, buffer, BUFSIZE, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if(sendto_error == -1) {
		fprintf(stderr, "error sending udp message: %s\n", strerror(errno));
	}
}

char *udp_read(int s) {
	char *buf = new char[BUFSIZE]; 
	bzero(buf, BUFSIZE);
	int recvfrom_error;
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	socklen_t addrlen = sizeof(servaddr);
	recvfrom_error = recvfrom(s, buf, BUFSIZE, 0, (struct sockaddr *)&servaddr, &addrlen);
	if(recvfrom_error == -1) {
		fprintf(stderr, "error receiving udp message: %s\n", strerror(errno));
	}
	return buf;
}

void udp_readwrite_test(int out_socket, int in_socket, int serv_in_udp_port) {
	char *buf = new char[BUFSIZE];
	strncpy(buf, "This is a test!", 15);
	udp_out(serv_in_udp_port, buf, out_socket);
	buf = udp_read(in_socket);
	cout << buf << endl;
}
