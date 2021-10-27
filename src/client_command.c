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
#include "include/image.h"


/*
      #@*     ,@@(      (@*          .#&@@@@&/    .&@(      #@,           (@%
      #@*    .@(.@#     (@*        .@&       /@%  .&@%@*    #@,        (@# &%
      #@*   .@#  ,@%    (@*        @&.        /@/ .&@ ,@&.  #@,            &%
      #@*  ,&@@@@@@@#   (@*        @&,        /@/ .&@   /@% #@,            &%
/@*   %@.  @%      ,@&  (@*         &@*     .#@#  .&@     %@&@,            &%
 ,#@@&/  .@&.       *@% (@@@@@@@@/    ,#&@@%/.    .&@      ,@@,            **

*/

void display_list(char* intro, char* content){
	char* padding = "\n                         - ";
	int len_padding = strlen(padding);
	for (int i = 0; i < strlen(intro); i++)
		write(STDOUT_FILENO, intro+i, sizeof(char));
	for (int i = 0; i < len_padding; i++)
		write(STDOUT_FILENO, padding+i, sizeof(char));
	for (int i = 0; i < strlen(content)-1; i++) {
		if(*(content+i) == '@'){
			for (int i = 0; i < len_padding; i++)
				write(STDOUT_FILENO, padding+i, sizeof(char));
		}
		else
			write(STDOUT_FILENO, content+i, sizeof(char));
	}
	write(STDOUT_FILENO, "\n", sizeof(char));
}

int test_nick(char* nick_name){
	char* tmp = nick_name;
	tmp[strlen(nick_name)] = '\0';

	// SERVER name is not available
	if(!(strcmp(nick_name, "SERVER"))){
		printf("Can not use \"SERVER\" as a name\n");
		return -1;
	}

	// can not exceed the limit size
	if(strlen(tmp) > NICK_LEN){
		printf("Can not exceed %d\n", NICK_LEN);
		return -1;
	}

	// can not use non alphanumeric char
	int n = strlen(nick_name);
	int i=-1;
	for (i = 0; i < n; i++) {
		if(!isalnum(tmp[i])) // non alphanumeric char detected
			break;
	}
	if(i != n){
		printf("%s contains non alphanumeric caractere\n", nick_name);
		return -1;
	}

	return 0;
}

void enter_new_nick(char* nick_name){
	int unvalid_nick = 1;
	int n = -1;

	while (unvalid_nick){
		memset(nick_name, 0, NICK_LEN);
		n = 0;
		while ((nick_name[n++] = getchar()) != '\n') {}
		nick_name[n-1] = '\0';

		char* command = strtok(nick_name, " ");
		if(command == NULL){
			printf("Use \"/nick\" command\n");
			continue;
		}
		else if((strcmp(command, "/nick"))){
			printf("Use \"/nick\" command\n");
			continue;
		}

		char* new = nick_name+strlen("/nick ");
		unvalid_nick = test_nick(new);
		strcpy(nick_name, new);
	}
}

void server_ask_nick(struct message* msg, char* buff, char* nick_name, int sockfd){
	printf("[%s]: %s %s\n", msg->nick_sender, buff, msg->infos); // display server request

	enter_new_nick(nick_name); // enter and check a brand new nick name

	msg->pld_len = -1;
	memset(msg->nick_sender, 0, NICK_LEN);
	strcpy(msg->infos, nick_name);
	msg->type = NICKNAME_NEW;

	write_in_socket(sockfd, msg, sizeof(struct message)); // answear the server
}

void echo(struct message* msg, char* buff, char* nick_name, int sockfd){
	msg->pld_len = strlen(buff);
	strcpy(msg->nick_sender, nick_name);
	msg->type = ECHO_SEND;
	memset(msg->infos, 0, INFOS_LEN);

	write_in_socket(sockfd, msg, sizeof(struct message)); // send msg to the server
	write_in_socket(sockfd, buff, MSG_LEN);
}


/*
      #@*     ,@@(      (@*          .#&@@@@&/    .&@(      #@,        ,&@@@@%.
      #@*    .@(.@#     (@*        .@&       /@%  .&@%@*    #@,       (@,    /@,
      #@*   .@#  ,@%    (@*        @&.        /@/ .&@ ,@&.  #@,             *@#
      #@*  ,&@@@@@@@#   (@*        @&,        /@/ .&@   /@% #@,          .&@(
/@*   %@.  @%      ,@&  (@*         &@*     .#@#  .&@     %@&@,        #@#
 ,#@@&/  .@&.       *@% (@@@@@@@@/    ,#&@@%/.    .&@      ,@@,      .&@@@@@@@@*

*/

void change_nick(struct message* msg, char* buff, char*nick_name, char* new, int sockfd){
	strcpy(msg->nick_sender, nick_name);
	strcpy(msg->infos, new);
	msg->type = NICKNAME_NEW;
	msg->pld_len = -1;

	write_in_socket(sockfd, msg, sizeof(struct message)); // send the new nick name to the server
	read_from_socket(sockfd, msg, sizeof(struct message)); // server answear
	read_from_socket(sockfd, buff, MSG_LEN);

	printf("[%s]: %s%s\n", msg->nick_sender, buff, msg->infos);
	if(!(strcmp(buff, "Your new pseudo is ")))
		strcpy(nick_name, msg->infos);
}

void who_is_online(struct message* msg, char* buff, char* nick_name, int sockfd){
	msg->pld_len = -1;
	strcpy(msg->nick_sender, nick_name);
	msg->type = NICKNAME_LIST;
	memset(msg->infos, 0, INFOS_LEN);

	write_in_socket(sockfd, msg, sizeof(struct message)); // ask the server

	// get the answear
	memset(msg, 0, sizeof(struct message));
	memset(buff, 0, MSG_LEN);
	read_from_socket(sockfd, msg, sizeof(struct message));
	read_from_socket(sockfd, buff, MSG_LEN);
	// formatting the answear
	buff++; // a "@" is added at the very beggining by the server, but I don't konw why
	char* intro = "[SERVER]: Online users are";
	display_list(intro, buff);
}

void who_is_usr(struct message* msg, char* buff, char* nick_name, char* target, int sockfd){
	msg->pld_len = -1;
	strcpy(msg->nick_sender, nick_name);
	msg->type = NICKNAME_INFOS;
	strcpy(msg->infos, target);

	write_in_socket(sockfd, msg, sizeof(struct message)); // ask the server

	read_from_socket(sockfd, msg, sizeof(struct message)); // answear
	read_from_socket(sockfd, buff, MSG_LEN);

	// formatting answear
	if(strlen(buff) == 0)
		printf("[%s]: %s is not online\n", msg->nick_sender, target);
	else{
		char display[100+NICK_LEN] = {0};
		strcpy(display, "[");
		strcat(display, msg->nick_sender);
		strcat(display, "]: ");
		strcat(display, target);
		strcat(display, " connected since ");
		strcat(display, strtok(buff, " "));
		strcat(display, " with IP address ");
		strcat(display, strtok(NULL, " "));
		strcat(display, " and port number ");
		strcat(display, strtok(NULL, " "));
		strcat(display, "\n");
		for (int i = 0; i < sizeof(display); i++) {
			write(STDOUT_FILENO, display+i, sizeof(char));
		}
	}
}

void msg_all(struct message* msg, char* buff, char* nick_name, int sockfd){
	msg->pld_len = strlen(buff);
	strcpy(msg->nick_sender, nick_name);
	msg->type = BROADCAST_SEND;
	memset(msg->infos, 0, INFOS_LEN);

	write_in_socket(sockfd, msg, sizeof(struct message)); // send msg to the server
	write_in_socket(sockfd, buff, MSG_LEN);
}

void msg_private(struct message* msg, char* buff, char* nick_name, char* target, int sockfd){
	msg->pld_len = strlen(buff);
	strcpy(msg->nick_sender, nick_name);
	msg->type = UNICAST_SEND;
	strcpy(msg->infos, target);

	write_in_socket(sockfd, msg, sizeof(struct message)); // send msg to the server
	write_in_socket(sockfd, buff, MSG_LEN);
}


/*
      #@*     ,@@(      (@*          .#&@@@@&/    .&@(      #@,          ,%@%/
      #@*    .@(.@#     (@*        .@&       /@%  .&@%@*    #@,        /@(   ,@#
      #@*   .@#  ,@%    (@*        @&.        /@/ .&@ ,@&.  #@,             *%@*
      #@*  ,&@@@@@@@#   (@*        @&,        /@/ .&@   /@% #@,              /@%.
/@*   %@.  @%      ,@&  (@*         &@*     .#@#  .&@     %@&@,        /%.    *@*
 ,#@@&/  .@&.       *@% (@@@@@@@@/    ,#&@@%/.    .&@      ,@@,        /@@@@@@,

*/

void new_channel(struct message* msg, char* buff, char* nick_name, char* new, char* channel_name, int sockfd){
	memset(msg, 0, sizeof(struct message));
	memset(buff, 0, MSG_LEN);
	msg->pld_len = -1;
	strcpy(msg->nick_sender, nick_name);
	msg->type = MULTICAST_CREATE;
	strcpy(msg->infos, new);

	char* tmp = new;
	tmp[strlen(new)] = '\0';

	int n = strlen(tmp);
	int i=-1;
	for (i = 0; i < n; i++) {
		if(!isalnum(tmp[i])){ // non alphanumeric char detected
			printf("%s contains non alphanumeric caractere\n", new);
			break;
		}
	}
	if(i != n)
		return;

	write_in_socket(sockfd, msg, sizeof(struct message));
	read_from_socket(sockfd, msg, sizeof(struct message));
	read_from_socket(sockfd, buff, MSG_LEN);

	printf("[%s]: %s: %s\n", msg->nick_sender, buff, new);

	if(!(strcmp(buff, "Welcome on your new channel")))
		strcpy(channel_name, new);
}

void channel_list(struct message* msg, char* buff, char* nick_name, int sockfd){
	msg->pld_len = -1;
	strcpy(msg->nick_sender, nick_name);
	msg->type = MULTICAST_LIST;
	memset(msg->infos, 0, INFOS_LEN);

	write_in_socket(sockfd, msg, sizeof(struct message)); // ask the server

	// get the answear
	memset(msg, 0, sizeof(struct message));
	memset(buff, 0, MSG_LEN);
	read_from_socket(sockfd, msg, sizeof(struct message));
	read_from_socket(sockfd, buff, MSG_LEN);

	// formatting the answear
	if(msg->pld_len > 0){
		char* intro = "[SERVER]: All channels are";
		display_list(intro, buff);
	}
	else
		printf("[SERVER]: No existing channel\n");
}

void join_channel(struct message* msg, char* buff, char* nick_name, char* new, char* channel, int sockfd){
	msg->pld_len = -1;
	strcpy(msg->nick_sender, nick_name);
	msg->type = MULTICAST_JOIN;
	strcpy(msg->infos, new);

	write_in_socket(sockfd, msg, sizeof(struct message));

	memset(msg, 0, sizeof(struct message));
	memset(buff, 0, MSG_LEN);
	read_from_socket(sockfd, msg, sizeof(struct message));
	read_from_socket(sockfd, buff, MSG_LEN);

	printf("[%s]: %s %s\n", msg->nick_sender, buff, msg->infos);
	if(!(strcmp(buff, "Welcome in the channel")))
		strcpy(channel, msg->infos);

	else
		printf("You can use /create %s\n", msg->infos);
}

void leave_channel(struct message* msg, char* buff, char* nick_name, char* channel, int sockfd){
	msg->pld_len = -1;
	strcpy(msg->nick_sender, nick_name);
	msg->type = MULTICAST_QUIT;
	strcpy(msg->infos, channel);

	write_in_socket(sockfd, msg, sizeof(struct message));

	memset(msg, 0, sizeof(struct message));
	memset(buff, 0, MSG_LEN);
	read_from_socket(sockfd, msg, sizeof(struct message));
	read_from_socket(sockfd, buff, MSG_LEN);

	printf("[%s]: %s %s\n", msg->nick_sender, buff, channel);
	memset(channel, 0, CHAN_LEN);
}

void msg_channel(struct message* msg, char* buff, char* nick_name, char* channel, int sockfd){
	msg->pld_len = strlen(buff);
	strcpy(msg->nick_sender, nick_name);
	msg->type = MULTICAST_SEND;
	strcpy(msg->infos, channel);

	write_in_socket(sockfd, msg, sizeof(struct message));
	write_in_socket(sockfd, buff, MSG_LEN);
}


/*

      #@*     ,@@(      (@*          .#&@@@@&/    .&@(      #@,              .
      #@*    .@(.@#     (@*        .@&       /@%  .&@%@*    #@,            (@@(
      #@*   .@#  ,@%    (@*        @&.        /@/ .&@ ,@&.  #@,          *@,,@(
      #@*  ,&@@@@@@@#   (@*        @&,        /@/ .&@   /@% #@,        ,@(  ,@(
/@*   %@.  @%      ,@&  (@*         &@*     .#@#  .&@     %@&@,       *@@@@@@@@@/
 ,#@@&/  .@&.       *@% (@@@@@@@@/    ,#&@@%/.    .&@      ,@@,             ,@(

*/

void ask_send(struct message* msg, char* buff, char* nick_name, char* target, char* file_name, int sockfd){

	FILE* exist = fopen(file_name, "r");
	if(NULL == exist){
		printf("%s does not exist\n", file_name);
		return ;
	}
	fclose(exist);

	strcpy(buff, file_name);

	msg->pld_len = strlen(buff);
	strcpy(msg->nick_sender, nick_name);
	strcpy(msg->infos, target);

	write_in_socket(sockfd, msg, sizeof(struct message)); // send msg to the server
	write_in_socket(sockfd, buff, MSG_LEN);
}

int accept_file_transfert(struct message* msg, char* buff, char* nick_name, int sockfd, char* port){
	char src[NICK_LEN];
	strcpy(src, msg->nick_sender);
	strcpy(msg->nick_sender, nick_name);
	int listen_fd = -1;

	char answear = getchar();

	if(answear == 'y' || answear == 'Y'){
		msg->pld_len = -1;
		while(-1 == listen_fd){
			int port_num = atoi(port)+1;
			sprintf(port, "%d", port_num);
			sprintf(buff, "127.0.0.1:%s", port);

			msg->pld_len = strlen(buff);
			strcpy(msg->infos, src);

			write_in_socket(sockfd, msg, sizeof(struct message));
			write_in_socket(sockfd, buff, MSG_LEN);
			listen_fd = socket_listen_and_bind(port, 1);

			if(-1 == listen_fd){ // chosen port is already taken
				strcpy(msg->infos, "@@__WAIT__@@");
				write_in_socket(sockfd, msg, sizeof(struct message));
			}
		}

	}
	else{
		memset(msg, 0, sizeof(struct message));
		msg->pld_len = -1;
		msg->type = FILE_REJECT;
		strcpy(msg->infos, src);
		strcpy(msg->nick_sender, nick_name);
		write_in_socket(sockfd, msg, sizeof(struct message));
	}
	return listen_fd;
}

void file_sending(struct message* msg, char* buff, char* nick_name, char* file_name, int fd_target){
	FILE* file_to_send = fopen(file_name, "r");

	fseek(file_to_send, 0, SEEK_END);
	int bytes_sent = ftell(file_to_send);
	fseek(file_to_send, 0, SEEK_SET);

	int nb_packet = floor(bytes_sent/MSG_LEN);
	int rest = bytes_sent - nb_packet*MSG_LEN;
	nb_packet += rest > 0;

	// send data to the target
	memset(msg, 0, sizeof(struct message));
	msg->pld_len = MSG_LEN;
	strcpy(msg->nick_sender, nick_name);
	msg->type = FILE_SEND;
	sprintf(msg->infos, "%d", nb_packet);
	write_in_socket(fd_target, msg, sizeof(struct message));

	for (int i = 0; i < nb_packet; i++){
		memset(buff, 0, MSG_LEN);
		fread(buff, sizeof(char), MSG_LEN, file_to_send);
		write_in_socket(fd_target, buff, MSG_LEN);
	}

	fclose(file_to_send);
}

void file_receiving(struct message* msg, char* buff, char* nick_name, char* file_name, int fd_transfert, int sockfd){
	int nb_packet = atoi(msg->infos);
	int pckt_rcv = 0;

	// create a new directory for the client if it does not exist yet
	char path[7+strlen(nick_name)+strlen(file_name)];
	memset(path, 0, strlen(path));

	// general directory
	strcpy(path, "inbox");
	struct stat sate_p = {0};

	if (-1 == stat(path, &sate_p))
		mkdir(path, 0700);

	// client directory
	strcat(path, "/");
	strcat(path, nick_name);
	memset(&sate_p, 0, sizeof(struct stat));

	if (-1 == stat(path, &sate_p))
		mkdir(path, 0700);

	// create a new file
	strcat(path, "/");
	strcat(path, file_name);
	int fd_store = open(path, O_RDWR | O_CREAT, S_IRWXU);

	// redirection: fd_store >> STDOUT
	int stdout_save = dup(STDOUT_FILENO);
	close(STDOUT_FILENO);
	dup(fd_store);
	close(fd_store);

	while (pckt_rcv++ < nb_packet) {
	  memset(buff, 0, MSG_LEN);
	  if(-1 == read_from_socket(fd_transfert, buff, MSG_LEN))
	  	break;
	  printf("%s",buff);
	}

	// restore stdout
	close(STDOUT_FILENO);
	dup(stdout_save);
	close(stdout_save);


	strcpy(msg->infos, msg->nick_sender);
	msg->type = FILE_ACK;
	strcpy(msg->nick_sender, nick_name);
	msg->pld_len = -1;

	write_in_socket(sockfd, msg, sizeof(struct message));
}


/*

		`7MM"""Yp,   .g8""8q. `7MN.   `7MF'`7MMF'   `7MF'.M"""bgd
		MM    Yb .dP'    `YM. MMN.    M    MM       M ,MI    "Y
		MM    dP dM'      `MM M YMb   M    MM       M `MMb.
		MM"""bg. MM        MM M  `MN. M    MM       M   `YMMNq.
		MM    `Y MM.      ,MP M   `MM.M    MM       M .     `MM
		MM    ,9 `Mb.    ,dP' M     YMM    YM.     ,M Mb     dM
		.JMMmmmd9    `"bmmd"' .JML.    YM     `bmmmmd"' P"Ybmmd"


*/

void image_sending(struct message* msg, char* buff, char* nick_name, char* file_name, int fd_transfert){
	// converting image for transfert
	f_header_t* f_header = malloc(sizeof(f_header_t));
	i_header_t* i_header = malloc(sizeof(i_header_t));

	char* tmp_path = "___tmp___output___.txt";

	FILE* image = fopen(file_name, "rb");
	int fd_txt = open(tmp_path, O_RDWR | O_CREAT, S_IRWXU);
	complete_header(f_header, i_header, image);

	write_matrix(image, i_header, fd_txt);

	// transfert
	FILE* file_to_send = fopen(tmp_path, "r");

	fseek(file_to_send, 0, SEEK_END);
	int bytes_sent = ftell(file_to_send);
	fseek(file_to_send, 0, SEEK_SET);

	int nb_packet = floor(bytes_sent/MSG_LEN);
	int rest = bytes_sent - nb_packet*MSG_LEN;
	nb_packet += rest > 0;

	// send data to the target
	memset(msg, 0, sizeof(struct message));
	msg->pld_len = -1; // uncommon protocol : must not read a buff after it
	strcpy(msg->nick_sender, nick_name);
	msg->type = IMAGE_SEND;
	sprintf(msg->infos, "%d", nb_packet);
	write_in_socket(fd_transfert, msg, sizeof(struct message));
	write_in_socket(fd_transfert, f_header, FH_SIZE);
	write_in_socket(fd_transfert, i_header, IH_SIZE);

	for (int i = 0; i < nb_packet; i++){
		memset(buff, 0, MSG_LEN);
		fread(buff, sizeof(char), MSG_LEN, file_to_send);
		write_in_socket(fd_transfert, buff, MSG_LEN);
	}

	fclose(file_to_send);
	close(fd_txt);
	free(f_header);
	free(i_header);
	fclose(image);
	unlink(tmp_path);
}

void image_receving(struct message* msg, char* buff, char* nick_name, char* file_name, int fd_transfert, int sockfd){
	f_header_t* f_header = malloc(sizeof(f_header_t));
	i_header_t* i_header = malloc(sizeof(i_header_t));

	read_from_socket(fd_transfert, f_header, FH_SIZE);
	read_from_socket(fd_transfert, i_header, IH_SIZE);


	file_receiving(msg, buff, nick_name, "___tmp___.txt", fd_transfert, sockfd);

	char path[100];
	memset(path, 0, 100);
	sprintf(path, "inbox/%s/___tmp___.txt", nick_name);
	int fd_txt = open(path, O_RDWR | O_CREAT, S_IRWXU);
	printf("RGB file open check\n");
	create_image(f_header, i_header, fd_txt);

	char image_path[8+NICK_LEN+MSG_LEN];
	sprintf(image_path, "inbox/%s/%s", nick_name, file_name);
	rename("new_img.bmp", image_path);

	printf("\033[H\033[2J");

	free(f_header);
	free(i_header);

	close(fd_txt);
	unlink(path);
}
