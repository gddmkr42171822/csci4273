#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/time.h>
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

#define BUFSIZE 1024 
#define HEADER_LEN 40
#define IN_SOCKET_TYPE 2
#define OUT_SOCKET_TYPE 1
using namespace std;

struct header {
	int hlp;
	vector<char> other_info;
	int message_len;
};


static int serv_in_udp_port, out_udp_socket, in_udp_socket;

char *append_header_to_message(int higher_protocol_id, int other_info, char *buffer, int message_length) {
	char *send_buffer = new char[BUFSIZE];
	header *message_header = new header;
	message_header->hlp = higher_protocol_id;
	message_header->other_info.resize(other_info);
	message_header->message_len = message_length;
	Message *m = new Message(buffer, message_length);
	m->msgAddHdr((char*)message_header, sizeof(header));
	m->msgFlat(send_buffer);
	return send_buffer;
}

void ethernet_receive(void *msg) {
	char* message = new char[BUFSIZE];
	Message *temp_msg = (Message *) msg;
	temp_msg->msgFlat(message);
	message[temp_msg->msgLen()] = '\n';
	cout << message << endl;
}

void ip_receive(void *msg) {
	(void) msg;
}

void tcp_receive(void *msg) {
	(void) msg;
}

void udp_receive(void *msg) {
	(void) msg;
}

void telnet_receive(void *msg) {
	(void) msg;
}

void ftp_receive(void *msg) {
	(void) msg;
}

void rdp_receive(void *msg) {
	(void) msg;
}

void dns_receive(void *msg) {
	(void) msg;
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
		fprintf(stderr, "can't  to port: %s\n", strerror(errno));
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

void socket_send(int prot_id, Message *msg) {
	char *send_buffer = new char[BUFSIZE];
	char *message = new char[BUFSIZE]; 
	int sendto_error;
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(serv_in_udp_port);
	bzero(send_buffer, BUFSIZE);
	bzero(message, BUFSIZE);
	msg->msgFlat(message);
	send_buffer = append_header_to_message(prot_id, 0, message, msg->msgLen());
	sendto_error = sendto(out_udp_socket, send_buffer, BUFSIZE, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if(sendto_error == -1) {
		fprintf(stderr, "error sending udp message: %s\n", strerror(errno));
	}
}

void socket_receive(ThreadPool *th) {
	char *read_buffer = new char[BUFSIZE];
	int recvfrom_error;
	while(1) {
		bzero(read_buffer, BUFSIZE);
		recvfrom_error = recvfrom(in_udp_socket, read_buffer, BUFSIZE, 0, NULL, NULL);
		if(recvfrom_error == -1) {
			fprintf(stderr, "error receiving udp message: %s\n", strerror(errno));
		}
		Message *m = new Message(read_buffer, recvfrom_error);
		header *message_header = (header *)m->msgStripHdr(HEADER_LEN);
		(void) message_header;
		if(message_header->hlp == 1) {
			th->dispatch_thread(ethernet_receive, (void*)m);
		}
		else {
			fprintf(stderr, "error stripping socket header");
		}
	}
}

int main() {
	ThreadPool th(25);
	ThreadPool *thread_pool = &th; 
	char* message = new char[BUFSIZE];
	bzero(message, BUFSIZE);
	memcpy(message, "This is a test!", 15);
	Message *msg = new Message(message, 15); 
	int continue_;
	out_udp_socket = create_udp_socket(OUT_SOCKET_TYPE);
	in_udp_socket = create_udp_socket(IN_SOCKET_TYPE);
	thread socket_r (socket_receive, thread_pool);
	cout << "Enter the port number of the server in udp socket: ";
	cin >> serv_in_udp_port;
	cout << "Write message?";
	cin >> continue_;
	thread socket_s (socket_send, 1, msg);
	socket_r.join();
	socket_s.join();
	return 0;
}
