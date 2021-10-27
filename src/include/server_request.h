#if !defined(__SERVER_REQUEST__H__)
    #define __SERVER_REQUEST__H__

    typedef struct log_client{
    	int fd;
    	int port;
    	char addr[100];
    	char nick_name[NICK_LEN];
    	char time[16];
    	char channel[CHAN_LEN];
    	struct log_client* next;
    }log_client;

    // jalon 1
    void addclient(struct log_client* all_clients, int fd, int port, char* addr);
    void exit_client(struct log_client* all_clients, struct pollfd* pollfd);
    int first_nick_name(struct message* msg, char* buff, struct log_client* all_clients, int client_fd);
    void accept_new_client(struct message* msg, char* buff, struct pollfd* pollfds, int nfds, struct log_client* all_clients, int listen_fd);
    int nick_available(struct message* msg, struct pollfd* pollfd, struct log_client* all_clients);
    void new_nick_name(struct message* msg, char* buff, struct pollfd* pollfd, struct log_client* all_clients);

    // jalon 2
    void who_is_online(struct message* msg, char* buff, struct pollfd* pollfd, struct log_client* all_clients);
    void who_is_usr(struct message* msg, char* buff, struct pollfd* pollfd, log_client* all_clients);
    void msg_all(struct message* msg, char* buff, struct pollfd* pollfd, log_client* all_clients);
    void msg_private(struct message* msg, char* buff, struct pollfd* pollfd, struct log_client* all_clients);
    void echo(struct message* msg, char* buff, struct pollfd* pollfd);

    // jalon 3
    void new_channel(struct message* msg, char* buff, struct pollfd* pollfd, struct log_client* all_clients);
    void channel_list(struct message* msg, char* buff, struct pollfd* pollfd, struct log_client* all_clients);
    void join_channel(struct message* msg, char* buff, struct pollfd* pollfd, struct log_client* all_clients);
    void leave_channel(struct message* msg, char* buff, struct pollfd* pollfd, struct log_client* all_clients);
    void msg_channel(struct message* msg, char* buff, struct pollfd* pollfd, struct log_client* all_clients);

    //jalon 4
    void ask_send(struct message* msg, char* buff, struct pollfd* pollfd, struct log_client* all_clients);
    void accept_file_transfert(struct message* msg, char* buff, struct pollfd* pollfd, struct log_client* all_clients);
    void ack_refuse_file(struct message* msg, char* buff, struct log_client* all_clients);



#endif
