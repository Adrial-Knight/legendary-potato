#if !defined(__CLIENT_COMMAND__H__)
    #define __CLIENT_COMMAND__H__

    int test_nick(char* nick_name);
    void display_list(char* intro, char* content);

    // jalon 1
    void enter_new_nick(char* nick_name);
    void server_ask_nick(struct message* msg, char* buff, char* nick_name, int sockfd);
    void echo(struct message* msg, char* buff, char* nick_name, int sockfd);

    // jalon 2
    void change_nick(struct message* msg, char* buff, char*nick_name, char* new, int sockfd);
    void who_is_online(struct message* msg, char* buff, char* nick_name, int sockfd);
    void who_is_usr(struct message* msg, char* buff, char* nick_name, char* target, int sockfd);
    void msg_all(struct message* msg, char* buff, char* nick_name, int sockfd);
    void msg_private(struct message* msg, char* buff, char* nick_name, char* target, int sockfd);

    // jalon 3
    void new_channel(struct message* msg, char* buff, char* nick_name, char* new, char* channel_name, int sockfd);
    void channel_list(struct message* msg, char* buff, char* nick_name, int sockfd);
    void join_channel(struct message* msg, char* buff, char* nick_name, char* new, char* channel, int sockfd);
    void leave_channel(struct message* msg, char* buff, char* nick_name, char* channel, int sockfd);
    void msg_channel(struct message* msg, char* buff, char* nick_name, char* channel, int sockfd);

    //jalon 4
    void ask_send(struct message* msg, char* buff, char* nick_name, char* target, char* file_name, int sockfd);
    int accept_file_transfert(struct message* msg, char* buff, char* nick_name, int sockfd, char* port);
    void file_sending(struct message* msg, char* buff, char* nick_name, char* file_name, int fd_target);
    void file_receiving(struct message* msg, char* buff, char* nick_name, char* file_name, int fd_transfert, int sockfd);

    //bonus: sending images (only in .bmp format)
    void image_sending(struct message* msg, char* buff, char* nick_name, char* file_name, int fd_transfert);
    void image_receving(struct message* msg, char* buff, char* nick_name, char* file_name, int fd_transfert, int sockfd);

#endif
