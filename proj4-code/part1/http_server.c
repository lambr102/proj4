#define _GNU_SOURCE

#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "http.h"

#define BUFSIZE 512
#define LISTEN_QUEUE_LEN 5

int keep_going = 1;

void handle_sigint(int signo) {
    keep_going = 0;
}

int main(int argc, char **argv) {
    // First argument is directory to serve, second is port
    if (argc != 3) {
        printf("Usage: %s <directory> <port>\n", argv[0]);
        return 1;
    }
    // Uncomment the lines below to use these definitions:
    const char *serve_dir = argv[1];
    const char *port = argv[2];

    // Handling SIGINT
    struct sigaction sigact;
    sigact.sa_handler = handle_sigint;
    if (sigfillset(&sigact.sa_mask) ==-1){
        perror("sigfillset");
        return 1;
    }
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

    int val = getaddrinfo(NULL, port, &hints, &server);
    if (val != 0) {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(val));
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
            if (write_http_response(client_fd, absolute_path) == -1){
                perror("write_http_response");
                return 1;
            }
	    }
    }
    close(sock_fd);

    return 0;
}
