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
#include "message.h"

#define BUFSIZE 1024 
#define DEFAULT_NUM_THREADS 30
#define IN_SOCKET_TYPE 2
#define OUT_SOCKET_TYPE 1
#define HEADER_LEN 40
#define NUM_PROTOCOL_PIPES 20
#define NUM_PROTOCOL_PIPES_FDS 40
#define NUM_MESSAGES 100 
using namespace std;

/* Pipearray designations:
	0, 1 (ethernet recieve pipe read,write)
	2, 3 (ethernet send pipe read, write)
	4, 5 (ip receive pipe read, write)
	6, 7 (ip send pipe read, write)
	8, 9 (udp receive pipe read, write)
	10, 11 (tcp receive pipe read, write)
	12, 13 (tcp send pipe read, write)
	14, 15 (ftp receive pipe read, write)
	16, 17 (telnet receive pipe read, write)
	18, 19 (telnet send pipe read, write)
	20, 21 (rdp receive pipe read, write)
	22, 23 (dns receive pipe read, write)
	24, 25 (rdp send pipe read, write)
	26, 27 (application_message_ftp send pipe read, write)
	28, 29 (application_message_telnet send pipe read, write)
	30, 31 (application_message_rdp send pipe read, write)
	32, 33 (application_message_dns send pipe read, write)
	34, 35 (ftp send pipe read, write)
	36, 37 (dns send pipe read, write)
	38, 39 (udp send pipe read, write)
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
	delete m;
	return send_buffer;
}

void application_to_dns(int *pipearray) {
	char *message = new char[BUFSIZE];
	memcpy(message, "8", 1);
	for(int i = 0;i < NUM_MESSAGES;i++) {
		write(pipearray[33], message, BUFSIZE);
		usleep(5000);// 5 milliseconds
	}
}

void application_to_rdp(int *pipearray) {
	char *message = new char[BUFSIZE];
	memcpy(message, "7", 1);
	for(int i = 0;i < NUM_MESSAGES;i++) {
		write(pipearray[31], message, BUFSIZE);
		usleep(5000);// 5 milliseconds
	}

}

void application_to_telnet(int *pipearray) {
	char *message = new char[BUFSIZE];
	memcpy(message, "6", 1);
	for(int i = 0;i < NUM_MESSAGES;i++) {
		write(pipearray[29], message, BUFSIZE);
		usleep(5000);// 5 milliseconds
	}

}

void application_to_ftp(int *pipearray) {
	char *message = new char[BUFSIZE];
	memcpy(message, "5", 1);
	for(int i = 0;i < NUM_MESSAGES;i++) {
		write(pipearray[27], message, BUFSIZE);
		usleep(5000);// 5 milliseconds
	}
}

void dns_send_pipe(int *pipearray) {
	int bytes_read;
	char *read_buffer = new char[BUFSIZE];
	char *temp_buffer;
	char *send_buffer = new char[BUFSIZE];
	while(1) {
		bzero(read_buffer, BUFSIZE);
		bzero(send_buffer, BUFSIZE);
		bytes_read = read(pipearray[32], read_buffer, BUFSIZE);
		if(bytes_read == -1) {
			fprintf(stderr, "error reading from application_dns send pipe: %s\n", strerror(errno));
			exit(1);
		}
		temp_buffer = append_header_to_message(0, 8, read_buffer, bytes_read);
		memcpy(send_buffer, "8", 1);
		memcpy(send_buffer + 1, temp_buffer, (HEADER_LEN + bytes_read));
		if(write(pipearray[37], send_buffer, BUFSIZE) == -1) {
			fprintf(stderr, "error writing to rdp/dns send pipe: %s\n", strerror(errno));
		}
	}
}

void dns_receive_pipe(int *pipearray) {
	int bytes_read;
	char *buffer = new char[BUFSIZE];
	char *write_buffer = new char[BUFSIZE];
	int i = 0;
	while(1) {
		bzero(buffer, BUFSIZE);
		bzero(write_buffer, BUFSIZE);
		if((bytes_read = read(pipearray[22], buffer, BUFSIZE)) == -1) {
			fprintf(stderr, "error reading from dns receive pipe: %s\n", strerror(errno));
		}
		Message *m = new Message(buffer, bytes_read);
		header *temp_header = (header *)m->msgStripHdr(HEADER_LEN);
		(void) temp_header;
		m->msgFlat(write_buffer);
		write_buffer[temp_header->message_len] = '\n';
		i++;
		cout << write_buffer;
		if(i == NUM_MESSAGES) {
			cout << i << endl;
			return;
		}
		delete m;
	}
}

void rdp_send_pipe(int *pipearray) {
	int bytes_read;
	char *read_buffer = new char[BUFSIZE];
	char *temp_buffer;
	char *send_buffer = new char[BUFSIZE];
	while(1) {
		bzero(read_buffer, BUFSIZE);
		bzero(send_buffer, BUFSIZE);
		bytes_read = read(pipearray[30], read_buffer, BUFSIZE);
		if(bytes_read == -1) {
			fprintf(stderr, "error reading from application_rdp send pipe: %s\n", strerror(errno));
			exit(1);
		}
		temp_buffer = append_header_to_message(0, 12, read_buffer, bytes_read);
		memcpy(send_buffer, "7", 1);
		memcpy(send_buffer + 1, temp_buffer, (HEADER_LEN + bytes_read));
		if(write(pipearray[25], send_buffer, BUFSIZE) == -1) {
			fprintf(stderr, "error writing to rdp/dns send pipe: %s\n", strerror(errno));
		}
	}
}

void rdp_receive_pipe(int *pipearray) {
	int bytes_read;
	char *buffer = new char[BUFSIZE];
	char *write_buffer = new char[BUFSIZE];
	int i = 0;
	while(1) {
		bzero(buffer, BUFSIZE);
		bzero(write_buffer, BUFSIZE);
		if((bytes_read = read(pipearray[20], buffer, BUFSIZE)) == -1) {
			fprintf(stderr, "error reading from rdp receive pipe: %s\n", strerror(errno));
		}
		Message *m = new Message(buffer, bytes_read);
		header *temp_header = (header *)m->msgStripHdr(HEADER_LEN);
		(void) temp_header;
		m->msgFlat(write_buffer);
		write_buffer[temp_header->message_len] = '\n';
		i++;
		cout << write_buffer;
		if(i == NUM_MESSAGES) {
			cout << i << endl;
			return;
		}
		delete m;
	}
}

void telnet_send_pipe(int *pipearray) {
	int bytes_read;
	char *read_buffer = new char[BUFSIZE];
	char *temp_buffer;
	char *send_buffer = new char[BUFSIZE];
	while(1) {
		bzero(read_buffer, BUFSIZE);
		bzero(send_buffer, BUFSIZE);
		bytes_read = read(pipearray[28], read_buffer, BUFSIZE);
		if(bytes_read == -1) {
			fprintf(stderr, "error reading from application telnet send pipe: %s\n", strerror(errno));
			exit(1);
		}
		temp_buffer = append_header_to_message(0, 8, read_buffer, bytes_read);
		memcpy(send_buffer, "6", 1);
		memcpy(send_buffer + 1, temp_buffer, (HEADER_LEN + bytes_read));
		if(write(pipearray[19], send_buffer, BUFSIZE) == -1) {
			fprintf(stderr, "error writing to telnet send pipe: %s\n", strerror(errno));
		}
	}
}

void telnet_receive_pipe(int *pipearray) {
	int bytes_read;
	char *buffer = new char[BUFSIZE];
	char *write_buffer = new char[BUFSIZE];
	int i = 0;
	while(1) {
		bzero(buffer, BUFSIZE);
		bzero(write_buffer, BUFSIZE);
		if((bytes_read = read(pipearray[16], buffer, BUFSIZE)) == -1) {
			fprintf(stderr, "error reading from telnet receive pipe: %s\n", strerror(errno));
		}
		Message *m = new Message(buffer, bytes_read);
		header *temp_header = (header *)m->msgStripHdr(HEADER_LEN);
		(void) temp_header;
		m->msgFlat(write_buffer);
		write_buffer[temp_header->message_len] = '\n';
		i++;
		cout << write_buffer;
		if(i == NUM_MESSAGES) {
			cout << i << endl;
			return;
		}
		delete m;
	}
}

void ftp_send_pipe(int *pipearray) {
	int bytes_read;
	char *read_buffer = new char[BUFSIZE];
	char *temp_buffer;
	char *send_buffer = new char[BUFSIZE];
	while(1) {
		bzero(read_buffer, BUFSIZE);
		bzero(send_buffer, BUFSIZE);
		bytes_read = read(pipearray[26], read_buffer, BUFSIZE);
		if(bytes_read == -1) {
			fprintf(stderr, "error reading from application ftp send pipe: %s\n", strerror(errno));
			exit(1);
		}
		temp_buffer = append_header_to_message(0, 8, read_buffer, bytes_read);
		memcpy(send_buffer, "5", 1);
		memcpy(send_buffer + 1, temp_buffer, (HEADER_LEN + bytes_read));
		if(write(pipearray[35], send_buffer, BUFSIZE) == -1) {
			fprintf(stderr, "error writing to ftp send pipe: %s\n", strerror(errno));
		}
	}
}

void ftp_receive_pipe(int *pipearray) {
	int bytes_read;
	char *buffer = new char[BUFSIZE];
	char *write_buffer = new char[BUFSIZE];
	int i = 0;
	while(1) {
		bzero(buffer, BUFSIZE);
		bzero(write_buffer, BUFSIZE);
		if((bytes_read = read(pipearray[14], buffer, BUFSIZE)) == -1) {
			fprintf(stderr, "error reading from ftp receive pipe: %s\n", strerror(errno));
		}
		Message *m = new Message(buffer, bytes_read);
		header *temp_header = (header *)m->msgStripHdr(HEADER_LEN);
		(void) temp_header;
		m->msgFlat(write_buffer);
		write_buffer[temp_header->message_len] = '\n';
		i++;
		cout << write_buffer;
		if(i == NUM_MESSAGES) {
			cout << i << endl;
			return;
		}
		delete m;
	}
}

void udp_send_pipe(int *pipearray) {
	int bytes_read, higher_protocol_id;
	char *read_buffer = new char[BUFSIZE];
	char *read_buffer_minus_id = new char[BUFSIZE];
	char *temp_buffer;
	char *send_buffer = new char[BUFSIZE];
	int read_dns_messages = 0;
	int read_rdp_messages = 0;
	while(1) {
		bzero(read_buffer, BUFSIZE);
		bzero(read_buffer_minus_id, BUFSIZE);
		bzero(send_buffer, BUFSIZE);
		if(read_rdp_messages < NUM_MESSAGES) {
			bytes_read = read(pipearray[24], read_buffer, BUFSIZE);
			read_rdp_messages++;
			if(bytes_read == -1) {
				fprintf(stderr, "error reading from telnet send pipe: %s\n", strerror(errno));
				exit(1);
			}
			higher_protocol_id =  (int)atol(&read_buffer[0]);
			memcpy(read_buffer_minus_id, read_buffer + 1, BUFSIZE-1);
			temp_buffer = append_header_to_message(higher_protocol_id, 4, read_buffer_minus_id, bytes_read-1);
			memcpy(send_buffer, "4", 1);
			memcpy(send_buffer + 1, temp_buffer, (HEADER_LEN + bytes_read-1));
			if(write(pipearray[39], send_buffer, BUFSIZE) == -1) {
				fprintf(stderr, "error writing to udp/tcp send pipe: %s\n", strerror(errno));
			}
		}
		else if(read_dns_messages < NUM_MESSAGES) {
			bytes_read = read(pipearray[36], read_buffer, BUFSIZE);
			read_dns_messages++;
			if(bytes_read == -1) {
				fprintf(stderr, "error reading from ftp send pipe: %s\n", strerror(errno));
				exit(1);
			}
			higher_protocol_id =  (int)atol(&read_buffer[0]);
			memcpy(read_buffer_minus_id, read_buffer + 1, BUFSIZE-1);
			temp_buffer = append_header_to_message(higher_protocol_id, 4, read_buffer_minus_id, bytes_read-1);
			memcpy(send_buffer, "4", 1);
			memcpy(send_buffer + 1, temp_buffer, (HEADER_LEN + bytes_read-1));
			if(write(pipearray[39], send_buffer, BUFSIZE) == -1) {
				fprintf(stderr, "error writing to udp/tcp send pipe: %s\n", strerror(errno));
			}
		}
	}
}

void udp_receive_pipe(int *pipearray) {
	int bytes_read;
	char *buffer = new char[BUFSIZE];
	char *write_buffer = new char[BUFSIZE];
	while(1) {
		bzero(buffer, BUFSIZE);
		bzero(write_buffer, BUFSIZE);
		if((bytes_read = read(pipearray[8], buffer, BUFSIZE)) == -1) {
			fprintf(stderr, "error reading from udp receive pipe: %s\n", strerror(errno));
		}
		Message *m = new Message(buffer, bytes_read);
		header *temp_header = (header *)m->msgStripHdr(HEADER_LEN);
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
		delete m;
	}
}

void tcp_send_pipe(int *pipearray) {
	int bytes_read, higher_protocol_id;
	char *read_buffer = new char[BUFSIZE];
	char *read_buffer_minus_id = new char[BUFSIZE];
	char *temp_buffer;
	char *send_buffer = new char[BUFSIZE];
	int read_telnet_messages = 0;
	int read_ftp_messages = 0;
	while(1) {
		bzero(read_buffer, BUFSIZE);
		bzero(read_buffer_minus_id, BUFSIZE);
		bzero(send_buffer, BUFSIZE);
		if(read_telnet_messages < NUM_MESSAGES) {
			bytes_read = read(pipearray[18], read_buffer, BUFSIZE);
			read_telnet_messages++;
			if(bytes_read == -1) {
				fprintf(stderr, "error reading from telnet send pipe: %s\n", strerror(errno));
				exit(1);
			}
			higher_protocol_id =  (int)atol(&read_buffer[0]);
			memcpy(read_buffer_minus_id, read_buffer + 1, BUFSIZE-1);
			temp_buffer = append_header_to_message(higher_protocol_id, 4, read_buffer_minus_id, bytes_read-1);
			memcpy(send_buffer, "3", 1);
			memcpy(send_buffer + 1, temp_buffer, (HEADER_LEN + bytes_read-1));
			if(write(pipearray[13], send_buffer, BUFSIZE) == -1) {
				fprintf(stderr, "error writing to udp/tcp send pipe: %s\n", strerror(errno));
			}
		}
		else if(read_ftp_messages < NUM_MESSAGES) {
			bytes_read = read(pipearray[34], read_buffer, BUFSIZE);
			read_ftp_messages++;
			if(bytes_read == -1) {
				fprintf(stderr, "error reading from ftp send pipe: %s\n", strerror(errno));
				exit(1);
			}
			higher_protocol_id =  (int)atol(&read_buffer[0]);
			memcpy(read_buffer_minus_id, read_buffer + 1, BUFSIZE-1);
			temp_buffer = append_header_to_message(higher_protocol_id, 4, read_buffer_minus_id, bytes_read-1);
			memcpy(send_buffer, "3", 1);
			memcpy(send_buffer + 1, temp_buffer, (HEADER_LEN + bytes_read-1));
			if(write(pipearray[13], send_buffer, BUFSIZE) == -1) {
				fprintf(stderr, "error writing to udp/tcp send pipe: %s\n", strerror(errno));
			}
		}
	}
}

void tcp_receive_pipe(int *pipearray) {
	int bytes_read;
	char *buffer = new char[BUFSIZE];
	char *write_buffer = new char[BUFSIZE];
	while(1) {
		bzero(buffer, BUFSIZE);
		bzero(write_buffer, BUFSIZE);
		if((bytes_read =read(pipearray[10], buffer, BUFSIZE)) == -1) {
			fprintf(stderr, "error reading from tcp receive pipe: %s\n", strerror(errno));
		}
		Message *m = new Message(buffer, bytes_read);
		header *temp_header = (header *)m->msgStripHdr(HEADER_LEN);
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
		delete m;
	}
}

void ip_receive_pipe(int *pipearray) {
	int bytes_read;
	char *buffer = new char[BUFSIZE];
	char *write_buffer = new char[BUFSIZE];
	while(1) {
		bzero(buffer, BUFSIZE);
		bzero(write_buffer, BUFSIZE);
		if((bytes_read =read(pipearray[4], buffer, BUFSIZE)) == -1) {
			fprintf(stderr, "error reading from ip receive pipe: %s\n", strerror(errno));
		}
		Message *m = new Message(buffer, bytes_read);
		header *temp_header = (header *)m->msgStripHdr(HEADER_LEN);
		m->msgFlat(write_buffer);
		if(temp_header->hlp == 3) {
			if(write(pipearray[11], write_buffer, BUFSIZE) == -1) {
				fprintf(stderr, "error writing to tcp receive pipe: %s\n", strerror(errno));
			}
		}
		else if(temp_header->hlp == 4) {
			if(write(pipearray[9], write_buffer, BUFSIZE) == -1) {
				fprintf(stderr, "error writing to udp receive pipe: %s\n", strerror(errno));
			}
		}
		delete m;
	}
}

void ip_send_pipe(int *pipearray) {
	int bytes_read, higher_protocol_id;
	char *read_buffer = new char[BUFSIZE];
	char *read_buffer_minus_id = new char[BUFSIZE];
	char *send_buffer;
	int read_tcp_messages = 0;
	int read_udp_messages = 0;
	while(1) {
		bzero(read_buffer, BUFSIZE);
		bzero(read_buffer_minus_id, BUFSIZE);
		if(read_tcp_messages < NUM_MESSAGES*2) {
			bytes_read = read(pipearray[12], read_buffer, BUFSIZE);
			read_tcp_messages++;
			if(bytes_read == -1) {
				fprintf(stderr, "error reading from tcp send pipe: %s\n", strerror(errno));
				exit(1);
			}
			higher_protocol_id =  (int)atol(&read_buffer[0]);
			memcpy(read_buffer_minus_id, read_buffer + 1, BUFSIZE-1);
			send_buffer = append_header_to_message(higher_protocol_id, 12, read_buffer_minus_id, bytes_read-1);
			if(write(pipearray[7], send_buffer, BUFSIZE) == -1) {
				fprintf(stderr, "error writing to ip send pipe: %s\n", strerror(errno));
			}
		}
		else if(read_udp_messages < NUM_MESSAGES*2) {
			bytes_read = read(pipearray[38], read_buffer, BUFSIZE);
			read_udp_messages++;
			if(bytes_read == -1) {
				fprintf(stderr, "error reading from udp send pipe: %s\n", strerror(errno));
				exit(1);
			}
			higher_protocol_id =  (int)atol(&read_buffer[0]);
			memcpy(read_buffer_minus_id, read_buffer + 1, BUFSIZE-1);
			send_buffer = append_header_to_message(higher_protocol_id, 12, read_buffer_minus_id, bytes_read-1);
			if(write(pipearray[7], send_buffer, BUFSIZE) == -1) {
				fprintf(stderr, "error writing to ip send pipe: %s\n", strerror(errno));
			}
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
		if(write(pipearray[3], send_buffer, BUFSIZE) == -1) {
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
		(void) temp_header;
		m->msgFlat(write_buffer);
		if(write(pipearray[5], write_buffer, BUFSIZE) == -1) {
			fprintf(stderr, "error writing to ip receive pipe: %s\n", strerror(errno));
		}
		delete m;
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

void socket_send(int port_number, int s, int ethernet_send_pipe_read_end) {
	int bytes_read;
	char buffer[BUFSIZE];
	char *send_buffer = new char[BUFSIZE];
	while(1) {
		bzero(buffer, BUFSIZE);
		bzero(send_buffer, BUFSIZE);
		if((bytes_read = read(ethernet_send_pipe_read_end, buffer, BUFSIZE)) == -1) {
			fprintf(stderr, "error reading from send_pipe pipe: %s\n", strerror(errno));
		}
		send_buffer = append_header_to_message(1, 0, buffer, bytes_read);
		int sendto_error;
		struct sockaddr_in servaddr;
		bzero(&servaddr, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servaddr.sin_port = htons(port_number);
		sendto_error = sendto(s, send_buffer, BUFSIZE, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
		if(sendto_error == -1) {
			fprintf(stderr, "error sending udp message: %s\n", strerror(errno));
		}
	}
}

void socket_receive(int s, int ethernet_receive_pipe_write_end) {
	char *buf = new char[BUFSIZE];
	char *write_buffer = new char [BUFSIZE];
	int recvfrom_error;
	while(1) {
		bzero(buf, BUFSIZE);
		bzero(write_buffer, BUFSIZE);
		recvfrom_error = recvfrom(s, buf, BUFSIZE, 0, NULL, NULL);
		if(recvfrom_error == -1) {
			fprintf(stderr, "error receiving udp message: %s\n", strerror(errno));
		}
		Message *m = new Message(buf, recvfrom_error);
		header *temp_header = (header *)m->msgStripHdr(HEADER_LEN);
		(void) temp_header;
		m->msgFlat(write_buffer);
		if(write(ethernet_receive_pipe_write_end, write_buffer, BUFSIZE) == -1) {
			fprintf(stderr, "error writing to ethernet receive pipe: %s\n", strerror(errno));
		}
		delete m;
	}
}

int main() {
	int *pipearray = create_all_protocol_pipes();
	int serv_in_udp_port, out_udp_socket, in_udp_socket, continue_;
	out_udp_socket = create_udp_socket(OUT_SOCKET_TYPE);
	in_udp_socket = create_udp_socket(IN_SOCKET_TYPE);
	cout << "Enter the port number of the server in udp socket: ";
	cin >> serv_in_udp_port;
	thread socket_r (socket_receive, in_udp_socket, pipearray[1]);
	thread socket_s (socket_send, serv_in_udp_port, out_udp_socket, pipearray[2]);
	thread ethernet_receive (ethernet_receive_pipe, pipearray);
	thread ethernet_send (ethernet_send_pipe, pipearray);
	thread ip_send (ip_send_pipe, pipearray);
	thread ip_receive (ip_receive_pipe, pipearray);
	thread tcp_send (tcp_send_pipe, pipearray);
	thread tcp_receive (tcp_receive_pipe, pipearray);
	thread dns_send (dns_send_pipe, pipearray);
	thread udp_send (udp_send_pipe, pipearray);
	thread udp_receive (udp_receive_pipe, pipearray);
	thread ftp_send (ftp_send_pipe, pipearray);
	thread ftp_receive (ftp_receive_pipe, pipearray);
	thread telnet_send (telnet_send_pipe, pipearray);
	thread telnet_receive (telnet_receive_pipe, pipearray);
	thread rdp_send (rdp_send_pipe, pipearray);
	thread rdp_receive (rdp_receive_pipe, pipearray);
	thread dns_receive (dns_receive_pipe, pipearray);
	cout << "Write message? ";
	cin >> continue_;
	sleep(3);
	struct timeval begin, end;
	gettimeofday(&begin, NULL);
	thread application_telnet (application_to_telnet, pipearray);
	application_telnet.join();
	thread application_ftp (application_to_ftp, pipearray);
	application_ftp.join();
	thread application_rdp (application_to_rdp, pipearray);
	application_rdp.join();
	thread application_dns (application_to_dns, pipearray);
	application_dns.join();
	telnet_receive.join();
	ftp_receive.join();
	rdp_receive.join();
	dns_receive.join();
	gettimeofday(&end, NULL);
	double elapsed = (end.tv_sec - begin.tv_sec) + ((end.tv_usec - begin.tv_usec)/1000000.0);
	cout << "elapsed time: " << elapsed << " seconds\n" << endl;
	cout << "End program? ";
	cin >> continue_;
	socket_r.join();
	socket_s.join();
	ethernet_receive.join();
	ethernet_send.join();
	ip_send.join();
	ip_receive.join();
	tcp_send.join();
	tcp_receive.join();
	udp_send.join();
	udp_receive.join();
	ftp_send.join();
	telnet_send.join();
	rdp_send.join();
	dns_send.join();
	return 0;
}
