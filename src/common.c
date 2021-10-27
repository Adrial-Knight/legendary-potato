#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <err.h>
#include "include/common.h"


int read_from_socket(int fd, void *buf, int size) {
	int ret = 0;
	int offset = 0;
	while (offset != size) {
		ret = read(fd, buf + offset, size - offset);
		if (-1 == ret) {
			perror("Reading from client socket");
			return -1;
		}
		if (0 == ret) {
			close(fd);
			return -1;
		}
		offset += ret;
	}
	return offset;
}

int write_in_socket(int fd, void *buf, int size) {
	int ret = 0, offset = 0;
	while (offset != size) {
		if (-1 == (ret = write(fd, buf + offset, size - offset))) {
			perror("Writing from client socket");
			return -1;
		}
		offset += ret;
	}
	return offset;
}

int socket_listen_and_bind(char *port, int backlog) {
	int listen_fd = -1;
	if (-1 == (listen_fd = socket(AF_INET, SOCK_STREAM, 0))) {
		perror("Socket");
		exit(EXIT_FAILURE);
	}

	int yes = 1;
	if (-1 == setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	struct addrinfo indices;
	memset(&indices, 0, sizeof(struct addrinfo));
	indices.ai_family = AF_INET;
	indices.ai_socktype = SOCK_STREAM;
	indices.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
	struct addrinfo *address, *tmp;
	tmp = address;

	int err = 0;
	if (0 != (err = getaddrinfo(NULL, port, &indices, &address))) {
		errx(1, "%s", gai_strerror(err));
	}

	while (address != NULL) {
		if (address->ai_family == AF_INET) {
			if (-1 == bind(listen_fd, address->ai_addr, address->ai_addrlen)) {
				perror("Binding");
			}
			if (-1 == listen(listen_fd, backlog)) {
				perror("Listen");
			}
			return listen_fd;
		}
		address = address->ai_next;
	}
	freeaddrinfo(tmp);
	return listen_fd;
}

int accept_new_connexion(int listen_fd, char* address, int* port){
	struct sockaddr client_addr;
	socklen_t size = sizeof(client_addr);
	int client_fd;
	if (-1 == (client_fd = accept(listen_fd, &client_addr, &size))) {
		perror("Accept");
	}

	struct sockaddr_in *sockptr = (struct sockaddr_in *)(&client_addr);
	struct in_addr client_address = sockptr->sin_addr;

	// display and register IP_address:port_number to fd
	strcpy(address, inet_ntoa(client_address));
	*port = ntohs(sockptr->sin_port);

	return client_fd;
}
