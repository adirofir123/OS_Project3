#ifndef PROACTOR_H
#define PROACTOR_H

#include <pthread.h>

typedef void* (*proactorFunc)(int sockfd);

pthread_t startProactor(int listen_sockfd, proactorFunc handler);
int stopProactor(pthread_t tid);

#endif // PROACTOR_H
