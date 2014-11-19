#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <thread>

#include "threadpool.h"
#include <semaphore.h>

#define BUFSIZE 4096
#define DEFAULT_NUM_THREADS 30
#define in_socket_type 2
#define out_socket_type 1

int create_udp_socket(int socket_type);
void udp_write(int port_number, int s, int ethernet_send_pipe_read_end);
void udp_read(int s, int ethernet_receive_pipe_write_end); 
int *create_pipe(); 

using namespace std;

int main() {
	int in_udp_socket, out_udp_socket, serv_in_udp_port;
	out_udp_socket = create_udp_socket(out_socket_type);
	in_udp_socket = create_udp_socket(in_socket_type);
	cout << "Enter the port number of the server in udp socket: ";
	cin >> serv_in_udp_port;
	int *ethernet_send_pipe = create_pipe();
	int *ethernet_receive_pipe = create_pipe();
	char buffer[12];
	write(ethernet_send_pipe[1], "Hello there!", 12); 
	thread first (udp_write, serv_in_udp_port,out_udp_socket, ethernet_send_pipe[0]); 
	thread second (udp_read, in_udp_socket, ethernet_receive_pipe[1]);
	read(ethernet_receive_pipe[0], buffer, 12);
	cout << buffer << endl;
	first.join();
	second.join();
	return 0;
}

int *create_pipe() {
	int *pipefd = new int[2];
	if(pipe(pipefd) == -1) {
		fprintf(stderr, "error creating pipe: %s\n", strerror(errno));
	}
	return pipefd;
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

void udp_write(int port_number, int s, int ethernet_send_pipe_read_end) {
	char buffer[BUFSIZE];
	//while(1) {
		bzero(buffer, BUFSIZE);
		if(read(ethernet_send_pipe_read_end, buffer, BUFSIZE) == -1) {
			fprintf(stderr, "error reading from ethernet_send pipe: %s\n", strerror(errno));
		}
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
	//}
}

void udp_read(int s, int ethernet_receive_pipe_write_end) {
	char *buf = new char[BUFSIZE]; 
	int recvfrom_error;
	//while(1) {
		bzero(buf, BUFSIZE);
		recvfrom_error = recvfrom(s, buf, BUFSIZE, 0, NULL, NULL);
		if(recvfrom_error == -1) {
			fprintf(stderr, "error receiving udp message: %s\n", strerror(errno));
		}
		if(write(ethernet_receive_pipe_write_end, buf, BUFSIZE) == -1) {
			fprintf(stderr, "error writing to ethernet receive pipe: %s\n", strerror(errno));
		}
	//}
}

