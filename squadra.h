#ifndef SQUADRA_H
#define SQUADRA_H

#define SIZE_NAME_TEAM 128
#define SIZE_ARRAY_TEAMS 50
#define SIZE_ARRAY_PLAYER_PARTECIPANTI 4

#include "player.h"
#include <pthread.h>

typedef struct{

    char nome_squadra[SIZE_NAME_TEAM];
    player *capitano;
    player *players[SIZE_ARRAY_PLAYER_PARTECIPANTI];
    player *richiestePartecipazione[SIZE_ARRAY_PLAYERS];
    int numeroPlayers;

}squadra;

int aggiungi_nuova_squadra(char *messaggio, int client_socket);

char* serializza_array_squadre();

void *send_lista_squadre_client(void* client_socket);

char *serializza_oggetto_composizione_squadre(int indexSquadra);

void aggiungi_richiesta_partecipazione_squadra(char *messaggio, int client_socket);

void send_aggiornamento_composizione_squadra(char *nomeSquadra);

void aggiornamento_composizione_squadra(char *messaggio);

int cerca_squadra_match(char *messaggio,int sockCapitano);


#endif
