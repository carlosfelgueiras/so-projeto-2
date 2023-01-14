#include "producer-consumer.h"
#include "betterassert.h"

#include <pthread.h>
#include <stdlib.h>

/*Functions that initialises the producer-consumer queue*/
int pcq_create(pc_queue_t *queue, size_t capacity) {
    // Creating the buffer which will hold the pointers
    queue->pcq_buffer = (void **)malloc(capacity * sizeof(void *));

    if (queue->pcq_buffer == NULL) { // Checks if it was created correctly
        return -1;
    }

    queue->pcq_capacity = capacity;

    // Initializes the various locks in the queue
    if (pthread_mutex_init(&queue->pcq_current_size_lock, NULL) != 0)
        return -1;
    queue->pcq_current_size = 0;

    if (pthread_mutex_init(&queue->pcq_head_lock, NULL) != 0)
        return -1;
    queue->pcq_head = 0;

    if (pthread_mutex_init(&queue->pcq_tail_lock, NULL) != 0)
        return -1;
    queue->pcq_tail = 0;

    if (pthread_mutex_init(&queue->pcq_pusher_condvar_lock, NULL) != 0)
        return -1;

    if (pthread_cond_init(&queue->pcq_pusher_condvar, NULL) != 0)
        return -1;

    if (pthread_mutex_init(&queue->pcq_popper_condvar_lock, NULL) != 0)
        return -1;

    if (pthread_cond_init(&queue->pcq_popper_condvar, NULL) != 0)
        return -1;

    return 0;
}

/*Functions that destroys the producer-consumer queue*/
int pcq_destroy(pc_queue_t *queue) {
    free(queue->pcq_buffer); // Frees the buffer which holds the pointers

    // Destroy the various locks of the queue
    if (pthread_mutex_destroy(&queue->pcq_current_size_lock) != 0)
        return -1;

    if (pthread_mutex_destroy(&queue->pcq_head_lock) != 0)
        return -1;

    if (pthread_mutex_destroy(&queue->pcq_tail_lock) != 0)
        return -1;

    if (pthread_mutex_destroy(&queue->pcq_pusher_condvar_lock) != 0)
        return -1;

    if (pthread_cond_destroy(&queue->pcq_pusher_condvar) != 0)
        return -1;

    if (pthread_mutex_destroy(&queue->pcq_popper_condvar_lock) != 0)
        return -1;

    if (pthread_cond_destroy(&queue->pcq_popper_condvar) != 0)
        return -1;

    return 0;
}

/*Function that adds pointers to the queue*/
int pcq_enqueue(pc_queue_t *queue, void *elem) {
    // Locks the lock associated to the condvar pusher
    if (pthread_mutex_lock(&queue->pcq_pusher_condvar_lock) != 0)
        return -1;

    // If the queue is full, the request will be blocked
    while (queue->pcq_current_size == queue->pcq_capacity)
        pthread_cond_wait(&queue->pcq_pusher_condvar,
                          &queue->pcq_pusher_condvar_lock);

    // Locks the head of the queue, to add the new element
    if (pthread_mutex_lock(&queue->pcq_head_lock) != 0)
        return -1;
    queue->pcq_buffer[queue->pcq_head] = elem;

    queue->pcq_head = (queue->pcq_head + 1) % queue->pcq_capacity;
    pthread_mutex_unlock(&queue->pcq_head_lock);

    // Locks to change the size of the pcq
    if (pthread_mutex_lock(&queue->pcq_current_size_lock) != 0)
        return -1;

    queue->pcq_current_size++;

    if (pthread_mutex_unlock(&queue->pcq_current_size_lock) != 0)
        return -1;

    // Signals the function pcq_dequeue, so that the request may continue
    if (pthread_cond_signal(&queue->pcq_popper_condvar) != 0)
        return -1;

    if (pthread_mutex_unlock(&queue->pcq_pusher_condvar_lock) != 0)
        return -1;

    return 0;
}

/*Function that removers pointers from the queue*/
void *pcq_dequeue(pc_queue_t *queue) {
    void *res;
    // Locks the lock associated to the condvar popper
    if (pthread_mutex_lock(&queue->pcq_popper_condvar_lock) != 0)
        return NULL;

    // If the queue is empty, the request will be blocked
    while (queue->pcq_current_size == 0)
        pthread_cond_wait(&queue->pcq_popper_condvar,
                          &queue->pcq_popper_condvar_lock);

    // Locks the tail so that it acqueires the pointer
    if (pthread_mutex_lock(&queue->pcq_tail_lock) != 0)
        return NULL;

    res = queue->pcq_buffer[queue->pcq_tail];

    queue->pcq_tail = (queue->pcq_tail + 1) % queue->pcq_capacity;

    pthread_mutex_unlock(&queue->pcq_tail_lock);

    // Locks the size to reduce it
    if (pthread_mutex_lock(&queue->pcq_current_size_lock) != 0)
        return NULL;

    queue->pcq_current_size--;

    if (pthread_mutex_unlock(&queue->pcq_current_size_lock) != 0)
        return NULL;

    // Signals the function pcq_enqueue, so that the request may continue
    if (pthread_cond_signal(&queue->pcq_pusher_condvar) != 0)
        return NULL;

    if (pthread_mutex_unlock(&queue->pcq_popper_condvar_lock) != 0)
        return NULL;

    return res;
}