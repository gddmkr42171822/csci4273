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

#define BUFSIZE 4096

struct Session {
	int port;
	char session_name[8];
};

int messagehandler(char *buffer, int bufferlen, struct Session *socket_sessions, int *session_index, int server_udpport, int coordinator_address);
int start_tcpsession(char *message, struct Session *socket_sessions , int *session_index, int server_udpport, int coordinator_address);



int main(int argc, char *argv[]) {

	/* Create struct to hold session names and sockets */
	struct Session socket_sessions[10];

	/* Create index to keep track of sessions */
	int session_num = 0;
	int *session_index = &session_num;
	
	struct sockaddr_in servaddr, clientaddr;
	int s;
	int errno, recvlen;
	socklen_t addrlen = sizeof(clientaddr);
	unsigned short portnum = 8888;
	char buf[BUFSIZE];
	
	/* Create udp socket and bind */
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		fprintf(stderr, "cannot create UDP socket %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(portnum);
	
	if(bind(s, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {

		fprintf(stderr, "can't bind to %d port: %s; Trying other port\n", portnum, strerror(errno));
		
		servaddr.sin_port=htons(0);
		
		if ((bind(s, (struct sockaddr *)&servaddr, sizeof(servaddr))) < 0) {
                fprintf(stderr, "can't bind: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
		}
        else {
            socklen_t socklen = sizeof(servaddr);

            if ((getsockname(s, (struct sockaddr *)&servaddr, &socklen)) < 0) {
                fprintf(stderr, "getsockname: %s\n", strerror(errno));
			}

            fprintf(stdout, "\nNew server port number is %d\n", ntohs(servaddr.sin_port));
        }
	}
	else {
			fprintf(stdout, "\nChat coordinator bound to port %d\n", ntohs(servaddr.sin_port));
	}
	
	int tcp_port = 0;

	while (1) {

		fprintf(stdout, "\nChat_coordinator, waiting for a response...\n");
		memset(buf, '\0', sizeof(buf));

		/* Receive udp response from client */
		recvlen = recvfrom(s, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &addrlen);
		if (recvlen > 0) {
			fprintf(stdout, "UDP Message Receive size %d\n", recvlen);
			//buf[recvlen] = '\0';
			fprintf(stdout, "UDP received message: %s\n", buf);
			
			/* Handle udp messages/commands from client */
			tcp_port = messagehandler(buf, recvlen, socket_sessions, session_index, servaddr.sin_port, servaddr.sin_addr.s_addr);

		}
		else {
			fprintf(stderr, "recv failed %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		fprintf(stdout, "Chat_coordinator, enter message to send...\n");
		fprintf(stdout, "Newly created TCP port is %d\n", tcp_port);

		memset(buf, '\0', sizeof(buf));

		/* Send new port back to client */
		sprintf(buf, "%d", tcp_port);

		if(sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr, addrlen) < 0){
			fprintf(stderr, "sendto error %s\n", strerror(errno));
		}
	}
}

int messagehandler(char *buffer, int bufferlen, struct Session *socket_sessions, int *session_index, int server_udpport, int coordinator_address) {

	char session[8];
	memset(session, '\0', sizeof(session));
	int tcp_port = 0;
	size_t current_size = 0;

	fprintf(stdout, "\nMessage handler has message => %s\n", buffer);
	fprintf(stdout, "The length of the buffer and null character is %d\n", bufferlen);
	
	if((strncmp(buffer, "Start", strlen("Start"))) == 0) {
		fprintf(stdout, "Client trying to Start a session\n");
		int i;
		for(i = 6;i < bufferlen-1; i++)  {
			session[current_size++] = ((char)buffer[i]);
		}

		fprintf(stdout, "Start Session Name: %s\n", session);

		/* Check if session already exists */
		if(*session_index > 0) {
			for(i = 0; i < *session_index; i++) {
				fprintf(stdout, "\nSearch through sessions, index at: %d\n", i);
				if((strcmp(socket_sessions[i].session_name, session)) == 0) {
					return -1;
				}
			}

			/* Add session to sessions struct */
			strcpy(socket_sessions[*session_index].session_name, session);
			tcp_port = start_tcpsession(session, socket_sessions, session_index, server_udpport, coordinator_address);
		}
		else {

			strcpy(socket_sessions[*session_index].session_name, session);
			tcp_port = start_tcpsession(session, socket_sessions, session_index, server_udpport, coordinator_address);
		}
	}
	else if((strncmp(buffer, "Find", strlen("Find"))) == 0) {

		fprintf(stdout, "Client tring to Find a session\n");

		/* Remove find command */
		int i;
		for(i = 5;i < bufferlen-1; i++) {
			session[current_size++] = ((char)buffer[i]);
		}

		fprintf(stdout, "Find Session Name %s\n", session);

		for(i = 0; i < *session_index; i++) {

			fprintf(stdout, "\nSearch through sessions in find, index at: %d\n", i);
			fprintf(stdout, "Finding session name %s\n", session);

			/* Send client tcp port if session already exists */
			if((strcmp(socket_sessions[i].session_name, session)) == 0) {
				tcp_port = socket_sessions[i].port;
				return tcp_port;
			}
		}
		/* Session doesn't exist */
		return -1;
	}
	else if((strncmp(buffer, "Terminate", strlen("Terminate")))== 0) {

		fprintf(stdout, "\nChat session server timed out, need to terminate a session\n");

		int i;
		for(i = 10;i < bufferlen; i++) {
			session[current_size++] = ((char)buffer[i]);
		}

		fprintf(stdout, "\nTerminating Session Name... %s\n", session);

		/* Remove session from sessions struct */
		for(i = 0; i <= *session_index; i++) {

			if((strcmp(socket_sessions[i].session_name, session)) == 0) {

				fprintf(stderr, "Found session %s in socket_sessions to terminate\n", socket_sessions[i].session_name);

				memset(socket_sessions[i].session_name, '\0', sizeof(socket_sessions[i].session_name));
				socket_sessions[i].port = -1;

				fprintf(stderr, "Session deleted!\n");

				return -1;
			}
		}
	}
	else {

		fprintf(stderr, "Command not recognized\n");
	}
	return tcp_port;
}

int start_tcpsession(char *message, struct Session *socket_sessions, int *session_index, int server_udpport, int coordinator_address) {
	
	fprintf(stdout, "\n-----In start_tcpsession function----\n");
	int tcp_port = 0;
	int s;
	struct sockaddr_in tcp_servaddr;
	
	/* Start new tcp session */
	/* Create and bind socket */
	if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		fprintf(stderr, "cannot create TCP socket %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	memset(&tcp_servaddr, 0, sizeof(tcp_servaddr));
	tcp_servaddr.sin_family = AF_INET;
	tcp_servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	tcp_servaddr.sin_port = htons(0);
	
	if ((bind(s, (struct sockaddr *)&tcp_servaddr, sizeof(tcp_servaddr))) < 0) {
                fprintf(stderr, "can't bind: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
	}
	
	socklen_t socklen = sizeof(tcp_servaddr);

	/* Get tcp port out of address struct created in main */
    if ((getsockname(s, (struct sockaddr *)&tcp_servaddr, &socklen)) < 0) {
		fprintf(stderr, "getsockname: %s\n", strerror(errno));
	}
	else {
		tcp_port = ntohs(tcp_servaddr.sin_port);
		fprintf(stdout, "\nTCP server port number is %d\n", tcp_port);
	}
	
	socket_sessions[*session_index].port = tcp_port;	
	fprintf(stdout, "\nSocket session name is %s and socket session port is %d\n", socket_sessions[*session_index].session_name, socket_sessions[*session_index].port);
	fprintf(stdout, "Session index at %d\n", *session_index);
	*session_index = *session_index + 1;


	/* Listen on socket */
	if((listen(s, 5)) < 0 ) {
		fprintf(stderr, "chat server can't listen on port %d with error %s\n", s, strerror(errno));
		exit(EXIT_FAILURE);
	}
	else {
		fprintf(stdout, "chat server listening to master socket %d\n", s);
	}
	
	/* Fork and send new socket to session server */
	pid_t pid;
	char str_s[BUFSIZE];
	char str_udpport[BUFSIZE];
	char str_coordaddress[BUFSIZE];
	sprintf(str_s, "%d", s);
	sprintf(str_udpport, "%d", ntohs(server_udpport));
	sprintf(str_coordaddress, "%d", coordinator_address);
	
	if ((pid = fork()) == -1) exit(1); /* FORK FAILED */ 
	if (pid == 0) {

		execl("chat_server", str_s, str_udpport, str_coordaddress, message, NULL); 
		exit(EXIT_SUCCESS);

	}
	return tcp_port;
}

