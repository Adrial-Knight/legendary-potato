#include <arpa/inet.h>
#include <err.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include "include/common.h"
#include "include/msg_struct.h"
#include "include/server_request.h"

#define BACKLOG 20

void msg_monitoring(struct message* msg, char* buff, struct pollfd* pollfd, struct log_client* all_clients){
	// "/quit" client commande
	if(!(strcmp(buff, "/quit"))){
		printf("/quit: client on socket %d has disconnected from server\n", pollfd->fd);
		exit_client(all_clients, pollfd);
		return;
	}

	switch (msg->type) {
		case NICKNAME_NEW:
			new_nick_name(msg, buff, pollfd, all_clients);
			break;
		case NICKNAME_LIST:
			who_is_online(msg, buff, pollfd, all_clients);
			break;
		case NICKNAME_INFOS:
			who_is_usr(msg, buff, pollfd, all_clients);
			break;
		case BROADCAST_SEND:
			msg_all(msg, buff, pollfd, all_clients);
			break;
		case UNICAST_SEND:
			msg_private(msg, buff, pollfd, all_clients);
			break;
		case ECHO_SEND:
			echo(msg, buff, pollfd);
			break;
		case MULTICAST_CREATE:
			new_channel(msg, buff, pollfd, all_clients);
			break;
		case MULTICAST_LIST:
			channel_list(msg, buff, pollfd, all_clients);
			break;
		case MULTICAST_JOIN:
			join_channel(msg, buff, pollfd, all_clients);
			break;
		case MULTICAST_QUIT:
			leave_channel(msg, buff, pollfd, all_clients);
			break;
		case MULTICAST_SEND:
			msg_channel(msg, buff, pollfd, all_clients);
			break;
		case FILE_REQUEST:
		case IMAGE_REQUEST:
			ask_send(msg, buff, pollfd, all_clients);
			break;
		case FILE_ACCEPT:
		case IMAGE_ACCEPT:
			accept_file_transfert(msg, buff, pollfd, all_clients);
			break;
		case FILE_REJECT:
		case FILE_ACK:
			ack_refuse_file(msg, buff, all_clients);
			break;
		default:
			break;
	}

	printf("\n\n------\nmsg:\n - pld_len: %d\n - nick_sender: %s\n - type: %d\n - infos: %s\n\nbuff: %s\n------\n\n\n", msg->pld_len, msg->nick_sender, msg->type, msg->infos, buff);

}

void server(int listen_fd, struct log_client* all_clients) {

	// Declare array of struct pollfd
	int nfds = BACKLOG;
	struct pollfd pollfds[nfds];
	struct message msg;
	char buff[MSG_LEN];

	// Init first slot with listening socket
	pollfds[0].fd = listen_fd;
	pollfds[0].events = POLLIN;
	pollfds[0].revents = 0;
	// Init remaining slot to default values
	for (int i = 1; i < nfds; i++) {
		pollfds[i].fd = -1;
		pollfds[i].events = 0;
		pollfds[i].revents = 0;
	}

	// server loop
	while (1) {
		memset(&msg, 0, sizeof(struct message));
		memset(buff, 0, MSG_LEN);

		// Block until new activity detected on existing socket
		int n_active = 0;
		if (-1 == (n_active = poll(pollfds, nfds, -1))) {
			perror("Poll error");
		}

		// Iterate on the array of monitored struct pollfd
		for (int i = 0; i < nfds; i++) {

			// If listening socket is active => accept new incoming connection
			if (pollfds[i].fd == listen_fd && pollfds[i].revents & POLLIN) {
				accept_new_client(&msg, buff, pollfds, nfds, all_clients, listen_fd);

				// Set .revents of listening socket back to default
				pollfds[i].revents = 0;
			}

			else if (pollfds[i].fd != listen_fd && pollfds[i].revents & POLLHUP) { // If a socket previously created to communicate with a client detects a disconnection from the client side
				printf("client on socket %d has disconnected from server\n", pollfds[i].fd);
				exit_client(all_clients, &pollfds[i]);
				break;
			}

			else if (pollfds[i].fd != listen_fd && pollfds[i].revents & POLLIN) { // If a socket different from the listening socket is active
				// read data from socket

				int ret_mes = read_from_socket(pollfds[i].fd, &msg, sizeof(struct message));

				if(ret_mes < 0){
					printf("client on socket %d has disconnected from server\n", pollfds[i].fd);
					exit_client(all_clients, &pollfds[i]);
					break;
				}

				if(msg.pld_len > 0)
					read_from_socket(pollfds[i].fd, buff, MSG_LEN);
				else
					memset(buff, 0, MSG_LEN);

				if(strcmp(msg.nick_sender, "SERVER"))
					msg_monitoring(&msg, buff, &pollfds[i], all_clients);
				else
					printf("SERVER receive a message from SERVER...\n");

				// Set activity detected back to default
				pollfds[i].revents = 0;
			}
		}
	}
}

int main(int argc, char *argv[]) {

	// Test argc
	if (argc != 2) {
		printf("Usage: %s <port_number>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// Create listening socket
	char *port = argv[1];
	int listen_fd = -1;
	if (-1 == (listen_fd = socket_listen_and_bind(port, BACKLOG))) {
		printf("Could not create, bind and listen properly\n");
		return 1;
	}
	// Handle new connections and existing ones
	struct log_client* all_clients = malloc(sizeof(struct log_client));
	all_clients->next = NULL;
	printf("Server running\n");
	server(listen_fd, all_clients);

	close(listen_fd);
	free(all_clients);

	exit(EXIT_SUCCESS);
}
