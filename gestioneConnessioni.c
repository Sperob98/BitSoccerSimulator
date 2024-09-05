#include "gestioneConnessioni.h"
#include "variabiliGlobali.h"
#include "squadra.h"

char *get_tipo_richiesta(char *messaggio){

    //parse JSON del messaggio
    struct json_object *parsed_json;
    parsed_json = json_tokener_parse(messaggio);

    //Estrazione tipo di richiesta
    struct json_object *tipoRichiesta;
    json_object_object_get_ex(parsed_json, "tipoRichiesta", &tipoRichiesta);

    //Return stringra della richiesta
    return json_object_get_string(tipoRichiesta);

}

void gestione_disconessione_client(int socket_disconessa){

    int indexPlayer;

    //Individuazione del player disconesso
    for(indexPlayer=0; indexPlayer<50; indexPlayer++){

        if(playersConnessi[indexPlayer] != NULL){

            if(playersConnessi[indexPlayer]->socket == socket_disconessa) break;
        }
    }

    //Individuazione di eventuale squadra a cui partecipa
    int trovato = 0;
    int capitano = 0;
    int indexSquadra;

    for(indexSquadra=0; indexSquadra<50; indexSquadra++){

        if(squadreInCostruzione[indexSquadra] != NULL){

            if(squadreInCostruzione[indexSquadra]->capitano == playersConnessi[indexPlayer]){

                trovato = 1;
                capitano = 1;
                break;
            }

            for(int k=0; k<50; k++){

                if(squadreInCostruzione[indexSquadra]->richiestePartecipazione[k] != NULL){

                    if(squadreInCostruzione[indexSquadra]->richiestePartecipazione[k] == playersConnessi[indexPlayer]){

                        squadreInCostruzione[indexSquadra]->richiestePartecipazione[k] = NULL;
                        trovato = 1;
                        printf("Rimossa richiesta di partecipazione di %s alla squadra %s per disconessione\n", playersConnessi[indexPlayer]->nomePlayer,squadreInCostruzione[indexSquadra]->nomeSquadra);
                        break;
                    }
                }

                if(k<4){

                    if(squadreInCostruzione[indexSquadra]->players[k] == playersConnessi[indexPlayer]){

                        squadreInCostruzione[indexSquadra]->players[k] = NULL;
                        trovato = 1;
                        printf("Rimossa partecipazione di %s alla squadra %s per disconessione\n", playersConnessi[indexPlayer]->nomePlayer,squadreInCostruzione[indexSquadra]->nomeSquadra);
                        break;
                    }
                }

            }

        }

        if(trovato == 1) break;
    }

    if(capitano == 1){

        for(int k=0; k<50; k++){

            if(squadreInCostruzione[indexSquadra]->richiestePartecipazione[k] != NULL){

                printf("test1\n");
                int socket = squadreInCostruzione[indexSquadra]->richiestePartecipazione[k]->socket;
                printf("test2\n");
                send(socket,"abbandonoCapitano\n", strlen("abbandonoCapitano\n"),0);
            }

            if(k<4){

                if(squadreInCostruzione[indexSquadra]->players[k] != NULL){

                    int socket = squadreInCostruzione[indexSquadra]->players[k]->socket;
                    printf("test4\n");
                    send(socket,"abbandonoCapitano\n", strlen("abbandonoCapitano\n"),0);
                }
            }
        }

        squadreInCostruzione[indexSquadra] = NULL;

    }else if(trovato == 1){

        send_aggiornamento_composizione_squadra(squadreInCostruzione[indexSquadra]->nomeSquadra);
    }
}
