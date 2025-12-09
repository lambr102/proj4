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

void *worker_thread_function(void *arg){
    connection_queue_t *queue = (connection_queue_t*) arg;
    while (keep_going != 0){
        int fd = connection_queue_dequeue(queue);
        if (fd == -1){
            return NULL;
        }
	    char resources_to_get[256] = {0}; // arbitrary size for now, maybe should change
        if (!read_http_request(fd, resources_to_get)){

            char absolute_path[512]; // again arbitary size
            snprintf(absolute_path, sizeof(resources_to_get), "%s%s", serve_dir, resources_to_get);
            write_http_response(fd, absolute_path);
	    }
        close(fd);
    }
    return NULL;
}


int main(int argc, char **argv) {
   // First argument is directory to serve, second is port
    if (argc != 3) {
        printf("Usage: %s <directory> <port>\n", argv[0]);
        return 1;
    }

    serve_dir = argv[1];
    const char *port = argv[2];

    connection_queue_t queue;
    connection_queue_init(&queue);

    // Handling SIGINT
    struct sigaction sigact;
    sigset_t newset, oset;
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
    sigfillset(&newset);
    sigprocmask(SIG_BLOCK, &newset, &oset);

    int result;
    pthread_t worker_threads[N_THREADS];
    for (int i = 0; i < N_THREADS; i++) {
        if ((result = pthread_create(worker_threads + i, NULL, worker_thread_function,
                                     &queue)) != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(result));
            return 1;
        }
    }


    sigprocmask( SIG_SETMASK, &oset, NULL );


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
        connection_queue_enqueue(&queue, client_fd);
    }

    connection_queue_shutdown(&queue);
    for (int i = 0; i < N_THREADS; i++) {
        if ((result = pthread_join(worker_threads[i], NULL)) != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(result));
        }
    }
    connection_queue_free(&queue);

    return 0;
}
