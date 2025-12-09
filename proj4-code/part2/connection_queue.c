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
        int waiter = pthread_cond_wait(&queue->queue_full, &queue->lock);
	if (waiter != 0){
		fprintf(stderr, "pthread_wait: %s\n", strerror(waiter)); // does this need more error checking?
	}
    }
    if (queue->shutdown){
        if(pthread_mutex_unlock(&queue->lock)){
		perror("pthread_mutex_unlock()");
	}
        return -1;
    }

    queue->client_fds[queue->write_idx] = connection_fd;
    queue->write_idx = (queue->write_idx + 1) % CAPACITY;

    queue ->length++;

    pthread_cond_signal(&queue->queue_empty);
    if(pthread_mutex_unlock(&queue->lock)){
    	perror("pthread_mutex_unlock");	
	return -1;
    }

    return 0;
}

int connection_queue_dequeue(connection_queue_t *queue) {

    pthread_mutex_lock(&queue->lock);
    while(queue->length == 0 && !queue->shutdown){
        pthread_cond_wait(&queue->queue_empty, &queue->lock);
    }
    if (queue->length == 0 && queue->shutdown){
        if(pthread_mutex_unlock(&queue->lock)){
		perror("pthread_mutex_unlock");	
	}
        return -1; //this needs error checking in the larger function because it will be treated as a fd, that was our bug
    }

    int returning_fd;

    returning_fd = queue->client_fds[queue->read_idx];
    queue->client_fds[queue->read_idx] = 0;

    queue->read_idx = (queue->read_idx +1) % CAPACITY;
    queue->length --;

    pthread_cond_signal(&queue->queue_full);
    if(pthread_mutex_unlock(&queue->lock)){
    	perror("pthread_mutex_unlock");
	return -1;
    }


    return returning_fd;
}

int connection_queue_shutdown(connection_queue_t *queue) {
    if(pthread_mutex_lock(&queue->lock)){
    	perror("pthread_mutex_lock");
	return -1;
    }
    queue -> shutdown = 1;
    pthread_cond_broadcast(&queue->queue_empty);
    pthread_cond_broadcast(&queue->queue_full);
    if(pthread_mutex_unlock(&queue->lock)){
    	perror("pthread_mutex_unlock"); 
	return -1;
    }
    return 0;
}

int connection_queue_free(connection_queue_t *queue) {
    pthread_mutex_destroy(&queue->lock);
    pthread_cond_destroy(&queue->queue_full);
    pthread_cond_destroy(&queue->queue_empty);

    return 0;
}
