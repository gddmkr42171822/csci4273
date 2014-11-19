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
#include "message.h"
#include <semaphore.h>

#define BUFSIZE 4096
#define DEFAULT_NUM_THREADS 30
#define IN_SOCKET_TYPE 2
#define OUT_SOCKET_TYPE 1
#define HEADER_LEN 40
using namespace std;

struct header {
	int hlp;
	vector<char> other_info;
	int message_len;
};

void pipe_receive(int pipe_receive_read_end, int pipe_receive_write_end) {
	char *buffer = new char[BUFSIZE];
	char *write_buffer = new char[BUFSIZE];
	bzero(buffer, BUFSIZE);
	if(read(pipe_receive_read_end, buffer, BUFSIZE) == -1) {
		fprintf(stderr, "error reading from ethernet receive pipe: %s\n", strerror(errno));
	}
	Message *m = new Message(buffer, BUFSIZE);
	char *stripped_header = m->msgStripHdr(HEADER_LEN);
	header *temp_header = new header;
	memcpy(temp_header, stripped_header, sizeof(HEADER_LEN));
	cout << "Temp_header->hlp: " << temp_header->hlp << endl;
	m->msgFlat(write_buffer);
	cout << "Message after strip: " << write_buffer << endl;
	if(write(pipe_receive_write_end, write_buffer, BUFSIZE) == -1) {
		fprintf(stderr, "error writing to ip receive pipe: %s\n", strerror(errno));
	}
}
void pipe_send(int pipe_send_write_end, int pipe_send_read_end, int higher_protocol_id, int other_info) { 
	int n;
	char *buffer = new char[BUFSIZE];
	char *send_buffer = new char[BUFSIZE];
	header *ethernet_header = new header;
	bzero(ethernet_header, sizeof(header));
	ethernet_header->hlp = higher_protocol_id;
	ethernet_header->other_info.resize(other_info);
	bzero(buffer, BUFSIZE);
	n = read(pipe_send_read_end, buffer, BUFSIZE);
	if(n == -1) {
		fprintf(stderr, "error reading from ip send pipe: %s\n", strerror(errno));
		exit(1);
	}
	Message *m = new Message(buffer, n);
	ethernet_header->message_len = n;
	char struct_buffer[sizeof(*ethernet_header)];
	memcpy(struct_buffer, ethernet_header, sizeof(*ethernet_header));
	m->msgAddHdr(struct_buffer, sizeof(struct_buffer));
	m->msgFlat(send_buffer);
	if(write(pipe_send_write_end, send_buffer, BUFSIZE) == -1) {
		fprintf(stderr, "error writing to ethernet send pipe: %s\n", strerror(errno));
	}
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

void socket_write(int port_number, int s, int ethernet_send_pipe_read_end) {
	char buffer[BUFSIZE];
	//while(1) {
		bzero(buffer, BUFSIZE);
		if(read(ethernet_send_pipe_read_end, buffer, BUFSIZE) == -1) {
			fprintf(stderr, "error reading from pipe_send pipe: %s\n", strerror(errno));
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

void socket_read(int s, int ethernet_receive_pipe_write_end) {
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

void pipe_sendreceive_test() {
	int *ethernet_send_pipe = create_pipe();
	int *ethernet_receive_pipe = create_pipe();
	int *ip_receive_pipe = create_pipe();
	int *ip_send_pipe = create_pipe();
	char buffer[19];
	bzero(buffer, 19);
	/*
	write(ethernet_receive_pipe[1], "aaaaaaaaaaaaaaaahi!", 19); 
	pipe_receive(ethernet_receive_pipe[0], ip_receive_pipe[1]);
	read(ip_receive_pipe[0], buffer, 3);
	*/
	memcpy(buffer, "hi!", 3);
	cout << buffer << endl;
	write(ip_send_pipe[1], buffer, 3);
	//pipe_send(ethernet_send_pipe[1], ip_send_pipe[0], 3, 12);
	pipe_send(ethernet_receive_pipe[1], ip_send_pipe[0], 3, 12);
	pipe_receive(ethernet_receive_pipe[0], ip_send_pipe[1]);
	char buffer1[43];
	char test_buffer[43];
	bzero(buffer1, 43);
	read(ip_send_pipe[0], buffer1, 3);
	//read(ethernet_send_pipe[0], buffer1, 43);
	Message *m = new Message(buffer1, 3);
	cout << m->msgLen() << endl;
	//char *stripped_header = new char[sizeof(header)];	
	//stripped_header = m->msgStripHdr(sizeof(header));
	//cout << m->msgLen() << endl;
	m->msgFlat(test_buffer);
	test_buffer[m->msgLen()] = '\0'; 
	cout << test_buffer << endl;
	/*
	header *temp_header = new header;
	memcpy(temp_header, stripped_header, sizeof(header));
	cout << "Ethernet hlp: " << temp_header->hlp << endl;
	cout << "Expected 3" << endl;
	cout << "Message len: " << temp_header->message_len << endl;
	cout << "Expected 3" << endl; 
	cout << "Size of OI: "<< temp_header->other_info.size() << endl;
	cout << "Expected 12" << endl;
	*/
}

void socket_readwrite_test() {
	int *ethernet_send_pipe = create_pipe();
	int *ethernet_receive_pipe = create_pipe();
	char *buf = new char[BUFSIZE];
	strncpy(buf, "This is a test!", 15);
	write(ethernet_send_pipe[1], buf, 15); 
	int in_udp_socket, out_udp_socket, serv_in_udp_port;
	out_udp_socket = create_udp_socket(OUT_SOCKET_TYPE);
	in_udp_socket = create_udp_socket(IN_SOCKET_TYPE);
	cout << "Enter the port number of the server in udp socket: ";
	cin >> serv_in_udp_port;
	thread write_socket (socket_write, serv_in_udp_port, out_udp_socket, ethernet_send_pipe[0]); 
	thread read_socket (socket_read, in_udp_socket, ethernet_receive_pipe[1]);
	bzero(buf, 15);
	read(ethernet_receive_pipe[0], buf, 15);
	cout << buf << endl;
	cout << "Expected: This is a test!" << endl;
	write_socket.join();
	read_socket.join();
}

int main() {
//	socket_readwrite_test();
	pipe_sendreceive_test();
	return 0;
}

