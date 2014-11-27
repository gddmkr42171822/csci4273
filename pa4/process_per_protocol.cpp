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
#define NUM_PROTOCOL_PIPES 16
#define NUM_PROTOCOL_PIPES_FDS 32
using namespace std;


/* Pipearray designations:
	0,1 (ethernet recieve pipe read,write)
	2,3 (ethernet send pipe read, write)
	4,5 (ip receive pipe read, write)
	6,7 (ip send pipe read, write)
	8,9 (upd receive pipe read, write)
	10,11 (udp send pipe read, write)
	12, 13 (tcp receive pipe read, write)
	14, 15 (tcp send pipe read, write)
	16, 17 (ftp receive pipe read, write)
	18, 19 (ftp send pipe read, write)
	20, 21 (telnet receive pipe read, write)
	22, 23 (telnet send pipe read, write)
	24, 25 (RDP receive pipe read, write)
	26, 27 (RDP send pipe read, write)
	28, 29 (DNS receive pipe read, write)
	30, 31 (DNS send pipe read, write)
*/

struct header {
	int hlp;
	vector<char> other_info;
	int message_len;
};

void write_to_receive_pipe(int *pipearray, int hlp, char *write_buffer) {
	switch (hlp) {
		case 2: //write to IPs receive pipe
			if(write(pipearray[5], write_buffer, BUFSIZE) == -1) {
				fprintf(stderr, "error writing to ip receive pipe: %s\n", strerror(errno));
			}
			break;
		case 3: //write to TCP's receive pipe
			if(write(pipearray[13], write_buffer, BUFSIZE) == -1) {
				fprintf(stderr, "error writing to tcp receive pipe: %s\n", strerror(errno));
			}
			break;
		case 4: //write to UDP's receive pipe
			if(write(pipearray[9], write_buffer, BUFSIZE) == -1) {
				fprintf(stderr, "error writing to udp receive pipe: %s\n", strerror(errno));
			}
			break;
		case 5: //write to FTP's receive pipe
			if(write(pipearray[17], write_buffer, BUFSIZE) == -1) {
				fprintf(stderr, "error writing to ftp receive pipe: %s\n", strerror(errno));
			}
			break;
		case 6: //write to telent's receive pipe
			if(write(pipearray[21], write_buffer, BUFSIZE) == -1) {
				fprintf(stderr, "error writing to telnet receive pipe: %s\n", strerror(errno));
			}
			break;
		case 7: //write to RDP's receive pipe
			if(write(pipearray[25], write_buffer, BUFSIZE) == -1) {
				fprintf(stderr, "error writing to rdp receive pipe: %s\n", strerror(errno));
			}
			break;
		case 8: //write to DNS's receive pipe
			if(write(pipearray[29], write_buffer, BUFSIZE) == -1) {
				fprintf(stderr, "error writing to dns receive pipe: %s\n", strerror(errno));
			}
			break;
	}
}

void read_from_receive_pipe(int receive_pipe_read_end, int *pipearray) {
	char *buffer = new char[BUFSIZE];
	char *write_buffer = new char[BUFSIZE];
	while(1) {
		bzero(buffer, BUFSIZE);
		if(read(receive_pipe_read_end, buffer, BUFSIZE) == -1) {
			fprintf(stderr, "error reading from ethernet receive pipe: %s\n", strerror(errno));
		}
		Message *m = new Message(buffer, BUFSIZE);
		header *temp_header = (header *)m->msgStripHdr(HEADER_LEN);
		cout << "hlp: " << temp_header->hlp << endl;
		cout << "oi: " << temp_header->other_info.size() << endl;
		m->msgFlat(write_buffer);
		write_to_receive_pipe(pipearray, temp_header->hlp, write_buffer);
	}
}

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

int get_protocol_other_info(int higher_protocol_id) {
	int other_info;
	cout << "higher_protocol_id: " << higher_protocol_id << endl;
	switch (higher_protocol_id) {
		case 2: //ip
			other_info = 8;//ethernet header
			break;
		case 3: //tcp
			other_info = 12;//ip header
			break;
		case 4: //udp
			other_info = 12;//ip header
			break;
		case 5: //ftp
			other_info = 4;//tcp header
			break;
		case 6: //telnet
			other_info = 4;//tcp header
			break;
		case 7: //RDP
			other_info = 4;//udp header
			break;
		case 8: //DNS
			other_info = 4;//udp header
			break;
	}
	return other_info;
}

void read_from_and_write_to_send_pipe(int send_pipe_write_end, int send_pipe_read_end) {
	int bytes_read, higher_protocol_id, other_info;
	char *read_buffer = new char[BUFSIZE];
	char *read_buffer_minus_id = new char[BUFSIZE];
	char *send_buffer;
	while(1) {
		bzero(read_buffer, BUFSIZE);
		bytes_read = read(send_pipe_read_end, read_buffer, BUFSIZE);
		if(bytes_read == -1) {
			fprintf(stderr, "error reading from ip send pipe: %s\n", strerror(errno));
			exit(1);
		}
		higher_protocol_id =  (int)atol(&read_buffer[0]);
		cout << "protocol_id: " << higher_protocol_id << endl;;
		other_info = get_protocol_other_info(higher_protocol_id);
		cout << "other_info: " << other_info << endl;
		memcpy(read_buffer_minus_id, read_buffer + 1, BUFSIZE-1);
		send_buffer = append_header_to_message(higher_protocol_id, other_info, read_buffer_minus_id, bytes_read-1);
		if(write(send_pipe_write_end, send_buffer, BUFSIZE) == -1) {
			fprintf(stderr, "error writing to ethernet send pipe: %s\n", strerror(errno));
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
		char *buffer;
		char *received_message = new char[BUFSIZE];
		strncpy(message, "bye!", 4);
		buffer = append_header_to_message(2, get_protocol_other_info(2), message, 4);
		out_udp_socket = create_udp_socket(OUT_SOCKET_TYPE);
		in_udp_socket = create_udp_socket(IN_SOCKET_TYPE);
		cout << "Enter the port number of the server in udp socket: ";
		cin >> serv_in_udp_port;
		write(pipearray[3], buffer, BUFSIZE);
		thread socket_receive (read_from_socket_write_to_pipe, in_udp_socket, pipearray[1]);
		thread socket_send (read_from_pipe_write_to_socket, serv_in_udp_port, out_udp_socket, pipearray[2]);
		thread ethernet_receive (read_from_receive_pipe, pipearray[0], pipearray);
		//thread ethernet_send (read_from_and_write_to_send_pipe, pipearray[3], pipearray[4]);
		cin >> continue_;
		read(pipearray[4], received_message, BUFSIZE);
		cout << "message: " << received_message << endl;
		socket_receive.join();
		socket_send.join();
		ethernet_receive.join();
		//ethernet_send.join();
	}
	else {
		int serv_in_udp_port, out_udp_socket, in_udp_socket, continue_;
		char *message = new char[BUFSIZE];
		char *buffer;
		char *received_message = new char[BUFSIZE];
		strncpy(message, "hi!", 3);
		buffer = append_header_to_message(2, get_protocol_other_info(2), message, 3);
		out_udp_socket = create_udp_socket(OUT_SOCKET_TYPE);
		in_udp_socket = create_udp_socket(IN_SOCKET_TYPE);
		cout << "Enter the port number of the server in udp socket: ";
		cin >> serv_in_udp_port;
		write(pipearray[3], buffer, BUFSIZE);
		thread socket_receive (read_from_socket_write_to_pipe, in_udp_socket, pipearray[1]);
		thread socket_send (read_from_pipe_write_to_socket, serv_in_udp_port, out_udp_socket, pipearray[2]);
		thread ethernet_receive (read_from_receive_pipe, pipearray[0], pipearray);
		//thread ethernet_send (read_from_and_write_to_send_pipe, pipearray[3], pipearray[4]);
		cin >> continue_;
		read(pipearray[4], received_message, BUFSIZE);
		cout << "message: " << received_message << endl;
		socket_receive.join();
		socket_send.join();
		ethernet_receive.join();
		//ethernet_send.join();
	}
	return 0;
}
