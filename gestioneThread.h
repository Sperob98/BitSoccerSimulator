#ifndef GESTIONE_THREAD_H
#define GESTIONE_THREAD_H

#include <pthread.h>

typedef struct {

    pthread_t thread_id;
    int clientSock;
    char messaggio[2000];

}thread_data;


#endif
