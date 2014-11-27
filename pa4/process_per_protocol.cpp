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
#define NUM_PROTOCOL_PIPES 17
#define NUM_PROTOCOL_PIPES_FDS 34
using namespace std;


/* Pipearray designations:
	0, 1 (ethernet recieve pipe read,write)
	2, 3 (ethernet send pipe read, write)
	4, 5 (ip receive pipe read, write)
	6, 7 (ip send pipe read, write)
	8, 9 (udp receive pipe read, write)
	10, 11 (tcp receive pipe read, write)
	12, 13 (udp/tcp send pipe read, write)
	14, 15 (ftp receive pipe read, write)
	16, 17 (telnet receive pipe read, write)
	18, 19 (telnet/ftp send pipe read, write)
	20, 21 (rdp receive pipe read, write)
	22, 23 (dns receive pipe read, write)
	24, 25 (rdp/dns send pipe read, write)
	26, 27 (application_message_ftp send pipe read, write)
	28, 29 (application_message_telnet send pipe read, write)
	30, 31 (application_message_rdp send pipe read, write)
	32, 33 (application_message_dns send pipe read, write)
*/

struct header {
	int hlp;
	vector<char> other_info;
	int message_len;
};

char *append_header_to_message(int higher_protocol_id, int other_info, char *buffer, int bytes_read) {
	char *send_buffer = new char[BUFSIZE];
	header *message_header = new header;
	message_header->hlp = higher_protocol_id;
	message_header->other_info.resize(other_info);
	message_header->message_len = bytes_read;
	Message *m = new Message(buffer, bytes_read);
	m->msgAddHdr((char*)message_header, sizeof(header));
	m->msgFlat(send_buffer);
	return send_buffer;
}

void application_to_dns(int *pipearray) {
	char *message = new char[BUFSIZE];
	memcpy(message, "This is DNS!", 12);
	for(int i = 0;i < 100;i++) {
		write(pipearray[33], message, 12);
	}
	usleep(5000);// 5 milliseconds
}

void application_to_rdp(int *pipearray) {
	char *message = new char[BUFSIZE];
	memcpy(message, "This is RDP!", 12);
	for(int i = 0;i < 100;i++) {
		write(pipearray[31], message, 12);
	}
	usleep(5000);// 5 milliseconds
}

void application_to_telnet(int *pipearray) {
	char *message = new char[BUFSIZE];
	memcpy(message, "This is telnet!", 15);
	for(int i = 0;i < 100;i++) {
		write(pipearray[29], message, 15);
	}
	usleep(5000);// 5 milliseconds
}

void application_to_ftp(int *pipearray) {
	char *message = new char[BUFSIZE];
	memcpy(message, "This is ftp!", 12);
	for(int i = 0;i < 100;i++) {
		write(pipearray[27], message, 12);
	}
	usleep(5000);// 5 milliseconds
}

void dns_send_pipe(int *pipearray) {
	int bytes_read;
	char *read_buffer = new char[BUFSIZE];
	char *send_buffer;
	while(1) {
		bzero(read_buffer, BUFSIZE);
		bytes_read = read(pipearray[32], read_buffer, BUFSIZE);
		if(bytes_read == -1) {
			fprintf(stderr, "error reading from application_dns send pipe: %s\n", strerror(errno));
			exit(1);
		}
		cout << "dns send message: " << read_buffer << endl;
		send_buffer = append_header_to_message(0, 8, read_buffer, bytes_read);
		if(write(pipearray[25], send_buffer, (bytes_read + HEADER_LEN)) == -1) {
			fprintf(stderr, "error writing to udp send pipe: %s\n", strerror(errno));
		}
	}
}

void dns_receive_pipe(int *pipearray) {
	char *buffer = new char[BUFSIZE];
	char *write_buffer = new char[BUFSIZE];
	while(1) {
		bzero(buffer, BUFSIZE);
		if(read(pipearray[20], buffer, BUFSIZE) == -1) {
			fprintf(stderr, "error reading from udp receive pipe: %s\n", strerror(errno));
		}
		Message *m = new Message(buffer, BUFSIZE);
		header *temp_header = (header *)m->msgStripHdr(HEADER_LEN);
		cout << "dns receive hlp: " << temp_header->hlp << endl;
		cout << "dns receive oi: " << temp_header->other_info.size() << endl;
		cout << "dns receive message_len: " << temp_header->message_len << endl;
		m->msgFlat(write_buffer);
		cout << "Message to dns: " << write_buffer << endl;
	}
}

void rdp_send_pipe(int *pipearray) {
	int bytes_read;
	char *read_buffer = new char[BUFSIZE];
	char *send_buffer;
	while(1) {
		bzero(read_buffer, BUFSIZE);
		bytes_read = read(pipearray[30], read_buffer, BUFSIZE);
		if(bytes_read == -1) {
			fprintf(stderr, "error reading from application_rdp send pipe: %s\n", strerror(errno));
			exit(1);
		}
		cout << "rdp send message: " << read_buffer << endl;
		send_buffer = append_header_to_message(0, 12, read_buffer, bytes_read);
		if(write(pipearray[25], send_buffer, (bytes_read + HEADER_LEN)) == -1) {
			fprintf(stderr, "error writing to udp send pipe: %s\n", strerror(errno));
		}
	}
}

void rdp_receive_pipe(int *pipearray) {
	char *buffer = new char[BUFSIZE];
	char *write_buffer = new char[BUFSIZE];
	while(1) {
		bzero(buffer, BUFSIZE);
		if(read(pipearray[20], buffer, BUFSIZE) == -1) {
			fprintf(stderr, "error reading from udp receive pipe: %s\n", strerror(errno));
		}
		Message *m = new Message(buffer, BUFSIZE);
		header *temp_header = (header *)m->msgStripHdr(HEADER_LEN);
		cout << "rdp receive hlp: " << temp_header->hlp << endl;
		cout << "rdp receive oi: " << temp_header->other_info.size() << endl;
		cout << "rdp receive message_len: " << temp_header->message_len << endl;
		m->msgFlat(write_buffer);
		cout << "Message to rdp: " << write_buffer << endl;
	}
}

void telnet_send_pipe(int *pipearray) {
	int bytes_read;
	char *read_buffer = new char[BUFSIZE];
	char *send_buffer;
	while(1) {
		bzero(read_buffer, BUFSIZE);
		bytes_read = read(pipearray[28], read_buffer, BUFSIZE);
		if(bytes_read == -1) {
			fprintf(stderr, "error reading from ip send pipe: %s\n", strerror(errno));
			exit(1);
		}
		cout << "telnet send message: " << read_buffer << endl;
		send_buffer = append_header_to_message(0, 8, read_buffer, bytes_read);
		if(write(pipearray[19], send_buffer, (bytes_read + HEADER_LEN)) == -1) {
			fprintf(stderr, "error writing to tcp send pipe: %s\n", strerror(errno));
		}
	}
}

void telnet_receive_pipe(int *pipearray) {
	char *buffer = new char[BUFSIZE];
	char *write_buffer = new char[BUFSIZE];
	while(1) {
		bzero(buffer, BUFSIZE);
		if(read(pipearray[16], buffer, BUFSIZE) == -1) {
			fprintf(stderr, "error reading from tcp receive pipe: %s\n", strerror(errno));
		}
		Message *m = new Message(buffer, BUFSIZE);
		header *temp_header = (header *)m->msgStripHdr(HEADER_LEN);
		cout << "telnet receive hlp: " << temp_header->hlp << endl;
		cout << "telnet receive oi: " << temp_header->other_info.size() << endl;
		cout << "telnet receive message_len: " << temp_header->message_len << endl;
		m->msgFlat(write_buffer);
		cout << "Message to telnet: " << write_buffer << endl;
	}
}

void ftp_send_pipe(int *pipearray) {
	int bytes_read;
	char *read_buffer = new char[BUFSIZE];
	char *send_buffer;
	while(1) {
		bzero(read_buffer, BUFSIZE);
		bytes_read = read(pipearray[26], read_buffer, BUFSIZE);
		if(bytes_read == -1) {
			fprintf(stderr, "error reading from ip send pipe: %s\n", strerror(errno));
			exit(1);
		}
		cout << "ftp send message: " << read_buffer << endl;
		send_buffer = append_header_to_message(0, 8, read_buffer, bytes_read);
		if(write(pipearray[19], send_buffer, (bytes_read + HEADER_LEN)) == -1) {
			fprintf(stderr, "error writing to ethernet send pipe: %s\n", strerror(errno));
		}
	}
}

void ftp_receive_pipe(int *pipearray) {
	char *buffer = new char[BUFSIZE];
	char *write_buffer = new char[BUFSIZE];
	while(1) {
		bzero(buffer, BUFSIZE);
		if(read(pipearray[14], buffer, BUFSIZE) == -1) {
			fprintf(stderr, "error reading from tcp receive pipe: %s\n", strerror(errno));
		}
		Message *m = new Message(buffer, BUFSIZE);
		header *temp_header = (header *)m->msgStripHdr(HEADER_LEN);
		cout << "ftp receive hlp: " << temp_header->hlp << endl;
		cout << "ftp receive oi: " << temp_header->other_info.size() << endl;
		cout << "ftp receive message_len: " << temp_header->message_len << endl;
		m->msgFlat(write_buffer);
		cout << "Message to ftp: " << write_buffer << endl;
	}
}

void udp_send_pipe(int *pipearray) {
	int bytes_read, higher_protocol_id;
	char *read_buffer = new char[BUFSIZE];
	char *read_buffer_minus_id = new char[BUFSIZE];
	char *send_buffer;
	while(1) {
		bzero(read_buffer, BUFSIZE);
		bzero(read_buffer_minus_id, BUFSIZE);
		bytes_read = read(pipearray[24], read_buffer, BUFSIZE);
		if(bytes_read == -1) {
			fprintf(stderr, "error reading from ip send pipe: %s\n", strerror(errno));
			exit(1);
		}
		higher_protocol_id =  (int)atol(&read_buffer[0]);
		cout << "ip send protocol_id: " << higher_protocol_id << endl;
		memcpy(read_buffer_minus_id, read_buffer + 1, BUFSIZE-1);
		cout << "ip send message: " << read_buffer << endl;
		send_buffer = append_header_to_message(higher_protocol_id, 12, read_buffer_minus_id, bytes_read-1);
		if(write(pipearray[7], send_buffer, (bytes_read + HEADER_LEN - 1)) == -1) {
			fprintf(stderr, "error writing to ethernet send pipe: %s\n", strerror(errno));
		}
	}
}

void udp_receive_pipe(int *pipearray) {
	char *buffer = new char[BUFSIZE];
	char *write_buffer = new char[BUFSIZE];
	while(1) {
		bzero(buffer, BUFSIZE);
		if(read(pipearray[8], buffer, BUFSIZE) == -1) {
			fprintf(stderr, "error reading from tcp receive pipe: %s\n", strerror(errno));
		}
		Message *m = new Message(buffer, BUFSIZE);
		header *temp_header = (header *)m->msgStripHdr(HEADER_LEN);
		cout << "tcp receive hlp: " << temp_header->hlp << endl;
		cout << "tcp receive oi: " << temp_header->other_info.size() << endl;
		cout << "tcp receive message_len: " << temp_header->message_len << endl;
		m->msgFlat(write_buffer);
		if(temp_header->hlp == 7) {
			if(write(pipearray[21], write_buffer, BUFSIZE) == -1) {
				fprintf(stderr, "error writing to rdp receive pipe: %s\n", strerror(errno));
			}
		}
		else if(temp_header->hlp == 8) {
			if(write(pipearray[23], write_buffer, BUFSIZE) == -1) {
				fprintf(stderr, "error writing to dns receive pipe: %s\n", strerror(errno));
			}
		}
	}
}

void tcp_send_pipe(int *pipearray) {
	int bytes_read, higher_protocol_id;
	char *read_buffer = new char[BUFSIZE];
	char *read_buffer_minus_id = new char[BUFSIZE];
	char *send_buffer;
	while(1) {
		bzero(read_buffer, BUFSIZE);
		bzero(read_buffer_minus_id, BUFSIZE);
		bytes_read = read(pipearray[18], read_buffer, BUFSIZE);
		if(bytes_read == -1) {
			fprintf(stderr, "error reading from ip send pipe: %s\n", strerror(errno));
			exit(1);
		}
		higher_protocol_id =  (int)atol(&read_buffer[0]);
		cout << "ip send protocol_id: " << higher_protocol_id << endl;
		memcpy(read_buffer_minus_id, read_buffer + 1, BUFSIZE-1);
		cout << "ip send message: " << read_buffer << endl;
		send_buffer = append_header_to_message(higher_protocol_id, 12, read_buffer_minus_id, bytes_read-1);
		if(write(pipearray[7], send_buffer, (bytes_read + HEADER_LEN - 1)) == -1) {
			fprintf(stderr, "error writing to ethernet send pipe: %s\n", strerror(errno));
		}
	}
}

void tcp_receive_pipe(int *pipearray) {
	char *buffer = new char[BUFSIZE];
	char *write_buffer = new char[BUFSIZE];
	while(1) {
		bzero(buffer, BUFSIZE);
		if(read(pipearray[10], buffer, BUFSIZE) == -1) {
			fprintf(stderr, "error reading from tcp receive pipe: %s\n", strerror(errno));
		}
		Message *m = new Message(buffer, BUFSIZE);
		header *temp_header = (header *)m->msgStripHdr(HEADER_LEN);
		cout << "tcp receive hlp: " << temp_header->hlp << endl;
		cout << "tcp receive oi: " << temp_header->other_info.size() << endl;
		cout << "tcp receive message_len: " << temp_header->message_len << endl;
		m->msgFlat(write_buffer);
		if(temp_header->hlp == 5) {
			if(write(pipearray[15], write_buffer, BUFSIZE) == -1) {
				fprintf(stderr, "error writing to ftp receive pipe: %s\n", strerror(errno));
			}
		}
		else if(temp_header->hlp == 6) {
			if(write(pipearray[17], write_buffer, BUFSIZE) == -1) {
				fprintf(stderr, "error writing to telnet receive pipe: %s\n", strerror(errno));
			}
		}
	}
}

void ip_receive_pipe(int *pipearray) {
	char *buffer = new char[BUFSIZE];
	char *write_buffer = new char[BUFSIZE];
	while(1) {
		bzero(buffer, BUFSIZE);
		if(read(pipearray[4], buffer, BUFSIZE) == -1) {
			fprintf(stderr, "error reading from ethernet receive pipe: %s\n", strerror(errno));
		}
		Message *m = new Message(buffer, BUFSIZE);
		header *temp_header = (header *)m->msgStripHdr(HEADER_LEN);
		cout << "ip receive hlp: " << temp_header->hlp << endl;
		cout << "ip receive oi: " << temp_header->other_info.size() << endl;
		cout << "ip receive message_len: " << temp_header->message_len << endl;
		m->msgFlat(write_buffer);
		if(temp_header->hlp == 3) {
			if(write(pipearray[13], write_buffer, BUFSIZE) == -1) {
				fprintf(stderr, "error writing to tcp receive pipe: %s\n", strerror(errno));
			}
		}
		else if(temp_header->hlp == 4) {
			if(write(pipearray[9], write_buffer, BUFSIZE) == -1) {
				fprintf(stderr, "error writing to udp receive pipe: %s\n", strerror(errno));
			}
		}
	}
}

void ip_send_pipe(int *pipearray) {
	int bytes_read, higher_protocol_id;
	char *read_buffer = new char[BUFSIZE];
	char *read_buffer_minus_id = new char[BUFSIZE];
	char *send_buffer;
	while(1) {
		bzero(read_buffer, BUFSIZE);
		bzero(read_buffer_minus_id, BUFSIZE);
		bytes_read = read(pipearray[12], read_buffer, BUFSIZE);
		if(bytes_read == -1) {
			fprintf(stderr, "error reading from ip send pipe: %s\n", strerror(errno));
			exit(1);
		}
		higher_protocol_id =  (int)atol(&read_buffer[0]);
		cout << "ip send protocol_id: " << higher_protocol_id << endl;
		memcpy(read_buffer_minus_id, read_buffer + 1, BUFSIZE-1);
		cout << "ip send message: " << read_buffer << endl;
		send_buffer = append_header_to_message(higher_protocol_id, 12, read_buffer_minus_id, bytes_read-1);
		if(write(pipearray[7], send_buffer, (bytes_read + HEADER_LEN - 1)) == -1) {
			fprintf(stderr, "error writing to ethernet send pipe: %s\n", strerror(errno));
		}
	}
}

void ethernet_send_pipe(int *pipearray) {
	int bytes_read;
	char *read_buffer = new char[BUFSIZE];
	char *send_buffer;
	while(1) {
		bzero(read_buffer, BUFSIZE);
		bytes_read = read(pipearray[6], read_buffer, BUFSIZE);
		if(bytes_read == -1) {
			fprintf(stderr, "error reading from ip send pipe: %s\n", strerror(errno));
			exit(1);
		}
		send_buffer = append_header_to_message(2, 8, read_buffer, bytes_read);
		if(write(pipearray[3], send_buffer, (bytes_read + HEADER_LEN)) == -1) {
			fprintf(stderr, "error writing to ethernet send pipe: %s\n", strerror(errno));
		}
	}
}

void ethernet_receive_pipe(int *pipearray) {
	char *buffer = new char[BUFSIZE];
	char *write_buffer = new char[BUFSIZE];
	while(1) {
		bzero(buffer, BUFSIZE);
		bzero(write_buffer, BUFSIZE);
		if(read(pipearray[0], buffer, BUFSIZE) == -1) {
			fprintf(stderr, "error reading from ethernet receive pipe: %s\n", strerror(errno));
		}
		Message *m = new Message(buffer, BUFSIZE);
		header *temp_header = (header *)m->msgStripHdr(HEADER_LEN);
		cout << "ethernet receive hlp: " << temp_header->hlp << endl;
		cout << "ethernet receive oi: " << temp_header->other_info.size() << endl;
		cout << "ethernet receive message_len: " << temp_header->message_len << endl;
		m->msgFlat(write_buffer);
		cout << "ethernet receive buffer: " << write_buffer << endl;
		if(write(pipearray[5], write_buffer, temp_header->message_len) == -1) {
			fprintf(stderr, "error writing to ip receive pipe: %s\n", strerror(errno));
		}
	}
}


int *create_all_protocol_pipes() {
	int i;
	int j = 0;
	int *pipearray = new int[NUM_PROTOCOL_PIPES_FDS];
	for(i = 0; i < NUM_PROTOCOL_PIPES; i++) {
		int *pipefd = new int[2];
		if(pipe(pipefd) == -1) {
			fprintf(stderr, "error creating pipe: %s\n", strerror(errno));
		}
		pipearray[j] = pipefd[0];
		j++;
		pipearray[j] = pipefd[1];
		j++;
	}
	return pipearray;
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
	if(::bind(s, (struct sockaddr *)&clientaddr, sizeof(clientaddr)) < 0) {
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

void read_from_pipe_write_to_socket(int port_number, int s, int ethernet_send_pipe_read_end) {
	char buffer[BUFSIZE];
	while(1) {
		bzero(buffer, BUFSIZE);
		if(read(ethernet_send_pipe_read_end, buffer, BUFSIZE) == -1) {
			fprintf(stderr, "error reading from send_pipe pipe: %s\n", strerror(errno));
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
	}
}

void read_from_socket_write_to_pipe(int s, int ethernet_receive_pipe_write_end) {
	char *buf = new char[BUFSIZE];
	int recvfrom_error;
	while(1) {
		bzero(buf, BUFSIZE);
		recvfrom_error = recvfrom(s, buf, BUFSIZE, 0, NULL, NULL);
		if(recvfrom_error == -1) {
			fprintf(stderr, "error receiving udp message: %s\n", strerror(errno));
		}
		if(write(ethernet_receive_pipe_write_end, buf, BUFSIZE) == -1) {
			fprintf(stderr, "error writing to ethernet receive pipe: %s\n", strerror(errno));
		}
	}
}

int main(int argc, char *argv[]) {
	int *pipearray = create_all_protocol_pipes();
	int job;
	if(argc < 2) {
		fprintf(stderr, "usage: ./process_per_protocol <job>");
		exit(1);
	}
	else {
		job = (int)atol(argv[1]);
	}

	if(job == 1) {
		int serv_in_udp_port, out_udp_socket, in_udp_socket, continue_;
		char *message = new char[BUFSIZE];
		char *received_message = new char[BUFSIZE];
		memcpy(message, "4bye!", 5);
		out_udp_socket = create_udp_socket(OUT_SOCKET_TYPE);
		in_udp_socket = create_udp_socket(IN_SOCKET_TYPE);
		cout << "Enter the port number of the server in udp socket: ";
		cin >> serv_in_udp_port;
		thread socket_receive (read_from_socket_write_to_pipe, in_udp_socket, pipearray[1]);
		thread socket_send (read_from_pipe_write_to_socket, serv_in_udp_port, out_udp_socket, pipearray[2]);
		thread ethernet_receive (ethernet_receive_pipe, pipearray);
		thread ethernet_send (ethernet_send_pipe, pipearray);
		thread ip_send (ip_send_pipe, pipearray);
		thread ip_receive (ip_receive_pipe, pipearray);
		write(pipearray[11], message, 5);
		cin >> continue_;
		read(pipearray[8], received_message, BUFSIZE);
		cout << "message: " << received_message << endl;
		socket_receive.join();
		socket_send.join();
		ethernet_receive.join();
		ethernet_send.join();
		ip_send.join();
		ip_receive.join();
	}
	else {
		int serv_in_udp_port, out_udp_socket, in_udp_socket, continue_;
		char *message = new char[BUFSIZE];
		char *received_message = new char[BUFSIZE];
		memcpy(message, "4hi!", 4);
		out_udp_socket = create_udp_socket(OUT_SOCKET_TYPE);
		in_udp_socket = create_udp_socket(IN_SOCKET_TYPE);
		cout << "Enter the port number of the server in udp socket: ";
		cin >> serv_in_udp_port;
		thread socket_receive (read_from_socket_write_to_pipe, in_udp_socket, pipearray[1]);
		thread socket_send (read_from_pipe_write_to_socket, serv_in_udp_port, out_udp_socket, pipearray[2]);
		thread ethernet_receive (ethernet_receive_pipe, pipearray);
		thread ethernet_send (ethernet_send_pipe, pipearray);
		thread ip_send (ip_send_pipe, pipearray);
		thread ip_receive (ip_receive_pipe, pipearray);
		write(pipearray[11], message, 4);
		cin >> continue_;
		read(pipearray[8], received_message, BUFSIZE);
		cout << "message: " << received_message << endl;
		socket_receive.join();
		socket_send.join();
		ethernet_receive.join();
		ethernet_send.join();
		ip_send.join();
		ip_receive.join();
	}
	return 0;
}
