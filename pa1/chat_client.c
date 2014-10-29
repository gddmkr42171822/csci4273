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

void tcp_communication(int tcp_s, char *session_name);


int main(int argc, char *argv[]) {
	
	char *portnum;
	char *host;
	int s, tcp_s, errno, recvlen;
	
	if(argc > 2) {
		host = argv[1];
		portnum = argv[2];
	}
	else {
		fprintf(stderr, "Usage: ./chat_client <host> <portnum>\n");
		exit(EXIT_FAILURE);
	}
	
	char buf[BUFSIZE];
	struct hostent *phe;
	struct sockaddr_in clientaddr, servaddr, tcp_servaddr;
	socklen_t addrlen = sizeof(servaddr);

	
	memset(&servaddr, 0, sizeof(servaddr));
	memset(&tcp_servaddr, 0, sizeof(tcp_servaddr));
    servaddr.sin_family = AF_INET;
    tcp_servaddr.sin_family = AF_INET;
	
	/* Map port number (char string) to port number (int)*/
    if ((servaddr.sin_port=htons((unsigned short)atoi(portnum))) == 0)
		fprintf(stderr,"can't get \"%s\" port number\n", portnum);

    /* Map host name to IP address, allowing for dotted decimal */
    if ((phe = gethostbyname(host)) != NULL) {
		memcpy(&servaddr.sin_addr, phe->h_addr, phe->h_length);
		memcpy(&tcp_servaddr.sin_addr, phe->h_addr, phe->h_length);
	}
    else {
		fprintf(stderr, "can't get \"%s\" host ip address\n", host);
	}
		
	/* Create UDP Socket, Fill Remote Struct, and Bind */
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		fprintf(stderr, "cannot create socket %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	memset(&clientaddr, 0, sizeof(clientaddr));
	clientaddr.sin_family = AF_INET;
	clientaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	clientaddr.sin_port = htons(0);
	
	if ((bind(s, (struct sockaddr *)&clientaddr, sizeof(clientaddr))) < 0) {
                fprintf(stderr, "can't bind: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
	}


	char session_name[BUFSIZE];

	memset(buf, '\0', sizeof(buf));
	fprintf(stdout, "\nClient enter command (Find <session> or Start <session>) to begin chat program: \n");

	/* Wait On Stdin And Handle Client Commands */
	while((fgets(buf, sizeof(buf), stdin)) != NULL) {

		if((strncmp(buf, "Start", strlen("Start"))) == 0) {

			memset(session_name, '\0', sizeof(session_name));
			
			strncpy(session_name, buf, strlen(buf));
			if(sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&servaddr, addrlen) < 0){
				fprintf(stderr, "sendto error %s\n", strerror(errno));
			}

			fprintf(stdout, "Sent udp message!\n");
			fprintf(stdout, "Client, wait for udp response...\n");

			memset(buf, '\0', sizeof(buf));
			recvlen = recvfrom(s, buf, BUFSIZE, 0, (struct sockaddr *)&servaddr, &addrlen);
			if (recvlen > 0) {
					fprintf(stdout, "received udp message: %s\n", buf);
				}
				else {
					fprintf(stderr, "recv failed %s\n", strerror(errno));
				}

				/* Check Received Messages For -1 Return */
				if((strncmp(buf, "-1", strlen("-1"))) == 0) {
					fprintf(stdout, "Session already exists\n");
				}
				else {
					/* Put New TCP Port In Servaddr Struct */
					if((tcp_servaddr.sin_port = htons((unsigned short)atoi(buf))) == 0) {
						fprintf(stderr, "can't get port number %s for tcp socket \n", buf);
					}
					else {
						fprintf(stdout, "Start TCP Servaddr (chat session) port is now %d\n", ntohs(tcp_servaddr.sin_port)); 
					}

					/* Create TCP Socket And Connect To Chat Session Server TCP Port */
					if ((tcp_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
						fprintf(stderr, "cannot create socket %s\n", strerror(errno));
						exit(EXIT_FAILURE);
					}
					 
					if((connect(tcp_s, (struct sockaddr *)&tcp_servaddr, sizeof(tcp_servaddr))) < 0) {
						fprintf(stderr, "can't connect to tcp server socket %s\n", strerror(errno));
						exit(EXIT_FAILURE);
					}

					fprintf(stdout, "START: TCP connected! You have created a new chat session\n");
					/* Start TCP Communication Between Chat_client And Chat_server */
					tcp_communication(tcp_s, session_name);
				}
			}
			else if ((strncmp(buf, "Find", strlen("Find"))) == 0) {

				memset(session_name, '\0', sizeof(session_name));


				strncpy(session_name, buf, strlen(buf));

				if(sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&servaddr, addrlen) < 0){
					fprintf(stderr, "sendto error %s\n", strerror(errno));
				}

				fprintf(stdout, "Sent udp message!\n");
				fprintf(stdout, "Client wait udp response...\n");

				memset(buf, '\0', sizeof(buf));

				recvlen = recvfrom(s, buf, BUFSIZE, 0, (struct sockaddr *)&servaddr, &addrlen);
				if (recvlen > 0) {
					fprintf(stdout, "received udp message: %s\n", buf);
				}
				else {
					fprintf(stderr, "recv failed %s\n", strerror(errno));
				}

				/* Check for -1 return */
				if((strncmp(buf, "-1", strlen("-1"))) == 0) {
					fprintf(stdout, "Session has not yet been created\n");
				}
				else {
					/* Put new port in servaddr struct */
					if((tcp_servaddr.sin_port = htons((unsigned short)atoi(buf))) == 0) {
						fprintf(stderr, "can't get port number %s for tcp socket \n", buf);
					}
					else {
						fprintf(stdout, "Join TCP Servaddr (chat session) port is now %d\n", ntohs(tcp_servaddr.sin_port)); 
					}

					/* Connect to new struct by creating and binding to new socket then connecting */
					if ((tcp_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
						fprintf(stderr, "cannot create socket %s\n", strerror(errno));
						exit(EXIT_FAILURE);
					}
					 
					if((connect(tcp_s, (struct sockaddr *)&tcp_servaddr, sizeof(tcp_servaddr))) < 0) {
						fprintf(stderr, "can't connect to tcp server socket %s\n", strerror(errno));
						exit(EXIT_FAILURE);
					}

					fprintf(stdout, "\nFIND: TCP connected! You have joined the chat session.");
					tcp_communication(tcp_s, session_name);
				}
			}
			else {
				fprintf(stderr, "\nChat command not found\n");
			}

			fprintf(stdout, "\nWaiting on udp stdin...\n");
			fprintf(stdout, "Client, enter command (Find <session> or Start <session>) to begin udp chat: \n");
		}
	exit(EXIT_SUCCESS);
}

void tcp_communication(int tcp_s, char *session_name) {
	char tcp_buf[BUFSIZE];
	int messagelen;

	/* Send and receive messages through created tcp connection */
	while(1) {
						
		fprintf(stdout, "\nClient, send tcp message:\n");

		/* Clear buffer and put stdin into it */
		memset(tcp_buf, '\0', sizeof(tcp_buf));
		fgets(tcp_buf, sizeof(tcp_buf), stdin);

		/* Handle leave and exit commands when client is in a chat session */
		if((strncmp(tcp_buf, "Leave", strlen("Leave"))) == 0) {
			char chat_session[8];
			int i;
			size_t current_size = 0;
			size_t bufferlen = strlen(session_name);

			memset(chat_session, '\0', sizeof(chat_session));

			if((strncmp(session_name, "Find", strlen("Find"))) == 0) {
				/* Remove find command */
				for(i = 5;i < bufferlen-1; i++) {
					chat_session[current_size++] = ((char)session_name[i]);
				}
			}

			if((strncmp(session_name, "Start", strlen("Start"))) == 0) {
				/* Remove find command */
				for(i = 6;i < bufferlen-1; i++) {
					chat_session[current_size++] = ((char)session_name[i]);
				}
			}

			fprintf(stdout, "\nYou have left the chat session |%s| \n", chat_session);
			messagelen = write(tcp_s, tcp_buf, strlen(tcp_buf));
			if(messagelen < 0) {
				fprintf(stderr, "Server write failure %s\n", strerror(errno));
			}
			break;
		}
		else if((strncmp(tcp_buf, "Exit", strlen("Exit"))) == 0) {
			exit(EXIT_SUCCESS);
		}
		else if((strncmp(tcp_buf, "GetAll", strlen("GetAll"))) == 0) {

			messagelen = write(tcp_s, tcp_buf, strlen(tcp_buf));
			if(messagelen < 0) {
				fprintf(stderr, "Server write failure %s\n", strerror(errno));
			}

			memset(tcp_buf, '\0', sizeof(tcp_buf));

			/* Get number of messages to be received */
			messagelen = read(tcp_s, tcp_buf, sizeof(tcp_buf));
			/* Check if chat session has timed out */
			if(strlen(tcp_buf) == 0) {
				fprintf(stderr, "\nTCP chat session error...closing session\n");
				(void) close(tcp_s);
				break;
			}

			if((strncmp(tcp_buf, "No new messages in the chat session", strlen("No new messages in the chat session"))) == 0) {
				fprintf(stdout, "Received tcp message: %s\n", tcp_buf);
				fprintf(stdout, "Size of tcp message was %zd\n", strlen(tcp_buf));
				
			}
			else {
				int num_messages = atoi(tcp_buf);

				/* Write back that number */
				fprintf(stdout, "\nGetAll: Number of Messages to be received %d\n", num_messages);

				memset(tcp_buf, '\0', sizeof(tcp_buf));
				sprintf(tcp_buf, "%d", num_messages);

				messagelen = write(tcp_s, tcp_buf, strlen(tcp_buf));
				if(messagelen < 0) {
					fprintf(stderr, "Server write failure %s\n", strerror(errno));
				}

				fprintf(stdout, "Wrote back number of messages %s\n", tcp_buf);

				
				memset(tcp_buf, '\0', sizeof(tcp_buf));


				int k;
				for(k = 0; k < num_messages; k++) {
					memset(tcp_buf, '\0', sizeof(tcp_buf));
					fprintf(stdout, "Receiving message\n");
					messagelen = read(tcp_s, tcp_buf, sizeof(tcp_buf));
					fprintf(stdout, "message number: |%d| message received: |%s|\n", (k + 1), tcp_buf);
					messagelen = write(tcp_s, tcp_buf, strlen(tcp_buf));
				}

			}
		}
		else {
			/* Write to server */
			messagelen = write(tcp_s, tcp_buf, strlen(tcp_buf));
			if(messagelen < 0) {
				fprintf(stderr, "Server write failure %s\n", strerror(errno));
			}


			fprintf(stdout, "\nClient waiting to receive tcp message: \n");
			memset(tcp_buf, '\0', sizeof(tcp_buf));

			/* Read response from server */
			messagelen = read(tcp_s, tcp_buf, sizeof(tcp_buf));
			if(messagelen < 0) {
				fprintf(stderr, "Server read failure %s \n", strerror(errno));
			}
			else {
				fprintf(stdout, "Received tcp message: %s\n", tcp_buf);
				fprintf(stdout, "Size of tcp message was %zd\n", strlen(tcp_buf));
				
				/* Check if chat session has timed out */
				if(strlen(tcp_buf) == 0) {
					fprintf(stderr, "TCP chat session error...closing session\n");
					(void) close(tcp_s);
					break;
				}
			}
		}
	}
}
