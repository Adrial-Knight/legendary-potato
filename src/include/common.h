#if !defined(__READWRITE__H__)
    #define __READWRITE__H__

    #define MSG_LEN 1024
    #define CHAN_LEN 16

    int read_from_socket(int fd, void *buf, int size);
    int write_in_socket(int fd, void *buf, int size);
    int socket_listen_and_bind(char *port, int backlog);
    int accept_new_connexion(int listen_fd, char* address, int* port);

#endif
