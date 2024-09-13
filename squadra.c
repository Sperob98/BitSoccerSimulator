#include "squadra.h"
#include "variabiliGlobali.h"
#include "gestioneThread.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <json-c/json.h>



char* serializza_squadra(const squadra* squadra) {

    json_object *jobj = json_object_new_object();
    json_object_object_add(jobj, "nomeSquadra", json_object_new_string(squadra->nomeSquadra));
    json_object_object_add(jobj, "capitano", json_object_new_int(squadra->capitano));
    //Creazione di un array JSON per i giocatori
    json_object *jarray = json_object_new_array();
    for (int i = 0; squadra->players[i] != NULL; i++) {
        json_object_array_add(jarray, json_object_new_string(squadra->players[i]));
    }
    json_object_object_add(jobj, "players", jarray);

    const char *json_str = json_object_to_json_string(jobj);
    char *json_copy = strdup(json_str); // Copia la stringa JSON per restituirla
    json_object_put(jobj); // Dealloca l'oggetto JSON

    return json_copy;
}

int aggiungi_nuova_squadra(char *messaggio, int client_socket) {

    int statoDiCreazione = 0;

    //Deserializzazione messaggio
    struct json_object *parsed_json;
    parsed_json = json_tokener_parse(messaggio);

    json_object *nomeSquadra;
    json_object *capitano;

    json_object_object_get_ex(parsed_json, "squadra", &nomeSquadra);
    json_object_object_get_ex(parsed_json, "capitano", &capitano);

    //Controlla se il nome squadra è unico
    pthread_mutex_lock(&mutexListaSquadre); //Prendi mutex dell'array
    for(int i=0; i<50; i++){

        if(squadreInCostruzione[i] != NULL){

            if(strcmp(squadreInCostruzione[i]->nomeSquadra,json_object_get_string(nomeSquadra)) == 0){

                //Invio stato di fallimento creazione, nome non unico
                if (send(client_socket, "statoCreazione\n", strlen("statoCreazione\n"), 0) < 0) {

                    printf("Errore nella send di risposta creazione squadra\n");
                    //Se fallisce la send eliminare il player dall'array..
                }

                if (send(client_socket, "busy\n", strlen("busy\n"), 0) < 0) {

                    //Se fallisce la send eliminare il player dall'array..
                }

                pthread_mutex_unlock(&mutexListaSquadre);

                return statoDiCreazione;
            }
        }
    }

    //Creazione della struttura della nuova squadra
    squadra *newSquadra = malloc(sizeof(squadra));

    //assegnazione nome della squadra
    newSquadra->nomeSquadra = malloc(strlen(json_object_get_string(nomeSquadra)) + 1);
    strcpy(newSquadra->nomeSquadra, json_object_get_string(nomeSquadra));

    //Mutua escusione
    pthread_mutex_lock(&mutexPlayers);

    //Cerca il capitano nell'array globale e lo assegna alla nuova squdra creata
    int i;
    for(i=0; i<50; i++){

        if(playersConnessi[i] != NULL){

            if(strcmp(playersConnessi[i]->nomePlayer,json_object_get_string(capitano)) == 0){

                //Assegnazione capitano
                newSquadra->capitano = playersConnessi[i];

                break;
            }
        }
    }


    newSquadra->numeroPlayers = 1;

    //Inizializza array players
    for(int k=0; k<4; k++){

        newSquadra->players[k] = NULL;
    }

    //Inizialliza array richieste
     for(int k=0; k<50; k++){

        newSquadra->richiestePartecipazione[k] = NULL;
    }

    pthread_mutex_init(&(newSquadra->mutexSquadra), NULL);

    pthread_cond_init(&(newSquadra->condSquadra), NULL);

    //Aggiunta della nuova squadra nell'array globale delle squadre
        for(i=0;i<50;i++){

        if(squadreInCostruzione[i] == NULL) break;
    }

    if(i < 50){

        squadreInCostruzione[i] = newSquadra;
        printf("Squadra %s fondata dal capitano %s\n", newSquadra->nomeSquadra,newSquadra->capitano->nomePlayer);
        statoDiCreazione = 1;

        //Invio stato di successo
        if (send(client_socket, "statoCreazione\n", strlen("statoCreazione\n"), 0) < 0) {

            printf("Errore nella send di risposta creazione squadra\n");
            //Se fallisce la send eliminare il player dall'array..
        }

        if (send(client_socket, "ok\n", strlen("ok\n"), 0) < 0) {

            printf("Errore nelal send di risposta creazione squadra\n");
            //Se fallisce la send eliminare il player dall'array..
        }

    }else{

        printf("Squadra %s non aggiunta, lista piena\n", newSquadra->nomeSquadra);
        free(newSquadra);

        //Invio stato di fallimento creazione per il limite massimo
        if (send(client_socket, "statoCreazione\n", strlen("statoCreazione\n"), 0) < 0) {

            printf("Errore nella send di risposta creazione squadra\n");
            //Se fallisce la send eliminare il player dall'array..
        }

        if (send(client_socket, "max\n", strlen("max\n"), 0) < 0) {

            //Se fallisce la send eliminare il player dall'array..
        }
    }

    //Rilascia mutex dei players e delle squadre
    pthread_mutex_unlock(&mutexPlayers);
    pthread_mutex_unlock(&mutexListaSquadre);

    return statoDiCreazione;
}

char* serializza_array_squadre() {

    json_object *jarray = json_object_new_array();

    for (int i = 0; i<50; i++) {

            if(squadreInCostruzione[i] != NULL){

                json_object *jobj = json_object_new_object();
                json_object_object_add(jobj, "nomeSquadra", json_object_new_string(squadreInCostruzione[i]->nomeSquadra));
                json_object_object_add(jobj, "capitano", json_object_new_string(squadreInCostruzione[i]->capitano->nomePlayer));
                json_object_object_add(jobj, "numeroPlayers", json_object_new_int(squadreInCostruzione[i]->numeroPlayers));
                json_object_array_add(jarray, jobj);
        }
    }

    const char *json_str = json_object_to_json_string(jarray);
    char *json_copy = strdup(json_str); // Copia la stringa JSON per restituirla
    json_object_put(jarray); // Dealloca l'oggetto JSON

    return json_copy;
}

void *send_lista_squadre_client(void* client_socket){

    printf("Nuovo thread(invio lista squadre) creato\n");

    int client = *(int*)client_socket;
    char msg_daInviare[1024];
    int bytes_read;
    int aggiornamentoLista = 1;

    while(1){

        pthread_mutex_lock(&mutexListaSquadre);
        while(aggiornamentoLista == 0){

            pthread_cond_wait(&condListaSquadre,&mutexListaSquadre);
            aggiornamentoLista = 1;
        }

        char tipoDiMessaggio[50];
        strcpy(tipoDiMessaggio,"AggiornamentoSquadra\n");
        if (send(client, tipoDiMessaggio, strlen(tipoDiMessaggio), 0) < 0) {
                perror("Errore invio dati");
                pthread_mutex_unlock(&mutexListaSquadre);
                printf("thread in chiusura\n");
                pthread_exit(NULL);
            }
        printf("Messaggio inviato %s", tipoDiMessaggio);


        char *json__arraySquadre_str = serializza_array_squadre();
        char buffer[2048];
        snprintf(buffer, sizeof(buffer), "%s\n", json__arraySquadre_str);
            if (send(client, buffer, strlen(buffer), 0) < 0) {
                perror("Errore invio dati");
                pthread_mutex_unlock(&mutexListaSquadre);
                printf("thread in chiusura\n");
                pthread_exit(NULL);
            }

        printf("Messaggio inviato %s\n", json__arraySquadre_str);

        aggiornamentoLista = 0;

        pthread_mutex_unlock(&mutexListaSquadre);
    }

}

char *serializza_oggetto_composizione_squadre(int indexSquadra){

    // Creazione dei due array JSON di stringhe
    struct json_object *json_array_richieste = json_object_new_array();
    struct json_object *json_array_accettati = json_object_new_array();

    if(squadreInCostruzione[indexSquadra]!= NULL){

    //Costruisce l'array dei player che hanno fatto richiesta di partecipazione
         for(int j=0; j<50; j++){

            if(squadreInCostruzione[indexSquadra]->richiestePartecipazione[j] != NULL){

                char richiesta[20];
                strcpy(richiesta,squadreInCostruzione[indexSquadra]->richiestePartecipazione[j]->nomePlayer);
                json_object_array_add(json_array_richieste, json_object_new_string(richiesta));
            }
        }

    //Costruisce l'array dei player accettati nella squadra
        char playerCapitano[50];
        strcpy(playerCapitano,squadreInCostruzione[indexSquadra]->capitano->nomePlayer);
        strcat(playerCapitano," (capitano)\0");
        json_object_array_add(json_array_accettati, json_object_new_string(playerCapitano));
        for(int j=0; j<4; j++){

            if(squadreInCostruzione[indexSquadra]->players[j] != NULL){
                printf("numero j: %d\n",j);
                char playerAccettato[20];
                strcpy(playerAccettato,squadreInCostruzione[indexSquadra]->players[j]->nomePlayer);
                json_object_array_add(json_array_accettati, json_object_new_string(playerAccettato));
            }
        }
    }

    //Creazione dell'oggetto principale che contiene i due array
    struct json_object *root = json_object_new_object();
    json_object_object_add(root, "richieste", json_array_richieste);
    json_object_object_add(root, "accettati", json_array_accettati);

    // Serializza l'oggetto JSON in una stringa
    char *json_string = json_object_to_json_string(root);
    char delimitatore[] = "\n";
    strcat(json_string,delimitatore);

    return json_string;
}

void send_aggiornamento_composizione_squadra(char *nomeSquadra){

    //indice for
    int i;

    //Cerca la squdra nell'array
    for(i=0;i<50;i++){

        if(squadreInCostruzione[i] != NULL){

            if(strcmp(squadreInCostruzione[i]->nomeSquadra,nomeSquadra) == 0) break;
        }
    }

    //Recupero degli oggetti che contengono array richieste e l'array accettati da mandare a client dello spogliatolio della squadra
    char *oggettoDaInviare = serializza_oggetto_composizione_squadre(i);

    //Aggiorna tutti i player client (e il capitano) della lista richieste

    //Aggiornamento al capitano
    int socket_player = squadreInCostruzione[i]->capitano->socket;
    send(socket_player,"AggiornamentoComposizioneSquadra\n",strlen("AggiornamentoComposizioneSquadra\n"),0);
    send(socket_player,oggettoDaInviare,strlen(oggettoDaInviare),0);
    printf("Inviato l'oggetto: %s al capitano %s\n",oggettoDaInviare,squadreInCostruzione[i]->capitano->nomePlayer);

    //Aggiornamento ai player che sono in lista di richiesta
    for(int k=0; k<50; k++){

        if(squadreInCostruzione[i]->richiestePartecipazione[k] != NULL){

            int socket_player = squadreInCostruzione[i]->richiestePartecipazione[k]->socket;
            send(socket_player,"AggiornamentoComposizioneSquadra\n",strlen("AggiornamentoComposizioneSquadra\n"),0);
            send(socket_player,oggettoDaInviare,strlen(oggettoDaInviare),0);
            printf("Inviato l'oggetto: %s al player %s\n",oggettoDaInviare,squadreInCostruzione[i]->richiestePartecipazione[k]->nomePlayer);
        }
    }

    //Aggiornamento dei player accettati
    for(int k=0; k<4; k++){

        if(squadreInCostruzione[i]->players[k] != NULL){

            int socket_player = squadreInCostruzione[i]->players[k]->socket;
            send(socket_player,"AggiornamentoComposizioneSquadra\n",strlen("AggiornamentoComposizioneSquadra\n"),0);
            send(socket_player,oggettoDaInviare,strlen(oggettoDaInviare),0);
            printf("Inviato l'oggetto: %s al player %s\n",oggettoDaInviare,squadreInCostruzione[i]->players[k]->nomePlayer);
        }
    }

}

void aggiungi_richiesta_partecipazione_squadra(char *messaggio, int client_socket){

        //Deserializzazione del messaggio JSON
        struct json_object *parsed_json;
        parsed_json = json_tokener_parse(messaggio);

        //Estrazione squadra a cui partecipare
        struct json_object *squadraJSON;
        json_object_object_get_ex(parsed_json, "squadra", &squadraJSON);
        char nomeSquadra[100];
        strcpy(nomeSquadra,json_object_get_string(squadraJSON));

        //Estrazione nomePlayer
        struct json_object *playerJSON;
        json_object_object_get_ex(parsed_json, "player", &playerJSON);
        char nomePlayer[50];
        strcpy(nomePlayer,json_object_get_string(playerJSON));

        //Dichiarazioni indici for
        int indexSquadra;
        int indexPlayer;

        //Cerca indice squadra a cui vuole partecipare il player nell'array globale
        for(indexSquadra=0; indexSquadra<50; indexSquadra++){

            if(squadreInCostruzione[indexSquadra] != NULL){

                if(strcmp(squadreInCostruzione[indexSquadra]->nomeSquadra,nomeSquadra) == 0) break;
            }
        }

        //Cerca indice player che ha fatto la richiesta nell'array globale
        for(indexPlayer=0; indexPlayer<50; indexPlayer++){

            if(playersConnessi[indexPlayer] != NULL){

                if(strcmp(playersConnessi[indexPlayer]->nomePlayer,nomePlayer) == 0) break;
            }
        }

        if(indexSquadra < 50 && indexPlayer < 50 ){
        //Cerca il primo slot di richieste libero per aggiungere la richiesta di partecipazione
            for(int k=0; k<50; k++){

                if(squadreInCostruzione[indexSquadra]->richiestePartecipazione[k] == NULL){

                    squadreInCostruzione[indexSquadra]->richiestePartecipazione[k] = playersConnessi[indexPlayer];
                    printf("Aggiunta richiesta di partecipazione del player %s alla squadra %s\n",nomePlayer,nomeSquadra);
                    break;
                }
            }

            //Risposta di successo
            if (send(client_socket, "statoRichiesta\n", strlen("statoRichiesta\n"), 0) < 0) {

                printf("Errore nella send di risposta richiesta di partecipazione\n");
                //Se fallisce la send eliminare il player dall'array..
            }

            if (send(client_socket, "ok\n", strlen("ok\n"), 0) < 0) {

                //Se fallisce la send eliminare il player dall'array..
            }

            send_aggiornamento_composizione_squadra(nomeSquadra);

            return;
        }


        //Risposta di fallimento
        if (send(client_socket, "statoRichiesta\n", strlen("statoRichiesta\n"), 0) < 0) {

            printf("Errore nella send di risposta richiesta di partecipazione\n");
            //Se fallisce la send eliminare il player dall'array..
        }

        if (send(client_socket, "ko\n", strlen("ko\n"), 0) < 0) {

            //Se fallisce la send eliminare il player dall'array..
        }

        return;
}

void aggiornamento_composizione_squadra(char *messaggio){

    //Deserializzazione del messaggio
    struct json_object *parsed_json;
    parsed_json = json_tokener_parse(messaggio);

    json_object *nomeSquadra;
    json_object *nomePlayer;
    json_object *decisioneCapitano;

    json_object_object_get_ex(parsed_json, "squadra", &nomeSquadra);
    json_object_object_get_ex(parsed_json, "nomePlayer", &nomePlayer);
    json_object_object_get_ex(parsed_json, "decisione", &decisioneCapitano);


    char *squadraString = malloc(strlen(json_object_get_string(nomeSquadra)) + 1);
    strcpy(squadraString,json_object_get_string(nomeSquadra));

    char *playerString = malloc(strlen(json_object_get_string(nomePlayer)) + 1);
    strcpy(playerString,json_object_get_string(nomePlayer));

    char *decisioneString = malloc(strlen(json_object_get_string(decisioneCapitano)) + 1);
    strcpy(decisioneString,json_object_get_string(decisioneCapitano));

    //indice for che individua la squadra nell'array
    int indexSquadra;
    //indice for che individua il player nell'array
    int indexPlayer;

    //Ricerca della squadra con cui il capitano ha preso un decisione
    for(indexSquadra=0; indexSquadra<50; indexSquadra++){

        if(squadreInCostruzione[indexSquadra] != NULL){

            if(strcmp(squadreInCostruzione[indexSquadra]->nomeSquadra,squadraString) == 0) break;
        }

    }

    //Ricerca del player della squadra il cui capitano ha preso una decisione
    for(indexPlayer=0; indexPlayer<50; indexPlayer++){

        if(playersConnessi[indexPlayer] != NULL){

            if(strcmp(playersConnessi[indexPlayer]->nomePlayer,playerString) == 0) break;
        }
    }

    if(indexSquadra > 49 || indexPlayer > 49){

        //Errore nell'indivuazione della squadra o del player, forse sono discoenssi
        int socketCapitano = squadreInCostruzione[indexSquadra]->capitano->socket;
        send(socketCapitano, "rispostaDecisione\n", strlen("rispostaDecisione\n"),0);
        send(socketCapitano, "ko\n", strlen("ko\n"),0);

        return;

    }


    //Rimozione del player tra le richieste a prescindere se accettato o rifiutato
    for(int k=0; k<50; k++){

        if(squadreInCostruzione[indexSquadra]->richiestePartecipazione[k] != NULL){

            if(strcmp(squadreInCostruzione[indexSquadra]->richiestePartecipazione[k]->nomePlayer,playerString) == 0){

                squadreInCostruzione[indexSquadra]->richiestePartecipazione[k] = NULL;
                printf("Rimosso il player %s tra le richieste della squadra %s\n",playerString,squadraString);

                break;
            }
        }
    }

    //Se accettato, inserisce nella lista accettati
    if(strcmp(decisioneString,"accettato") == 0){

        int k;

        for(k=0; k<4; k++){

            if(squadreInCostruzione[indexSquadra]->players[k] == NULL){

                squadreInCostruzione[indexSquadra]->players[k] = playersConnessi[indexPlayer];
                (squadreInCostruzione[indexSquadra]->numeroPlayers)++;
                printf("Aggiunto il player %s nella squadra %s \n",nomePlayer,squadraString);

                break;
            }
        }

        //Avviso squadra al completo
        if(k > 3){

            int socketCapitano = squadreInCostruzione[indexSquadra]->capitano->socket;
            send(socketCapitano, "rispostaDecisione\n", strlen("rispostaDecisione\n"),0);
            send(socketCapitano, "full\n", strlen("full\n"),0);

            return;
        }
    }else{

        //Avverti il client di essere stato rimosso
        int socketPlayer = playersConnessi[indexPlayer]->socket;
        send(socketPlayer, "rimozione\n", strlen("rimozione\n"),0);

    }

    int socketCapitano = squadreInCostruzione[indexSquadra]->capitano->socket;
    send(socketCapitano, "rispostaDecisione\n", strlen("rispostaDecisione\n"),0);
    send(socketCapitano, "ok\n", strlen("ok\n"),0);

    return;

}

int cerca_squadra_match(char *messaggio, int sockCapitano){

    //Deserializzazione del messaggio
    struct json_object *parsed_json;
    parsed_json = json_tokener_parse(messaggio);

    json_object *nomeSquadra;

    json_object_object_get_ex(parsed_json, "squadra", &nomeSquadra);

    char *squadraString = malloc(strlen(json_object_get_string(nomeSquadra)) + 1);
    strcpy(squadraString,json_object_get_string(nomeSquadra));

    //Cerca l'indice della squadra che ha chiesto il match nell'array delle squadreInCostruzione
    int indexSquadreInCostruzione;
    for(indexSquadreInCostruzione=0; indexSquadreInCostruzione<50; indexSquadreInCostruzione++){

        if(squadreInCostruzione[indexSquadreInCostruzione] != NULL){

            int risultatoCmp = strcmp(squadreInCostruzione[indexSquadreInCostruzione]->nomeSquadra,squadraString);
            if(risultatoCmp == 0) break;
        }
    }

    if(indexSquadreInCostruzione > 49){ //Caso fallimento nella ricerca della squadra

        printf("Stato richiesta match fallito\n");

        //Avverti capitano del fallimento della richiesta
        send(sockCapitano,"rispostaAlCapitano\n", strlen("rispostaAlCapitano\n"),0);
        send(sockCapitano,"ko\n", strlen("ko\n"),0);

        return -2;
    }

    //Cerca se nell'array delle squadre pronte c'è una squadra pronta (diversa dalla squadra che ha chiesto il match)
    int matchTrovato = 0;
    int indexSquadrePronte;
    for(indexSquadrePronte=0; indexSquadrePronte<15; indexSquadrePronte++){

        if(squadreComplete[indexSquadrePronte] != NULL){

            if(strcmp(squadreComplete[indexSquadrePronte]->nomeSquadra, squadraString) != 0){

                matchTrovato = 1;
                break;
            }
        }
    }

    //Se match trovato aggiungi partita e libera gli array squadreIncostruzione/Pronte
    if(matchTrovato == 1){

        int i;
        for(i=0; i<5; i++){ //Aggiunge partita nell'array delle partite

            if(partite[i] == NULL){

                partite[i] = malloc(sizeof(partita));
                partite[i]->squadra_A = squadreInCostruzione[indexSquadreInCostruzione];
                partite[i]->squadra_B = squadreComplete[indexSquadrePronte];
                strcpy(partite[i]->inizioTurno,"null");
                partite[i]->finePartita = 0;
                partite[i]->inizioPartita = 0;
                printf("Aggiunta partita: %s vs %s\n",squadraString,squadreComplete[indexSquadrePronte]->nomeSquadra);
                break;
            }
        }

        if(i>4){ //Fallimento caso array partite pieno

            //Avverti il capitano che la richiesta è fallita
            send(sockCapitano,"rispostaAlCapitano\n", strlen("rispostaAlCapitano\n"),0);
            send(sockCapitano,"ko\n", strlen("ko\n"),0);

            printf("Stato richiesta match fallito\n");

            return -2;
        }

        //Libera gli array
        squadreInCostruzione[indexSquadreInCostruzione] = NULL;
        squadreComplete[indexSquadrePronte] = NULL;

        //Avverti il capitano che la richiesta ha avuto successo
        send(sockCapitano,"rispostaAlCapitano\n", strlen("rispostaAlCapitano\n"),0);
        send(sockCapitano,"ok\n", strlen("ok\n"),0);

        //Ritorna indice della partita creata
        return i;

    }else{//Aggiungi squadra nelle squadre pronte

        int i;
        for(i=0; i<15; i++){

            if(squadreComplete[i] == NULL){

                squadreComplete[i] = squadreInCostruzione[indexSquadreInCostruzione];
                squadreInCostruzione[indexSquadreInCostruzione] = NULL;

                printf("Aggiunta la squadra %s in attesa di match\n",squadraString);

                //Avverti il client che la richiesta ha avuto successo
                send(sockCapitano,"rispostaAlCapitano\n", strlen("rispostaAlCapitano\n"),0);
                send(sockCapitano,"ok\n", strlen("ok\n"),0);

                return -1;;
            }
        }

        if(i > 14){

            //Avverti il capitano della richiesta
            send(sockCapitano,"rispostaAlCapitano\n", strlen("rispostaAlCapitano\n"),0);
            send(sockCapitano,"ko\n", strlen("ko\n"),0);

            printf("Stato richiesta match fallito\n");

        }

        return -2;
    }
}
