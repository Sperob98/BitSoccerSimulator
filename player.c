#include "player.h"
#include "variabiliGlobali.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <json-c/json.h>

void aggiungi_utente_connesso(char *messaggio, int client_socket){

    //Deserializza emssaggio
    struct json_object *parsed_json;
    parsed_json = json_tokener_parse(messaggio);

    json_object *nomePlayer;
    int *socketPlayer;

    json_object_object_get_ex(parsed_json, "utente", &nomePlayer);

    char nomePlayerString[15];
    strcpy(nomePlayerString,json_object_get_string(nomePlayer));


    //Controlla se non esiste già oppure se il server è pieno
    int i;
    pthread_mutex_lock(&mutexPlayers);
    int esiste = 0;
    for(i=0; i<50; i++){

        if(playersConnessi[i] != NULL){

            if(strcmp(playersConnessi[i]->nomePlayer,nomePlayerString) == 0){

                esiste = 1;
                break;
            }
        }
    }
    if(i == 51) esiste = 2; //Server sovraffollato

    //Se non esiste, aggiungi
    if(esiste == 0){

        player *newPlayer = malloc(sizeof(player));
        newPlayer->nomePlayer = malloc(strlen(nomePlayerString) + 1);
        strcpy(newPlayer->nomePlayer,nomePlayerString);
        newPlayer->socket = client_socket;

        newPlayer->infortunato = 0;

        for(int i=0; i<50; i++){

            if(playersConnessi[i] == NULL){

                playersConnessi[i] = newPlayer;
                printf("Connessione player %s aggiunta\n",newPlayer->nomePlayer);
                break;
            }
        }
    }

    pthread_mutex_unlock(&mutexPlayers);

    //Invio messaggio
    if(esiste == 0){


        if (send(client_socket, "ok\n", strlen("ok\n"), 0) < 0) {

                printf("invio risposta inserimento username fallito\n");
                close(client_socket);
            }
    }else if(esiste == 1){

        if (send(client_socket, "busy\n", strlen("busy\n"), 0) < 0) {

                printf("invio risposta inserimento username fallito\n");
                close(client_socket);
            }
    }else{

        if (send(client_socket, "pieno\n", strlen("pieno\n"), 0) < 0) {

                printf("invio risposta inserimento username fallito\n");
                close(client_socket);
            }
    }

}
