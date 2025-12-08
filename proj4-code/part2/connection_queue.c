#include "connection_queue.h"

#include <stdio.h>
#include <string.h>

int connection_queue_init(connection_queue_t *queue) {

    queue -> length = 0;
    queue -> shutdown = 0;
    queue -> write_idx = 0;
    queue -> read_idx = 0;
    pthread_mutex_init(&queue->lock, NULL);
    pthread_cond_init(&queue->queue_full, NULL);
    pthread_cond_init(&queue->queue_empty, NULL);
    return 0;
}

int connection_queue_enqueue(connection_queue_t *queue, int connection_fd) {

    pthread_mutex_lock(&queue->lock);
    while(queue->length == CAPACITY && !queue->shutdown){
        pthread_cond_wait(&queue->queue_full, &queue->lock);
    }
    if (queue->shutdown){
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }

    queue->client_fds[queue->write_idx] = connection_fd;
    queue->write_idx = (queue->write_idx + 1) % CAPACITY;

    queue ->length++;

    pthread_cond_signal(&queue->queue_empty);
    pthread_mutex_unlock(&queue->lock);

    return 0;
}

int connection_queue_dequeue(connection_queue_t *queue) {

    pthread_mutex_lock(&queue->lock);
    while(queue->length == 0){
        pthread_cond_wait(&queue->queue_empty, &queue->lock);
    }

    return 0;
}

int connection_queue_shutdown(connection_queue_t *queue) {
    // TODO Not yet implemented
    return 0;
}

int connection_queue_free(connection_queue_t *queue) {
    // TODO Not yet implemented
    return 0;
}
