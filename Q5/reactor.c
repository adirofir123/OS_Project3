#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <pthread.h>
#include "reactor.h"

#define MAX_FDS 1024

static Reactor* currentReactor = NULL;

struct Reactor {
    fd_set fds;
    reactorFunc handlers[MAX_FDS];
    int max_fd;
    int running;
    pthread_t thread;
};

static void* reactorLoop(void* arg) {
    Reactor* reactor = (Reactor*)arg;
    
    while (reactor->running) {
        fd_set read_fds = reactor->fds;
        struct timeval timeout = { .tv_sec = 1, .tv_usec = 0 };
        
        int activity = select(reactor->max_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (activity < 0) {
            perror("select error");
            continue;
        }
        
        if (activity == 0) { // Timeout
            continue;
        }
        
        for (int fd = 0; fd <= reactor->max_fd; fd++) {
            if (FD_ISSET(fd, &read_fds) && reactor->handlers[fd]) {
                reactor->handlers[fd](fd);
            }
        }
    }
    return NULL;
}

Reactor* createReactor() {
    Reactor* reactor = (Reactor*)malloc(sizeof(Reactor));
    if (!reactor) return NULL;
    
    FD_ZERO(&reactor->fds);
    memset(reactor->handlers, 0, sizeof(reactor->handlers));
    reactor->max_fd = -1;
    reactor->running = 1;
    
    if (pthread_create(&reactor->thread, NULL, reactorLoop, reactor) != 0) {
        free(reactor);
        return NULL;
    }
    
    currentReactor = reactor;
    return reactor;
}

int addFd(Reactor* reactor, int fd, reactorFunc func) {
    if (fd < 0 || fd >= MAX_FDS || !reactor || !func) return -1;
    
    FD_SET(fd, &reactor->fds);
    reactor->handlers[fd] = func;
    
    if (fd > reactor->max_fd) {
        reactor->max_fd = fd;
    }
    
    return 0;
}

int removeFd(Reactor* reactor, int fd) {
    if (fd < 0 || fd >= MAX_FDS || !reactor) return -1;
    
    FD_CLR(fd, &reactor->fds);
    reactor->handlers[fd] = NULL;
    
    // Update max_fd if needed
    if (fd == reactor->max_fd) {
        while (reactor->max_fd >= 0 && !FD_ISSET(reactor->max_fd, &reactor->fds)) {
            reactor->max_fd--;
        }
    }
    
    return 0;
}

Reactor* getCurrentReactor() {
    return currentReactor;
}

void stopReactor(Reactor* reactor) {
    if (!reactor) return;
    
    reactor->running = 0;
    pthread_join(reactor->thread, NULL);
    
    for (int fd = 0; fd <= reactor->max_fd; fd++) {
        if (FD_ISSET(fd, &reactor->fds)) {
            FD_CLR(fd, &reactor->fds);
        }
    }

    currentReactor = NULL;
    free(reactor);
    
}