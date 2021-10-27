#include <arpa/inet.h>
#include <err.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

#include "include/common.h"
#include "include/msg_struct.h"
#include "include/client_command.h"
#include "include/image.h"

int socket_and_connect(char *hostname, char *port) {
	int sock_fd = -1;
	// Create a socket
	if (-1 == (sock_fd = socket(AF_INET, SOCK_STREAM, 0))) {
		perror("Socket");
		exit(EXIT_FAILURE);
	}
	struct addrinfo hints, *address, *tmp;
	tmp = address;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICSERV;

	int error = getaddrinfo(hostname, port, &hints, &address);
	if (error) {
		errx(1, "%s", gai_strerror(error));
		exit(EXIT_FAILURE);
	}

	while (address != NULL) {
		if (address->ai_addr->sa_family == AF_INET) {
			if (-1 == connect(sock_fd, address->ai_addr, address->ai_addrlen)) {
				perror("Connect");
				exit(EXIT_FAILURE);
			}
			fflush(stdout);
			printf("Connection established\n");
			return sock_fd;
		}
		address = address->ai_next;
	}
	freeaddrinfo(tmp);
	return -1;
}

int receive_serv(struct message* msg, char* buff, struct pollfd* pollfds, int sockfd, int fd_transfert, char* port, char* nick_name, char* file_name){
	memset(msg, 0, sizeof(struct message));
	memset(buff, 0, MSG_LEN); // Cleaning memory

	if(-1 == read_from_socket(sockfd, msg, sizeof(struct message))){
		printf("Server shutdown !\n");
		return - 1; // server disconnected
	}

	if(msg->type < FILE_ACCEPT || msg->type == IMAGE_REQUEST)
		read_from_socket(sockfd, buff, MSG_LEN);

	switch (msg->type) {
		case NICKNAME_NEW:
			if(!strcmp(buff, "Your new pseudo is ") || !strcmp(buff, "Welcome on the chat"))
				printf("[%s]: %s %s\n", msg->nick_sender, buff, msg->infos);
			else
				server_ask_nick(msg, buff, nick_name, sockfd);
			break;
		case BROADCAST_SEND:
			printf("[%s] > all: %s\n", msg->nick_sender, buff);
			break;
		case UNICAST_SEND:
		case ECHO_SEND:
			printf("[%s]: %s \n", msg->nick_sender, buff);
			break;
		case MULTICAST_SEND:
			printf("[%s]> %s: %s \n", msg->nick_sender, msg->infos, buff);
			break;
		case MULTICAST_JOIN:
			printf("[%s] > %s: %s\n", msg->nick_sender, msg->infos, buff);
			break;
		case FILE_REQUEST:
		case IMAGE_REQUEST:
			if(!(strcmp(buff, "wants you to accept the transfer of the file named"))){
					strcpy(file_name, msg->infos);
					printf("%s %s \"%s\". Do you accept ? [Y/N]\n", msg->nick_sender, buff, file_name);
					if(msg->type == FILE_REQUEST)
						msg->type = FILE_ACCEPT;
					else
						msg->type = IMAGE_ACCEPT;

					fd_transfert = accept_file_transfert(msg, buff, nick_name, sockfd, port);
					pollfds[2].fd = fd_transfert;
				}
			else{
					printf("%s %s\n", msg->infos, buff);
			}
			break;
		case FILE_ACCEPT:
		case IMAGE_ACCEPT:
			;
			char* addr_transfert = strtok(msg->infos, ":");
			char* port_transfert = strtok(NULL, ".");
			if (-1 == (fd_transfert = socket_and_connect(addr_transfert, port_transfert))) {
				perror("FILE_ACCEPT");
			}
			if(msg->type == FILE_ACCEPT)
				file_sending(msg, buff, nick_name, file_name, fd_transfert);
			else
				image_sending(msg, buff, nick_name, file_name, fd_transfert);
			break;
		case FILE_REJECT:
			printf("%s cancelled file transfer\n", msg->nick_sender);
			break;
		case FILE_ACK:
			printf("%s has received the file\n", msg->nick_sender);
			break;
		default:
			break;
	}

	return 0;
}

int receive_data(struct message* msg, char* buff, char* nick_name, char* file_name, int fd_transfert, int sockfd){
	memset(msg, 0, sizeof(struct message));
	memset(buff, 0, MSG_LEN); // Cleaning memory

	int port = -1;
	char address[16] = {0};
	int client_fd = accept_new_connexion(fd_transfert, address, &port);

	if(-1 == read_from_socket(client_fd, msg, sizeof(struct message))){
		printf("Connexion closed !\n");
		return -1;
	}
	if(msg->type == FILE_SEND)
		file_receiving(msg, buff, nick_name, file_name, client_fd, sockfd);
	else
		image_receving(msg, buff, nick_name, file_name, client_fd, sockfd);

	printf("file %s stored in inbox/%s\n", file_name, nick_name);

	close(client_fd);
	close(fd_transfert);

	return 0;
}

int use_command(struct message* msg, char* buff, struct pollfd* pollfds, int sockfd, char* nick_name, char* channel, char* file_name){
	memset(buff, 0, MSG_LEN); // Cleaning memory

	int n = 0;
	while (n < MSG_LEN && (buff[n++] = getchar()) != '\n') {} // client typing

	buff[n-1] = '\0'; // ignore "\n"
	msg->pld_len = n;
	strcpy(msg->nick_sender, nick_name);

	char tmp[n];
	strcpy(tmp, buff);
	char* command = strtok(tmp, " ");

	if(command == NULL) // nothing usefull has been typing, avoid seg.fault
		return 0;

	else if(!(strcmp(command, "/nick"))){ // rename user
		char* new = buff+strlen(command)+1;
		if (0 == test_nick(new)){
			change_nick(msg, buff, nick_name, new, sockfd);
		}
	}

	else if(!(strcmp(command, "/who"))) // ask who is online
		who_is_online(msg, buff, nick_name, sockfd);

	else if(!(strcmp(command, "/whois"))){ // ask who is this user
		char* target = strtok(NULL, " ");
		if(target == NULL)
			printf("Usage: /whois <username>\n");
		else
			who_is_usr(msg, buff, nick_name, target, sockfd);
	}

	else if(!(strcmp(command, "/msgall"))) // message for all users
		msg_all(msg, buff+strlen(command)+1, nick_name, sockfd);

	else if(!(strcmp(command, "/msg"))){ // private message
		char* target = strtok(NULL, " ");
		if(target == NULL)
			printf("Usage: /msg <username> <msg>\n");
		else
			msg_private(msg, buff+strlen(command)+strlen(target)+2, nick_name, target, sockfd);
	}

	else if(!(strcmp(command, "/create"))){
		char* new = strtok(NULL, " ");
		if(new == NULL)
			printf("Usage: /create <channel_name>\n");
		else
			new_channel(msg, buff, nick_name, new, channel, sockfd);
	}

	else if(!strcmp(command, "/channel_list"))
		channel_list(msg, buff, nick_name, sockfd);

	else if(!strcmp(command, "/join")){
		char* new = strtok(NULL, " ");
		if(new == NULL)
			printf("Usage: /join <channel>\n");
		else
			join_channel(msg, buff, nick_name, new, channel, sockfd);
	}

	else if(!(strcmp(command, "/quit"))){
		if(!(strcmp(channel, ""))){ // leave the chat
			pollfds[0].fd = -1;
			return -1;
		}
		else // leave the current channel
			leave_channel(msg, buff, nick_name, channel, sockfd);
	}

	else if(!(strcmp(command, "/send"))){
		char* target = strtok(NULL, " ");
		char* file_received =  strtok(NULL, " ");
		if(file_received == NULL)
			printf("Usage: /send <user> <file_name.txt>\n");
		else{
			strcpy(file_name, file_received);
			msg->type = FILE_REQUEST;
			ask_send(msg, buff, nick_name, target, file_name, sockfd);
		}
	}
	else if(!(strcmp(command, "/send_image"))){
		char* target = strtok(NULL, " ");
		char* file_received =  strtok(NULL, " ");
		if(file_received == NULL)
			printf("Usage: /send <user> <file_name.bmp>\n");
		else{
			strcpy(file_name, file_received);
			msg->type = IMAGE_REQUEST;
			ask_send(msg, buff, nick_name, target, file_name, sockfd);
		}

	}
	else if(!(strcmp(command, "/clean"))){ // clean the terminal
		printf("\033[H\033[2J");
		printf("%s\n", command);
	}
	else{
		if(!(strcmp(channel, ""))) // not in a channel
			echo(msg, buff, nick_name, sockfd);
		else
			msg_channel(msg, buff, nick_name, channel, sockfd);
	}

	return 0;
}

void client_loop(int sockfd, char* port) {
	struct message msg;
	char buff[MSG_LEN];
	char nick_name[NICK_LEN]; // current user nick name
	memset(nick_name, 0, NICK_LEN);

	char channel[CHAN_LEN]; // current channel
	memset(channel, 0, CHAN_LEN);

	int fd_transfert = -1;
	char file_name[INFOS_LEN]; // for file sharing

	int nfds = 3; // stdin & server & client for file sharing
	struct pollfd pollfds[nfds];
	// init stdin
	pollfds[0].fd = STDIN_FILENO;
	pollfds[0].events = 1;
	pollfds[0].revents = 0;

	// init server connexion
	pollfds[1].fd = sockfd;
	pollfds[1].events = 1;
	pollfds[1].revents = 0;

	pollfds[2].fd = -1;
	pollfds[2].events = 1;
	pollfds[2].revents = 0;

	while (pollfds[0].fd == STDIN_FILENO) {
		memset(&msg, 0, sizeof(struct message));
		memset(buff, 0, MSG_LEN);

		// activity detection
		int n_active = 0;
		if (-1 == (n_active = poll(pollfds, nfds, -1))) {
			perror("Poll");
		}
		// Iterate on the array of monitored struct pollfd
		for (int i = 0; i < nfds; i++) {
			// Receiving message
			if (pollfds[i].fd == sockfd && pollfds[i].revents & POLLIN){
				if(-1 == receive_serv(&msg, buff, pollfds, sockfd, fd_transfert, port, nick_name, file_name))
					pollfds[0].fd = -1; // server disconnected

				pollfds[i].revents = 0; // Set standard input activity back to default
			}

			// receive data from another client
			else if (pollfds[i].fd > sockfd && pollfds[i].revents & POLLIN){
				receive_data(&msg, buff, nick_name, file_name, pollfds[i].fd, sockfd);
				pollfds[i].fd = -1;
				pollfds[i].revents = 0; // Set activity back to default
			}

			// Client is using a command
			else if(pollfds[i].fd == STDIN_FILENO && pollfds[i].revents & POLLIN){
				if(-1 == use_command(&msg, buff, pollfds, sockfd, nick_name, channel, file_name))
					pollfds[0].fd = -1; // leave the chat

				pollfds[i].revents = 0; // Set standard input activity back to default
			}
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		printf("Usage: %s <hostname> <port_number>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	char *hostname = argv[1];
	char *port = argv[2];

	int sock_fd = -1;
	if (-1 == (sock_fd = socket_and_connect(hostname, port))) {
		printf("Could not create socket and connect properly\n");
		return 1;
	}

	client_loop(sock_fd, port);
	close(sock_fd);
	printf("You left the chat !\n");
	return 0;
}
