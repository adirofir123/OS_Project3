#include "proactor.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>

// Struct for initial accept thread setup
typedef struct {
    int sockfd;
    proactorFunc func;
} ProactorArgs;

// Struct passed to client thread via adapter
typedef struct {
    proactorFunc func;
    int fd;
} TaskArgs;

// Adapter function that bridges void* thread call to int-based handler
void* thread_adapter(void* arg) {
    TaskArgs* args = (TaskArgs*)arg;
    void* result = args->func(args->fd);  // call the original user function
    free(args);  // clean up
    return result;
}

// Accept loop run by the accept thread
void* accept_loop(void* arg) {
    ProactorArgs* args = (ProactorArgs*)arg;
    int sockfd = args->sockfd;
    proactorFunc func = args->func;
    free(args);

    while (1) {
        struct sockaddr_storage their_addr;
        socklen_t sin_size = sizeof(their_addr);
        int client_fd = accept(sockfd, (struct sockaddr*)&their_addr, &sin_size);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }
        printf("New connection accepted: socket %d\n", client_fd);
        // Prepare args for the new client thread
        TaskArgs* task = malloc(sizeof(TaskArgs));
        task->func = func;
        task->fd = client_fd;

        pthread_t tid;
        pthread_create(&tid, NULL, thread_adapter, task);
        pthread_detach(tid);
    }

    return NULL;
}

pthread_t startProactor(int sockfd, proactorFunc handler) {
    pthread_t accept_tid;
    ProactorArgs* args = malloc(sizeof(ProactorArgs));
    args->sockfd = sockfd;
    args->func = handler;
    pthread_create(&accept_tid, NULL, accept_loop, args);
    return accept_tid;
}

int stopProactor(pthread_t tid) {
    return pthread_cancel(tid);
}
