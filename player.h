#ifndef PLAYER_H
#define PLAYER_H

#define SIZE_NAME_PLAYER 128
#define SIZE_ARRAY_PLAYERS 50

typedef struct{

    char nome_player[SIZE_NAME_PLAYER];
    int socket;
    int infortunato;
    int penalizzato;

}player;

void aggiungi_utente_connesso(char *messaggio, int client_socket);

#endif
