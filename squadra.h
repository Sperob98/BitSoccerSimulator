#ifndef SQUADRA_H
#define SQUADRA_H

#include "player.h"
#include <pthread.h>

typedef struct{

    char *nomeSquadra;
    player *capitano;
    player *players[4];
    player *richiestePartecipazione[50];
    int numeroPlayers;
    pthread_mutex_t mutexSquadra;
    pthread_cond_t condSquadra;

}squadra;

int aggiungi_nuova_squadra(char *messaggio, int client_socket);

char* serializza_array_squadre();

void *send_lista_squadre_client(void* client_socket);

char *serializza_oggetto_composizione_squadre(int indexSquadra);

void aggiungi_richiesta_partecipazione_squadra(char *messaggio, int client_socket);

void send_aggiornamento_composizione_squadra(char *nomeSquadra);

void aggiornamento_composizione_squadra(char *messaggio);



/////Da cancellare

void *aggiungi_richiestaPartecipazione_squadra(void* argThread);

void gestione_partecipazione_squadra(char *messaggio);

void *send_aggiornamento_composizione_squadre(void* arg);

#endif
