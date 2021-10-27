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


/*
      #@*     ,@@(      (@*          .#&@@@@&/    .&@(      #@,           (@%
      #@*    .@(.@#     (@*        .@&       /@%  .&@%@*    #@,        (@# &%
      #@*   .@#  ,@%    (@*        @&.        /@/ .&@ ,@&.  #@,            &%
      #@*  ,&@@@@@@@#   (@*        @&,        /@/ .&@   /@% #@,            &%
/@*   %@.  @%      ,@&  (@*         &@*     .#@#  .&@     %@&@,            &%
 ,#@@&/  .@&.       *@% (@@@@@@@@/    ,#&@@%/.    .&@      ,@@,            **

*/

void addclient(struct log_client* all_clients, int fd, int port, char* addr){
	struct log_client* tmp = all_clients;
	int n = 1;

	while(tmp->next != NULL){ // search the end of the chained list
		tmp = tmp->next;
		n++;
	}

	// register the client
	struct log_client* new_client = malloc(sizeof(struct log_client));
	tmp->next = new_client;

	new_client->fd = fd;
	new_client->port = port;
	strcpy(new_client->addr, addr);

	// date
	char hour[16] = {0};
	char date[16] = {0};
	time_t rawtime = time(NULL);
	struct tm *ptm = localtime(&rawtime);
	strftime(hour, 16, "@%T", ptm);
	strftime(date, 16, "%D", ptm);
	strcat(date, hour);
	strcpy(new_client->time, date);

	memset(new_client->channel, 0, CHAN_LEN);

	memset(new_client->nick_name, 0, NICK_LEN);
	new_client->next = NULL;

	if (n == 1) // show current connections
		printf("%d client online !\n", n);
	else
		printf("%d clients online !\n", n);
}

void exit_client(struct log_client* all_clients, struct pollfd* pollfd){
	struct log_client* info2delete = all_clients;
	struct log_client* previous = all_clients;

	while(info2delete->fd != pollfd->fd){
		previous = info2delete;
		info2delete = info2delete->next;
	}

	if(info2delete->next != NULL)
		previous->next = info2delete->next;
	else
		previous->next = NULL;

	close(pollfd->fd);
	pollfd->fd = -1;
	pollfd->events = 0;
	pollfd->revents = 0;
	free(info2delete);
}

int first_nick_name(struct message* msg, char* buff, struct log_client* all_clients, int client_fd){
	struct log_client* client_tmp = all_clients;

	msg->pld_len = strlen(buff);
	strcpy(msg->nick_sender, "SERVER");
	msg->type = NICKNAME_NEW;

	write_in_socket(client_fd, msg, sizeof(struct message));
	write_in_socket(client_fd, buff, MSG_LEN);

	if(-1 == read_from_socket(client_fd, msg, sizeof(struct message)))
		return -1; // client disconnected

	while(client_tmp != NULL){
		if(!(strcmp(msg->infos, client_tmp->nick_name))){
			return -2;	// nickname in msg->infos is already taken
		}
		else
			client_tmp = client_tmp->next;
	}

	return 0; // the new & valid nickname is in msg->infos
}

void accept_new_client(struct message* msg, char* buff, struct pollfd* pollfds, int nfds, struct log_client* all_clients, int listen_fd){
	// store new file descriptor in available slot in the array of struct pollfd set .events to POLLIN
	char address[16] = {0};
	int port = -1;
	int client_fd = accept_new_connexion(listen_fd, address, &port);

	for (int j = 0; j < nfds; j++) {
		if (pollfds[j].fd == -1) {
			pollfds[j].fd = client_fd;
			pollfds[j].events = POLLIN;
			break;
		}
	}

	addclient(all_clients, client_fd, port, address);

	strcpy(buff, "please login with /nick <your pseudo>");
	msg->pld_len = strlen(buff);
	strcpy(msg->nick_sender, "SERVER");
	msg->type = NICKNAME_NEW;

	write_in_socket(client_fd, msg, sizeof(struct message));
	write_in_socket(client_fd, buff, MSG_LEN);
}

int nick_available(struct message* msg, struct pollfd* pollfd, struct log_client* all_clients){
	struct log_client* tmp = all_clients;
	struct log_client* current_client = all_clients;

	while (tmp != NULL) {
		if((!strcmp(msg->infos, tmp->nick_name)))
			return -2;

		if(tmp->fd == pollfd->fd)
			current_client = tmp;

		tmp = tmp->next;
	}

	strcpy(current_client->nick_name, msg->infos);
	return 0;
}

void new_nick_name(struct message* msg, char* buff, struct pollfd* pollfd, struct log_client* all_clients){
	if(0 == nick_available(msg, pollfd, all_clients)){
		if(0 == strlen(msg->nick_sender))
			strcpy(buff, "Welcome on the chat");
		else
			strcpy(buff, "Your new pseudo is ");
	}

	else{
		strcpy(buff, "This pseudo is already taken:");
		if(strlen(msg->nick_sender) == 0)
			msg->type = NICKNAME_NEW;
	}

	msg->pld_len = strlen(buff);
	strcpy(msg->nick_sender, "SERVER");
	write_in_socket(pollfd->fd, msg, sizeof(struct message));
	write_in_socket(pollfd->fd, buff, MSG_LEN);
}



/*
      #@*     ,@@(      (@*          .#&@@@@&/    .&@(      #@,        ,&@@@@%.
      #@*    .@(.@#     (@*        .@&       /@%  .&@%@*    #@,       (@,    /@,
      #@*   .@#  ,@%    (@*        @&.        /@/ .&@ ,@&.  #@,             *@#
      #@*  ,&@@@@@@@#   (@*        @&,        /@/ .&@   /@% #@,          .&@(
/@*   %@.  @%      ,@&  (@*         &@*     .#@#  .&@     %@&@,        #@#
 ,#@@&/  .@&.       *@% (@@@@@@@@/    ,#&@@%/.    .&@      ,@@,      .&@@@@@@@@*

*/

void who_is_online(struct message* msg, char* buff, struct pollfd* pollfd, struct log_client* all_clients){
	memset(buff, 0, MSG_LEN);
	memset(msg, 0, sizeof(struct message));
	log_client* tmp = all_clients;

	while(tmp != NULL){
		strcat(buff, tmp->nick_name);
		strcat(buff, "@");
		tmp = tmp->next;
	}

	msg->pld_len = strlen(buff);
	strcpy(msg->nick_sender, "SERVER");
	msg->type = NICKNAME_LIST;
	write_in_socket(pollfd->fd, msg, sizeof(struct message));
	write_in_socket(pollfd->fd, buff, MSG_LEN);
}

void who_is_usr(struct message* msg, char* buff, struct pollfd* pollfd, log_client* all_clients){
	memset(buff, 0, MSG_LEN);
	log_client* tmp = all_clients;

	while(tmp != NULL){
		if(!(strcmp(tmp->nick_name, msg->infos)))
			break;
		tmp = tmp->next;
	}

	if(tmp != NULL)
		sprintf(buff, "%s %s %d", tmp->time, tmp->addr, tmp->port);

	memset(msg, 0, sizeof(struct message));
	strcpy(msg->nick_sender, "SERVER");
	msg->pld_len = strlen(buff);
	msg->type = NICKNAME_INFOS;
	write_in_socket(pollfd->fd, msg, sizeof(struct message));
	write_in_socket(pollfd->fd, buff, MSG_LEN);
}

void msg_all(struct message* msg, char* buff, struct pollfd* pollfd, log_client* all_clients){
	struct log_client* tmp = all_clients;

	while (tmp != NULL) {
		if(tmp->fd != pollfd->fd && tmp->fd != 0){
			write_in_socket(tmp->fd, msg, sizeof(struct message));
			write_in_socket(tmp->fd, buff, MSG_LEN);
		}
		tmp = tmp->next;
	}
}

void msg_private(struct message* msg, char* buff, struct pollfd* pollfd, struct log_client* all_clients){
	struct log_client* tmp = all_clients;

	while (tmp != NULL){
		if(!(strcmp(msg->infos, tmp->nick_name)))
			break; // destination found
		tmp = tmp->next;
	}

	if(tmp != NULL){
		write_in_socket(tmp->fd, msg, sizeof(struct message));
		write_in_socket(tmp->fd, buff, MSG_LEN);
	}
	else{
		strcpy(buff, "user ");
		strcat(buff, msg->infos);
		strcat(buff, " does not exist");
		msg->pld_len = strlen(buff);
		strcpy(msg->nick_sender, "SERVER");
		memset(msg->infos, 0, INFOS_LEN);
		write_in_socket(pollfd->fd, msg, sizeof(struct message));
		write_in_socket(pollfd->fd, buff, MSG_LEN);
	}
}

void echo(struct message* msg, char* buff, struct pollfd* pollfd){
	write_in_socket(pollfd->fd, msg, sizeof(struct message));
	write_in_socket(pollfd->fd, buff, MSG_LEN);
}


/*
      #@*     ,@@(      (@*          .#&@@@@&/    .&@(      #@,          ,%@%/
      #@*    .@(.@#     (@*        .@&       /@%  .&@%@*    #@,        /@(   ,@#
      #@*   .@#  ,@%    (@*        @&.        /@/ .&@ ,@&.  #@,             *%@*
      #@*  ,&@@@@@@@#   (@*        @&,        /@/ .&@   /@% #@,              /@%.
/@*   %@.  @%      ,@&  (@*         &@*     .#@#  .&@     %@&@,        /%.    *@*
 ,#@@&/  .@&.       *@% (@@@@@@@@/    ,#&@@%/.    .&@      ,@@,        /@@@@@@,

*/

void new_channel(struct message* msg, char* buff, struct pollfd* pollfd, struct log_client* all_clients){
	struct log_client* tmp = all_clients;
	struct log_client* current_client = all_clients;

	while(tmp != NULL){
		if(!(strcmp(tmp->channel, msg->infos))) // already exists
			break;

		else if(pollfd->fd == tmp->fd)
			current_client = tmp;

		tmp = tmp->next;
	}

	if(tmp == NULL){ // creation of a new channel
		strcpy(current_client->channel, msg->infos);
		strcpy(buff, "Welcome on your new channel");
	}

	else
		strcpy(buff, "This channel already exists");

	msg->pld_len = strlen(buff);
	strcpy(msg->nick_sender, "SERVER");

	write_in_socket(pollfd->fd, msg, sizeof(struct message));
	write_in_socket(pollfd->fd, buff, MSG_LEN);
}

void channel_list(struct message* msg, char* buff, struct pollfd* pollfd, struct log_client* all_clients){
	memset(buff, 0, MSG_LEN);
	log_client* tmp = all_clients;

	while(tmp != NULL){
		if(tmp->channel != NULL){ // user considered is in a channel
			if(strstr(buff, tmp->channel) == NULL){ // channel not already find
				strcat(buff, tmp->channel); // register this channel
				strcat(buff, "@"); // separate channel names with a non alphanumeric char
			}
		}
		tmp = tmp->next;
	}

	msg->pld_len = strlen(buff);
	strcpy(msg->nick_sender, "SERVER");

	write_in_socket(pollfd->fd, msg, sizeof(struct message));
	write_in_socket(pollfd->fd, buff, MSG_LEN);
}

void join_channel(struct message* msg, char* buff, struct pollfd* pollfd, struct log_client* all_clients){
	memset(buff, 0, MSG_LEN);
	int find = 0;
	log_client* tmp = all_clients;
	log_client* current_client = all_clients;

	strcpy(buff, msg->nick_sender);
	strcat(buff, " joines the channel");

	msg->pld_len = strlen(buff);
	msg->type = MULTICAST_SEND;
	strcpy(msg->nick_sender, "SERVER");


	while(tmp != NULL){
		if(tmp->channel != NULL){ // user considered is in a channel
			if(!(strcmp(msg->infos, tmp->channel))){ // channel correspond to the client request
				find = 1;
				write_in_socket(tmp->fd, msg, sizeof(struct message));
				write_in_socket(tmp->fd, buff, MSG_LEN);
			}
			if(tmp->fd == pollfd->fd)
				current_client = tmp;
		}
		tmp = tmp->next;
	}

	if(!(strcmp(current_client->channel, msg->infos)))
		strcpy(buff, "You already are in");

	else if(!find)
		strcpy(buff, "This channel does not exist");

	else{
		strcpy(current_client->channel, msg->infos);
		strcpy(buff, "Welcome in the channel");
	}

	msg->pld_len = strlen(buff);

	write_in_socket(pollfd->fd, msg, sizeof(struct message));
	write_in_socket(pollfd->fd, buff, MSG_LEN);
}

void leave_channel(struct message* msg, char* buff, struct pollfd* pollfd, struct log_client* all_clients){
	struct log_client* tmp = all_clients;
	struct log_client* current_client = all_clients;
	int alone = 1;

	strcpy(buff, msg->nick_sender);
	strcat(buff, " leaves the channel");

	msg->pld_len = strlen(buff);
	msg->type = MULTICAST_SEND;
	strcpy(msg->nick_sender, "SERVER");

	while(tmp != NULL){
		if(tmp->fd == pollfd->fd)
			current_client = tmp;

		else if(!strcmp(tmp->channel, msg->infos)){
			alone = 0;
			write_in_socket(tmp->fd, msg, sizeof(struct message));
			write_in_socket(tmp->fd, buff, MSG_LEN);
		}
		tmp = tmp->next;
	}

	memset(current_client->channel, 0, CHAN_LEN);

	strcpy(buff, "You left the channel");
	if(alone)
		strcat(buff, " and it has been destroyed:");
	msg->pld_len = strlen(buff);
	strcpy(msg->nick_sender, "SERVER");

	write_in_socket(pollfd->fd, msg, sizeof(struct message));
	write_in_socket(pollfd->fd, buff, MSG_LEN);
}

void msg_channel(struct message* msg, char* buff, struct pollfd* pollfd, struct log_client* all_clients){
	struct log_client* tmp = all_clients;
	msg->type = MULTICAST_SEND;

	while(tmp != NULL){
		if(!(strcmp(tmp->channel, msg->infos))){
			write_in_socket(tmp->fd, msg, sizeof(struct message));
			write_in_socket(tmp->fd, buff, MSG_LEN);
		}
		tmp = tmp->next;
	}
}


/*

      #@*     ,@@(      (@*          .#&@@@@&/    .&@(      #@,              .
      #@*    .@(.@#     (@*        .@&       /@%  .&@%@*    #@,            (@@(
      #@*   .@#  ,@%    (@*        @&.        /@/ .&@ ,@&.  #@,          *@,,@(
      #@*  ,&@@@@@@@#   (@*        @&,        /@/ .&@   /@% #@,        ,@(  ,@(
/@*   %@.  @%      ,@&  (@*         &@*     .#@#  .&@     %@&@,       *@@@@@@@@@/
 ,#@@&/  .@&.       *@% (@@@@@@@@/    ,#&@@%/.    .&@      ,@@,             ,@(

*/

void ask_send(struct message* msg, char* buff, struct pollfd* pollfd, struct log_client* all_clients){
	log_client* tmp = all_clients;
	while (tmp != NULL) {
		if(!(strcmp(tmp->nick_name, msg->infos)))
			break;
		tmp = tmp->next;
	}

	if(tmp != NULL){
		strcpy(msg->infos, buff);
		strcpy(buff, "wants you to accept the transfer of the file named");
		msg->pld_len = strlen(buff);
		write_in_socket(tmp->fd, msg, sizeof(struct message));
		write_in_socket(tmp->fd, buff, MSG_LEN);
	}
	else{
		strcpy(buff, "is not online");
		msg->pld_len = strlen(buff);
		write_in_socket(pollfd->fd, msg, sizeof(struct message));
		write_in_socket(pollfd->fd, buff, MSG_LEN);
	}
}

void accept_file_transfert(struct message* msg, char* buff, struct pollfd* pollfd, struct log_client* all_clients){
	/*
	int failed_once = 0;
	while(-1 == read_from_socket(pollfd->fd, msg, sizeof(struct message)))
		failed_once = 1;

	if(failed_once){ // get correct informations
		read_from_socket(pollfd->fd, msg, sizeof(struct message));
		read_from_socket(pollfd->fd, buff, MSG_LEN);
	}
	*/

	struct log_client* src = all_clients;
	while(src != NULL){
		if(!(strcmp(src->nick_name, msg->infos)))
			break;

		src = src->next;
	}

	strcpy(msg->infos, buff); // addr:port
	msg->pld_len = -1;
	write_in_socket(src->fd, msg, sizeof(struct message));
}

void ack_refuse_file(struct message* msg, char* buff, struct log_client* all_clients){
	struct log_client* target = all_clients;

	while((strcmp(target->nick_name, msg->infos)))
		target = target->next;

	write_in_socket(target->fd, msg, sizeof(struct message));
}
