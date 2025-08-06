#ifndef REACTOR_H
#define REACTOR_H

typedef void (*reactorFunc)(int fd);
typedef struct Reactor Reactor;

// Creates and starts new reactor
Reactor* createReactor();

// Adds fd to reactor with callback function
int addFd(Reactor* reactor, int fd, reactorFunc func);

// Removes fd from reactor
int removeFd(Reactor* reactor, int fd);

//Return the current reactor
Reactor* getCurrentReactor();

// Stops and destroys reactor
void stopReactor(Reactor* reactor);

#endif