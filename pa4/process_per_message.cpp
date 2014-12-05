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
#define NUM_MESSAGES 100
using namespace std;

struct header {
	int hlp;
	vector<char> other_info;
	int message_len;
};


static int serv_in_udp_port, out_udp_socket, in_udp_socket;
static int num_telnet_messages, num_rdp_messages, num_dns_messages, num_ftp_messages;

Message *append_header_to_message(int higher_protocol_id, int other_info, Message *msg) {
	header *message_header = new header;
	message_header->hlp = higher_protocol_id;
	message_header->other_info.resize(other_info);
	message_header->message_len = msg->msgLen();
	msg->msgAddHdr((char*)message_header, sizeof(header));
	return msg;
}

void telnet_receive(Message *msg) {
	char message[BUFSIZE];
	header *message_header = (header *)msg->msgStripHdr(HEADER_LEN);
	(void) message_header;
	msg->msgFlat(message);
	message[msg->msgLen()] = '\n';
	//cout << message << endl;
	num_telnet_messages++;
	if(num_telnet_messages == NUM_MESSAGES) {
		cout << "telnet messages received: " << num_telnet_messages << endl;
	}
	delete msg;
}

void ftp_receive(Message *msg) {
	char message[BUFSIZE];
	header *message_header = (header *)msg->msgStripHdr(HEADER_LEN);
	(void) message_header;
	msg->msgFlat(message);
	message[msg->msgLen()] = '\n';
	//cout << message << endl;
	num_ftp_messages++;
	if(num_ftp_messages == NUM_MESSAGES) {
		cout << "ftp messages received: " << num_ftp_messages << endl;
	}
	delete msg;
}

void rdp_receive(Message *msg) {
	char message[BUFSIZE];
	header *message_header = (header *)msg->msgStripHdr(HEADER_LEN);
	(void) message_header;
	msg->msgFlat(message);
	message[msg->msgLen()] = '\n';
	//cout << message << endl;
	num_rdp_messages++;
	if(num_rdp_messages == NUM_MESSAGES) {
		cout << "rdp messages received: " << num_rdp_messages << endl;
	}
	delete msg;
}

void dns_receive(Message *msg) {
	char message[BUFSIZE];
	header *message_header = (header *)msg->msgStripHdr(HEADER_LEN);
	(void) message_header;
	msg->msgFlat(message);
	message[msg->msgLen()] = '\n';
	//cout << message << endl;
	num_dns_messages++;
	if(num_dns_messages == NUM_MESSAGES) {
		cout << "dns messages received: " << num_dns_messages << endl;
	}
	delete msg;
}

void tcp_receive(Message *msg) {
	header *message_header = (header *)msg->msgStripHdr(HEADER_LEN);
	if(message_header->hlp == 5) {
		return ftp_receive(msg);
	}
	else if(message_header->hlp == 6) {
		return telnet_receive(msg);
	}
	else {
		fprintf(stderr, "error stripping tcp header");
	}
}

void udp_receive(Message *msg) {
	header *message_header = (header *)msg->msgStripHdr(HEADER_LEN);
	if(message_header->hlp == 7) {
		return rdp_receive(msg);
	}
	else if(message_header->hlp == 8) {
		return dns_receive(msg);
	}
	else {
		fprintf(stderr, "error stripping udp header");
	}
}

void ip_receive(Message *msg) {
	header *message_header = (header *)msg->msgStripHdr(HEADER_LEN);
	if(message_header->hlp == 3) {
		return tcp_receive(msg);
	}
	else if(message_header->hlp == 4) {
		return udp_receive(msg);
	}
	else {
		fprintf(stderr, "error stripping ip header");
	}
}

void ethernet_receive(void *msg) {
	//char* message = new char[BUFSIZE];
	Message *temp_msg = (Message *) msg;
	//temp_msg->msgFlat(message);
	//message[temp_msg->msgLen()] = '\n';
	//cout << message << endl;
	header *message_header = (header *)temp_msg->msgStripHdr(HEADER_LEN);
	if(message_header->hlp == 2) {
		return ip_receive(temp_msg);
	}
	else {
		fprintf(stderr, "error stripping ethernet header");
	}
}

void socket_send(int prot_id, Message *msg) {
	char *send_buffer = new char[BUFSIZE];
	Message *message; 
	int sendto_error;
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(serv_in_udp_port);
	bzero(send_buffer, BUFSIZE);
	message = append_header_to_message(prot_id, 0, msg);
	message->msgFlat(send_buffer);
	sendto_error = sendto(out_udp_socket, send_buffer, BUFSIZE, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if(sendto_error == -1) {
		fprintf(stderr, "error sending udp message: %s\n", strerror(errno));
	}
}

void ethernet_send(int prot_id, Message *msg) {
	Message *message = append_header_to_message(prot_id, 8, msg);
	return socket_send(1, message);
}

void ip_send(int prot_id, Message *msg) {
	Message *message = append_header_to_message(prot_id, 12, msg);
	return ethernet_send(2, message);
}

void tcp_send(int prot_id, Message *msg) {
	Message *message = append_header_to_message(prot_id, 4, msg);
	return ip_send(3, message);
}

void udp_send(int prot_id, Message *msg) {
	Message *message = append_header_to_message(prot_id, 4, msg);
	return ip_send(4, message);
}

void ftp_send(int prot_id, Message *msg) {
	Message *message = append_header_to_message(prot_id, 8, msg);
	return tcp_send(5, message);
}

void telnet_send(int prot_id, Message *msg) {
	Message *message = append_header_to_message(prot_id, 8, msg);
	return tcp_send(6, message);
}

void rdp_send(int prot_id, Message *msg) {
	Message *message = append_header_to_message(prot_id, 12, msg);
	return udp_send(7, message);
}

void dns_send(int prot_id, Message *msg) {
	Message *message = append_header_to_message(prot_id, 8, msg);
	return udp_send(8, message);
}

void application_ftp() {
	for(int i = 0; i < NUM_MESSAGES; i++) {
		char *message = new char[BUFSIZE];
		memcpy(message, "5", 1);
		Message *msg = new Message(message, 1);
		ftp_send(0, msg);
		usleep(5000);
	}
}

void application_telnet() {
	for(int i = 0; i < NUM_MESSAGES; i++) {
		char *message = new char[BUFSIZE];
		memcpy(message, "6", 1);
		Message *msg = new Message(message, 1);
		telnet_send(0, msg);
		usleep(5000);
	}
}

void application_rdp() {
	for(int i = 0; i < NUM_MESSAGES; i++) {
		char *message = new char[BUFSIZE];
		memcpy(message, "7", 1);
		Message *msg = new Message(message, 1);
		rdp_send(0, msg);
		usleep(5000);
	}
}

void application_dns() {
	
	for(int i = 0; i < NUM_MESSAGES; i++) {
		char *message = new char[BUFSIZE];
		memcpy(message, "8", 1);
		Message *msg = new Message(message, 1);
		dns_send(0, msg);
		usleep(500);
	}
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

void socket_receive(ThreadPool *th) {
	int recvfrom_error;
	while(1) {
		char *read_buffer = new char[BUFSIZE];
		bzero(read_buffer, BUFSIZE);
		recvfrom_error = recvfrom(in_udp_socket, read_buffer, BUFSIZE, 0, NULL, NULL);
		if(recvfrom_error == -1) {
			fprintf(stderr, "error receiving udp message: %s\n", strerror(errno));
		}
		Message *m = new Message(read_buffer, BUFSIZE);
		header *message_header = (header *)m->msgStripHdr(HEADER_LEN);
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
	num_telnet_messages = 0;
	num_rdp_messages = 0;
	num_dns_messages = 0;
	num_ftp_messages = 0;
	int continue_;
	out_udp_socket = create_udp_socket(OUT_SOCKET_TYPE);
	in_udp_socket = create_udp_socket(IN_SOCKET_TYPE);
	thread socket_r (socket_receive, thread_pool);
	cout << "Enter the port number of the server in udp socket: ";
	cin >> serv_in_udp_port;
	cout << "Write message?";
	cin >> continue_;
	sleep(3);
	struct timeval begin, end;
	gettimeofday(&begin, NULL);
	thread ftp (application_ftp);
	thread rdp (application_rdp);
	thread dns (application_dns);
	thread telnet(application_telnet);
	ftp.join();
	rdp.join();
	dns.join();
	telnet.join();
	cout << "All messages written!" << endl;
	while((num_telnet_messages != NUM_MESSAGES) && (num_rdp_messages != NUM_MESSAGES) \
	&& (num_dns_messages != NUM_MESSAGES) && (num_ftp_messages != NUM_MESSAGES)) {

	}
	gettimeofday(&end, NULL);
	double elapsed = (end.tv_sec - begin.tv_sec) + ((end.tv_usec - begin.tv_usec)/1000000.0);
	cout << "elapsed time: " << elapsed << "seconds" << endl;
	socket_r.join();
	return 0;
}
