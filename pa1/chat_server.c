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

struct Messages {
	int socket;
	char last_message[80];
	int last_index;
};

int udp_communication(int udpport, int server_address, char *session_name);
int tcp_communication(int ssock, char chat_history[][80], int *history_index, struct Messages *client_histories, int *client_history_index);

int main(int argc, char *argv[]) {
	int errno, fd, i;

	/* Create index to keep track of clients in session */
	int index = 0;
	int client_index = 0;
	int *client_history_index;
	client_history_index = &client_index;

	int current_index = 0;
	int *current_client_index;
	current_client_index = &current_index;

	/* Create buffer and index to keep track of chat history */
	int *history_index;
	history_index = &index;
	char chat_history[30][80];

	struct sockaddr_in clientaddr;

	/* Create a struct to hold last received messages of clients and clear out those messages */
	struct Messages client_histories[10];
	for(i = 0; i < 10; i++) {
		memset(client_histories[i].last_message, '\0', sizeof(client_histories[i].last_message));
		client_histories[i].last_index = -1;
	}

	struct timeval tv = { 0, 0};

	socklen_t alen = sizeof(clientaddr);
	fprintf(stdout, "\n--------This is chat_server executing-------\n");

	/* Get session name from args */
	char session_name[8];
	strcpy(session_name, argv[3]);
	fprintf(stdout, "This session's name is %s\n", session_name);


	int server_udpport = atoi(argv[1]);
	int msock = atoi(argv[0]);
	int coordaddress = atoi(argv[2]);

	fprintf(stdout, "\nThis is the master socket: %d\n", msock);

	/* Set up file descriptor table to hold clients in session */
	fd_set rfds;
	fd_set afds;
	int nfds;
	
	nfds = getdtablesize();
	FD_ZERO(&afds);
	FD_SET(msock, &afds);
	int select_error;

	/* Loop for communication between different Clients */
	while(1) {

		tv.tv_sec = 20;

		memcpy(&rfds, &afds, sizeof(rfds));

		if ((select_error = select(nfds, &rfds, (fd_set *)0, (fd_set *)0, &tv))) {
			fprintf(stderr, "Select: %s\n", strerror(errno));
		}
		else if(select_error == 0) {
			fprintf(stderr, "\nSocket timed out ---> session to be terminated with error %s\n", strerror(errno));
			udp_communication(server_udpport, coordaddress, session_name);
			shutdown(msock, SHUT_RDWR);
			(void) close(msock);
			exit(EXIT_SUCCESS);
		}

		/* If the master socket has already been created */
		if (FD_ISSET(msock, &rfds)) {

			int	ssock;
			fprintf(stdout, "\nAccepting a connection from a client...\n");
			ssock = accept(msock, (struct sockaddr *)&clientaddr, &alen);
			fprintf(stdout, "Client connection accepted\n");
			if(ssock < 0) {
				fprintf(stderr, "Accept failed with error: %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}
			else {
				FD_SET(ssock, &afds);

				socklen_t socklen = sizeof(clientaddr);
				if ((getsockname(ssock, (struct sockaddr *)&clientaddr, &socklen)) < 0) {
					fprintf(stderr, "getsockname: %s\n", strerror(errno));
				}
			
				/* Add clients to session history */
				int client_exists = 0;
				fprintf(stdout, "\nAdding client to client session history\n");
				for(i = 0;i <= *client_history_index; i++) {
					if(client_histories[i].socket == ssock) {
						fprintf(stderr, "Client %d has already been in this session\n", ssock);
						*current_client_index = *current_client_index + 1;
						client_exists = 1;
						break;
					}
				}

				if(!client_exists) {
					client_histories[*client_history_index].socket = ssock;
					*client_history_index = *client_history_index + 1;
					*current_client_index = *current_client_index + 1;
					fprintf(stdout, "\nAdding client %d to client history, history index now at %d\n", ssock, *client_history_index);
				}
			}
		}
		for (fd=0; fd<nfds; ++fd) {
			if (fd != msock && FD_ISSET(fd, &rfds)) {
				if (tcp_communication(fd, chat_history, history_index, client_histories, client_history_index) == 0) {
					(void) close(fd);
					*current_client_index = *current_client_index - 1;
					fprintf(stdout, "\nAfter a client left the session, there are |%d| clients in the session\n", *current_client_index);
					FD_CLR(fd, &afds);
					fprintf(stdout, "Client %d left session \n", fd);
				}
			}
		}
	}
	return 0;
}

int tcp_communication(int ssock, char chat_history[][80], int *history_index, struct Messages *client_histories, int *client_history_index) {
	int messagelen;
	int i, j;
	char buf[BUFSIZE];

	
	memset(buf, '\0', sizeof(buf));
	messagelen = read(ssock, buf, sizeof(buf));
	if(messagelen < 0) {
		fprintf(stderr, "Server read failure %s \n", strerror(errno));
	}
	else {
		fprintf(stdout, "Received message from client %d---: %s\n", ssock, buf);

		/* Add submitted messages to chat history */
		if((strncmp(buf, "Submit", strlen("Submit"))) == 0) {

			char message[80];
			size_t current_size = 0;

			memset(message, '\0', sizeof(message));
			fprintf(stdout, "\n-----Submitting message to history-----\n");

			int i;
			for(i = 7;i < messagelen-1; i++)  {
				message[current_size++] = ((char)buf[i]);
			}

			/* Add message to chat history */
			memset(chat_history[*history_index], '\0', sizeof(chat_history[*history_index]));
			strncpy(chat_history[*history_index], message, strlen(message));

			fprintf(stdout, "Message |%s| put into chat history\n", chat_history[*history_index]);
			fprintf(stdout, "There are |%d| messages in the chat history\n", *history_index+1);

			fprintf(stdout, "\nCopying message to client %d history struct\n", ssock);

			for( i = 0; i < *client_history_index; i++) {
				if(client_histories[i].socket == ssock) {

					memset(client_histories[i].last_message, '\0', sizeof(client_histories[i].last_message));
					strncpy(client_histories[i].last_message, chat_history[*history_index], strlen(chat_history[*history_index]));
					client_histories[i].last_index = *history_index;

					break; 
				}
			}

			messagelen = write(ssock, chat_history[*history_index], strlen(chat_history[*history_index]));
			if(messagelen < 0) {
				fprintf(stderr, "Server write failure %s\n", strerror(errno));
			}

			fprintf(stdout,"Client %d last received message was : %s at index in chat history %d\n", client_histories[i].socket, client_histories[i].last_message, client_histories[i].last_index);
			*history_index = *history_index + 1;
			return messagelen;
		}
		else if((strncmp(buf, "GetNext", strlen("GetNext"))) == 0) {

			/* Get first message not read by client */
			/* Find Client */
			for(i = 0; i < *client_history_index; i++) {	
				if(client_histories[i].socket == ssock) {

					char getnext_output[80];

					/* Match last message of client to chat history and print next message in chat history */

					/* If ther are no messages in chat history */
					if(*history_index == 0) {
						messagelen = write(ssock, "No new messages in the chat session", strlen("No new messages in the chat session"));
						return messagelen;
					}

					/* Client hasn't submitted any messages add first message in chat history to client message history */
					else if((strcmp(client_histories[i].last_message, "")) == 0) {

						strncpy(client_histories[i].last_message, chat_history[0], strlen(chat_history[0]));
						client_histories[i].last_index = 0;
						memset(getnext_output, '\0', sizeof(getnext_output));
						sprintf(getnext_output, "%zu", strlen(chat_history[0]));
						strcat(getnext_output, " ");
						strcat(getnext_output, chat_history[0]);
						fprintf(stdout, "\nThe getnext message being sent back to client is |%s|\n", getnext_output);

						//messagelen = write(ssock, chat_history[0], strlen(chat_history[0]));
						messagelen = write(ssock, getnext_output, strlen(getnext_output));
						if(messagelen < 0) {
							fprintf(stderr, "Server write failure %s\n", strerror(errno));
						}
						return messagelen;
					}
					else {
						for(j = (client_histories[i].last_index); j < *history_index; j++) {

							fprintf(stdout, "\nCompare client last message |%s| to message in chat history |%s|\n", client_histories[i].last_message, chat_history[j]);

							if((strncmp(client_histories[i].last_message, chat_history[j], strlen(client_histories[i].last_message))) == 0) { 
								fprintf(stdout, "The compared messages match\n"); 
							}

							/* The client last message is not the last message in client history */	
							if(((strncmp(client_histories[i].last_message, chat_history[j], strlen(client_histories[i].last_message))) == 0) && ((j) < (*history_index-1))) {

								fprintf(stdout, "\nThe client last message is not the last message in the chat history\n");

								/* Write message back */
								memset(getnext_output, '\0', sizeof(getnext_output));
								sprintf(getnext_output, "%zu", strlen(chat_history[j+1]));
								strcat(getnext_output, " ");
								strcat(getnext_output, chat_history[j+1]);
								fprintf(stdout, "\nThe getnext message being sent back to client is |%s|\n", getnext_output);

								messagelen = write(ssock, getnext_output, strlen(getnext_output));

								/* copy message to client message history */
								memset(client_histories[i].last_message, '\0', sizeof(client_histories[i].last_message));
								strncpy(client_histories[i].last_message, chat_history[j+1], strlen(chat_history[j+1]));
								client_histories[i].last_index = j+1;

								if(messagelen < 0) {
									fprintf(stderr, "Server write failure %s\n", strerror(errno));
								}

								return messagelen;
							} /* if the client's last message is the last message in the chat history do nothing */
							else if(((strncmp(client_histories[i].last_message, chat_history[*history_index-1], strlen(client_histories[i].last_message))) == 0) && (j == (*history_index-1))) {
								fprintf(stdout, "client last message is last message in chat history\n");

								memset(getnext_output, '\0', sizeof(getnext_output));

								strncpy(getnext_output, "No new messages in the chat session", strlen("No new messages in the chat session"));
								messagelen = write(ssock, getnext_output, strlen("No new messages in the chat session"));

								if(messagelen < 0) {
									fprintf(stderr, "Server write failure %s\n", strerror(errno));
								}

								return messagelen;
							}
						}
					}
				}
			}
		}
		else if((strncmp(buf, "GetAll", strlen("GetAll"))) == 0) {

			/* Get all messages not read by client */
			/* Find Current Client messages history */
			for(i = 0; i < *client_history_index; i++) {
				if(client_histories[i].socket == ssock) {
					
					/* No messages in chat history */
					if(*history_index == 0) {
						messagelen = write(ssock, "No new messages in the chat session", strlen("No new messages in the chat session"));
						return messagelen;
					}/* Client hasn't submitted any messages */
					else if(strlen(client_histories[i].last_message) == 0) {

						fprintf(stdout, "\nGetAll: Client hasn't submitted any messages\n");

						char getall_output[BUFSIZE];
						memset(getall_output, '\0', sizeof(getall_output));

						/* Send number of messages that will be sent to client */
						sprintf(getall_output, "%d", *history_index);
						fprintf(stdout, "Number of messages to be sent to client |%s|\n", getall_output);


						messagelen = write(ssock, getall_output, strlen(getall_output));
						if(messagelen < 0) {
							fprintf(stderr, "Server write failure %s\n", strerror(errno));
						}

						fprintf(stdout, "Sent number of messages |%s| to client\n", getall_output);

						memset(getall_output, '\0', sizeof(getall_output));
						messagelen = read(ssock, getall_output, sizeof(getall_output));
						fprintf(stdout, "GetAll message received: %s\n", getall_output);

						int num_messages = atoi(getall_output);
						int k;
						for(k = 0; k < num_messages;k++) {
							memset(getall_output, '\0', sizeof(getall_output));
							fprintf(stdout, "Write message |%s| to client, message number |%d|\n", chat_history[k], k + 1);
							sprintf(getall_output, "%zu", strlen(chat_history[k]));
							strcat(getall_output, " ");
							strcat(getall_output, chat_history[k]);
							messagelen = write(ssock, getall_output, strlen(getall_output));
							memset(getall_output, '\0', sizeof(getall_output));
							messagelen = read(ssock, getall_output, sizeof(getall_output));
							fprintf(stdout, "Received message |%s| from client\n", getall_output);
						}
						/* Copy last read message into client history */
						memset(client_histories[i].last_message, '\0', sizeof(client_histories[i].last_message));
						strncpy(client_histories[i].last_message, chat_history[*history_index-1], strlen(chat_history[*history_index-1]));
						client_histories[i].last_index = (*history_index-1);

						fprintf(stdout, "After GetAll command, last client message is |%s|\n", client_histories[i].last_message);
						
						return messagelen;
					}
					else {
						for(j = (client_histories[i].last_index); j < *history_index; j++) {

							fprintf(stdout, "\nGetAll looking through client message history\n");

							fprintf(stdout, "Compare client last message |%s| to message in chat history |%s|\n", client_histories[i].last_message, chat_history[j]);

							if((strncmp(client_histories[i].last_message, chat_history[j], strlen(client_histories[i].last_message))) == 0 ) { 
								fprintf(stdout, "They match\n"); 
							}

							/* Client doesn't have last message in message history */
							if(((strncmp(client_histories[i].last_message, chat_history[j], strlen(client_histories[i].last_message))) == 0) && ((j) < (*history_index-1))) {

								fprintf(stdout, "\nIn GetAll, client doesn't have last message in chat history\n");

								char getall_output[BUFSIZE];
								memset(getall_output, '\0', sizeof(getall_output));

								/* Send number of messages that will be sent to client */
								sprintf(getall_output, "%d", (*history_index-(j+1)));
								fprintf(stdout, "Number of messages to be sent to client |%s|\n", getall_output);


								messagelen = write(ssock, getall_output, strlen(getall_output));
								if(messagelen < 0) {
									fprintf(stderr, "Server write failure %s\n", strerror(errno));
								}

								fprintf(stdout, "Sent number of messages |%s| to client\n", getall_output);

								memset(getall_output, '\0', sizeof(getall_output));
								messagelen = read(ssock, getall_output, sizeof(getall_output));
								fprintf(stdout, "GetAll message received: %s\n", getall_output);

								int f;
								int g = 0;
								for(f = j + 1; f < *history_index;f++) {
									memset(getall_output, '\0', sizeof(getall_output));
									fprintf(stdout, "\nWrite message |%s| to client, message number |%d|\n", chat_history[f], ++g);
									sprintf(getall_output, "%zu", strlen(chat_history[f]));
									strcat(getall_output, " ");
									strcat(getall_output, chat_history[f]);
									messagelen = write(ssock, getall_output, strlen(getall_output));
									memset(getall_output, '\0', sizeof(getall_output));
									messagelen = read(ssock, getall_output, sizeof(getall_output));
									fprintf(stdout, "Received message |%s| from client\n", getall_output);
								}
								/* Copy last read message into client history */
								memset(client_histories[i].last_message, '\0', sizeof(client_histories[i].last_message));
								strncpy(client_histories[i].last_message, chat_history[*history_index-1], strlen(chat_history[*history_index-1]));
								client_histories[i].last_index = (*history_index-1);

								fprintf(stdout, "\nAfter GetAll command, last client message is |%s|\n", client_histories[i].last_message);

								return messagelen;
							} // if the last message is the last message in the chat history do nothing
							else if(((strncmp(client_histories[i].last_message, chat_history[*history_index-1], strlen(client_histories[i].last_message))) == 0) && (j == (*history_index-1))) {
								fprintf(stdout, "\nIn GetAll, client has last message in chat history\n");
								char getnext_output[40];

								memset(getnext_output, '\0', sizeof(getnext_output));

								strncpy(getnext_output, "No new messages in the chat session", strlen("No new messages in the chat session"));
								messagelen = write(ssock, getnext_output, strlen("No new messages in the chat session"));
								if(messagelen < 0) {
									fprintf(stderr, "Server write failure %s\n", strerror(errno));
								}
								return messagelen;
							}
						}
					}
				}
			}
		}
		else if((strncmp(buf, "Leave", strlen("Leave"))) == 0) {
			return 0;
		}
		else {
			/* If no chat commands are used just echo back what client sent */
			fprintf(stdout, "\nChat session echo message to enable client to send...\n");
			messagelen = write(ssock, buf, strlen(buf));
			if(messagelen < 0) {
				fprintf(stderr, "Server write failure %s\n", strerror(errno));
			}

			return messagelen;
		}
	}	
	return 0;
}

int udp_communication(int udpport, int server_address, char *session_name) {
	struct sockaddr_in chat_coordaddr;
	int s;

	memset(&chat_coordaddr, 0, sizeof(chat_coordaddr));

	chat_coordaddr.sin_family = AF_INET;
	chat_coordaddr.sin_addr.s_addr = server_address;
	chat_coordaddr.sin_port = htons(udpport);

	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		fprintf(stderr, "cannot create socket %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	char buf[BUFSIZE];
	memset(buf, '\0', sizeof(buf));
	strcat(buf, "Terminate ");
	strcat(buf, session_name);
	
	fprintf(stdout, "\nChat_server sending %s to chat_coordinator\n", buf);
	if((sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&chat_coordaddr, sizeof(chat_coordaddr))) < 0){
		fprintf(stderr, "sendto error %s\n", strerror(errno));
	}

	fprintf(stdout, "Sent chat_server Terminate udp message!\n");
	return 0;
}
