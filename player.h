#ifndef PLAYER_H
#define PLAYER_H

typedef struct{

    char *nomePlayer;
    int socket;
    int infortunato;

}player;

void aggiungi_utente_connesso(char *messaggio, int client_socket);

#endif
