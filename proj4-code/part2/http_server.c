#define _GNU_SOURCE

#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "connection_queue.h"
#include "http.h"

#define BUFSIZE 512
#define LISTEN_QUEUE_LEN 5
#define N_THREADS 5

int keep_going = 1;
const char *serve_dir;

void handle_sigint(int signo) {
    keep_going = 0;
}


int main(int argc, char **argv) {
   // First argument is directory to serve, second is port
    if (argc != 3) {
        printf("Usage: %s <directory> <port>\n", argv[0]);
        return 1;
    }

    serve_dir = argv[1];
    const char *port = argv[2];


    // Handling SIGINT
    struct sigaction sigact;
    sigset_t oset;
    sigact.sa_handler = handle_sigint;
    sigfillset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    if (sigaction(SIGINT, &sigact, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo *server;

    int ret_val = getaddrinfo(NULL, port, &hints, &server);
    if (ret_val != 0) {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(ret_val));
        return 1;
    }

    int sock_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    if (sock_fd == -1) {
        perror("socket");
        freeaddrinfo(server);
        return 1;
    }

    if (bind(sock_fd, server->ai_addr, server->ai_addrlen) == -1) {
        perror("bind");
        freeaddrinfo(server);
        close(sock_fd);
        return 1;
    }
    freeaddrinfo(server);

    if (listen(sock_fd, LISTEN_QUEUE_LEN) == -1) {
        perror("listen");
        close(sock_fd);
        return 1;
    }
    sigprocmask(SIG_BLOCK, &sigact.sa_mask, &oset);




    while (keep_going != 0){
        int client_fd = accept(sock_fd, NULL, NULL);
        if (client_fd == -1) {
            if (errno != EINTR) {
                perror("accept");
                close(sock_fd);
                return 1;
            } else {
                break;
            }
        }
	    char resources_to_get[256] = {0}; // arbitrary size for now, maybe should change
        if (!read_http_request(client_fd, resources_to_get)){
            char absolute_path[512]; // again arbitary size
            snprintf(absolute_path, sizeof(resources_to_get), "%s%s", serve_dir, resources_to_get);
            write_http_response(client_fd, absolute_path);
	    }
    }
    close(sock_fd);

    return 0;
}
