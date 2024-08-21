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

    pthread_mutex_lock(&mutexListaSquadre);//prende mutex array squadre
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

    pthread_mutex_unlock(&mutexListaSquadre); //Rilascia mutex array squadre

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

        pthread_mutex_unlock(&mutexListaSquadre);
        char *json__arraySquadre_str = serializza_array_squadre();
        pthread_mutex_lock(&mutexListaSquadre);
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

    //Assumi mutex
    pthread_mutex_lock(&mutexListaSquadre);
    pthread_mutex_lock(&mutexPlayers);

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

    //Assumi mutex
    pthread_mutex_unlock(&mutexListaSquadre);
    pthread_mutex_unlock(&mutexPlayers);

    return json_string;
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

        //Acquisizione mutex degli array globali
        pthread_mutex_lock(&mutexListaSquadre);
        pthread_mutex_lock(&mutexPlayers);

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

            //Recupero degli oggetti che contengono array richieste e l'array accettati da mandare a client dello spogliatolio della squadra
            pthread_mutex_unlock(&mutexListaSquadre);
            pthread_mutex_unlock(&mutexPlayers);
            char *oggettoDaInviare = serializza_oggetto_composizione_squadre(indexSquadra);
            pthread_mutex_lock(&mutexListaSquadre);
            pthread_mutex_lock(&mutexPlayers);

            //Aggiorna tutti i player client (e il capitano) della lista richieste
            for(int k=0; k<50; k++){

                if(squadreInCostruzione[indexSquadra]->richiestePartecipazione[k] != NULL){

                    int socket_player = squadreInCostruzione[indexSquadra]->richiestePartecipazione[k]->socket;
                    send(socket_player,"AggiornamentoComposizioneSquadra\n",strlen("AggiornamentoComposizioneSquadra\n"),0);
                    send(socket_player,oggettoDaInviare,strlen(oggettoDaInviare),0);
                    printf("Inviato l'oggetto: %s al player %s\n",oggettoDaInviare,squadreInCostruzione[indexSquadra]->richiestePartecipazione[k]->nomePlayer);


                }
            }

            for(int k=0; k<4; k++){

                if(squadreInCostruzione[indexSquadra]->players[k] != NULL){

                    int socket_player = squadreInCostruzione[indexSquadra]->players[k]->socket;
                    send(socket_player,"AggiornamentoComposizioneSquadra\n",strlen("AggiornamentoComposizioneSquadra\n"),0);
                    send(socket_player,oggettoDaInviare,strlen(oggettoDaInviare),0);
                    printf("Inviato l'oggetto: %s al player %s\n",oggettoDaInviare,squadreInCostruzione[indexSquadra]->players[k]->nomePlayer);
                }
            }

            int socket_player = squadreInCostruzione[indexSquadra]->capitano->socket;
            send(socket_player,"AggiornamentoComposizioneSquadra\n",strlen("AggiornamentoComposizioneSquadra\n"),0);
            send(socket_player,oggettoDaInviare,strlen(oggettoDaInviare),0);
            printf("Inviato l'oggetto: %s al capitano %s\n",oggettoDaInviare,squadreInCostruzione[indexSquadra]->capitano->nomePlayer);


            //Rilascio mutex degli array globali
            pthread_mutex_unlock(&mutexListaSquadre);
            pthread_mutex_unlock(&mutexPlayers);

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

        //Rilascio mutex degli array globali
        pthread_mutex_unlock(&mutexListaSquadre);
        pthread_mutex_unlock(&mutexPlayers);

        return;
}


////////////////////////////////////////DEPRECATE//////////////////////////////////////////////////////////////////

void *aggiungi_richiestaPartecipazione_squadra(void* arg){

    printf("Nuovo thread \"rihciesta partecipazione\" creato, in attesa che il capitano prende una decisione\n");

    char msg_daInviare[1024];
    int bytes_read;
    int i;

    thread_data *datiThread = (thread_data*)arg;

    if(datiThread != NULL){

        //parse JSON del messaggio
        struct json_object *parsed_json;
        parsed_json = json_tokener_parse(datiThread->messaggio);

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

        //Cerca la posizione dell'array in cui si trova la squadra a cui ha chiesto la partecipazione
        pthread_mutex_lock(&mutexListaSquadre); //Sincronizzazione poiché si tenta una lettura variabile globale
        for(i=0; i<50; i++){

            if(squadreInCostruzione[i] != NULL)
                if(strcmp(nomeSquadra,squadreInCostruzione[i]->nomeSquadra)==0) break;

        }

        //Aggiunge richiesta nella squadra e mette il client in attesa della decisione del capitano
        if(squadreInCostruzione[i]!=NULL){

            int j;
            for(j=0;j<50;j++){ //trova la prima posizione libera dell'array

                if((strcmp(squadreInCostruzione[i]->richiestePartecipazione[j],"null") == 0)){

                    strcpy(squadreInCostruzione[i]->richiestePartecipazione[j],nomePlayer);
                    printf("richiesta del player: %s aggiunta tra le ricieste della squadra: %s\n",nomePlayer,squadreInCostruzione[i]->nomeSquadra);
                    break;
                }
            }

            pthread_mutex_unlock(&mutexListaSquadre); //Rilascia mutex
            printf("sto per svegliare il thread\n");
            pthread_cond_broadcast(&condSquadra);
            pthread_mutex_lock(&mutexListaSquadre);

            int decisione = 0;
            //In attesa della decisione del capitano
            while(decisione == 0){ //Controlla se il capitano ha preso una decisione, se non l'ha presa rimetti in attesa. 0=non presa; 1=accettato; 2=rifiutato o squadra chiusa

                pthread_mutex_unlock(&mutexListaSquadre); //Rilascia mutex poiché va in attesa
                pthread_mutex_lock(&mutexDecisioneCap);
                pthread_cond_wait(&condDecisioneCap,&mutexDecisioneCap);
                pthread_mutex_lock(&mutexListaSquadre);
                if(squadreInCostruzione[i] != NULL)
                    if(strcmp(squadreInCostruzione[i]->nomeSquadra,nomeSquadra)==0) //La squadra è ancora in costruzione
                        if(strcmp(squadreInCostruzione[i]->richiestePartecipazione[j],nomePlayer) == 0) //Il player si trova ancora tra le richieste quindi non è stata presa ancora una decisione
                            decisione = 0;
                        else{ //Il player non è più tra le richieste, se si trova tra i player accettati allora assegniamo 1, 2 altrimenti;

                            for(int z=0; z<4; z++){

                                if(strcmp(squadreInCostruzione[z],nomePlayer) == 0){

                                    decisione = 1;
                                    strcpy(msg_daInviare, "accettato\n");
                                    break;
                                }
                            }

                            if(decisione != 1){

                                decisione = 2;
                                strcpy(msg_daInviare, "rifiutato\n");
                            }
                        }
            }

            char tipoMessaggio[50];
            strcpy(tipoMessaggio,"RispostaDelCapitano\n");
            send(datiThread->clientSock,tipoMessaggio,strlen(tipoMessaggio),0);
            send(datiThread->clientSock,msg_daInviare,strlen(msg_daInviare),0);
            printf("Risposta decisione capitano inviato\n");
        }

    }

    pthread_mutex_unlock(&mutexListaSquadre);
    pthread_mutex_unlock(&mutexDecisioneCap);

    pthread_exit(NULL);

}

void gestione_decisioneCapitano(char *messaggio){

    int i;

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

    printf("Elaborazione decisione capitano\n");

    //Cerca la squadra nell'array globale
    pthread_mutex_lock(&mutexListaSquadre); //Mutua esclusione
    for(i=0; i<50; i++){

        if(squadreInCostruzione[i] != NULL){

            if(strcmp(squadreInCostruzione[i]->nomeSquadra,squadraString) == 0)
                break;
        }
    }


    if(squadreInCostruzione[i] != NULL){

        for(int j=0; j<50; j++){

            if(strcmp(squadreInCostruzione[i]->richiestePartecipazione[j],playerString) == 0){

                strcpy(squadreInCostruzione[i]->richiestePartecipazione[j],"null"); //A prescindere dalla decisione presa, rimuovi la richiesta di partecipazione
                break;
            }

        }
    }

    if(strcmp(decisioneString,"accettato") == 0){

        if(squadreInCostruzione[i] != NULL){

            for(int j=0; j<4; j++){

                if(strcmp(squadreInCostruzione[i]->players[j],"null") == 0){

                    strcpy(squadreInCostruzione[i]->players[j],playerString); //Il player passa dalla lista richieste della squadra alla lista dei player partecipanti
                    squadreInCostruzione[i]->numeroPlayers = squadreInCostruzione[i]->numeroPlayers++;
                    break;

                }

            }
        }
    }

    pthread_mutex_unlock(&mutexListaSquadre);
    pthread_cond_broadcast(&condSquadra);
    pthread_cond_broadcast(&condDecisioneCap);
}

void *send_aggiornamento_composizione_squadre(void* arg){

    printf("Nuovo thread (invio aggiornamento spogliatoio) creato\n");

    //Leggi argomento
    thread_data *datiThread = (thread_data*)arg;
    char *messaggio = malloc(strlen(datiThread->messaggio) +1);
    strcpy(messaggio,datiThread->messaggio);
    int client_socket = datiThread->clientSock;


    //Deserializzazione del messaggio
    struct json_object *parsed_json;
    parsed_json = json_tokener_parse(messaggio);

    json_object *nomeSquadra;
    json_object_object_get_ex(parsed_json, "squadra", &nomeSquadra);

    char *squadraString = malloc(strlen(json_object_get_string(nomeSquadra)) + 1);
    strcpy(squadraString,json_object_get_string(nomeSquadra));

    int aggiornamentoListe = 1;

    while(1){

        while(aggiornamentoListe == 0){

            pthread_mutex_lock(&mutexSquadra);
            pthread_cond_wait(&condSquadra,&mutexSquadra);
            printf("Thread spogliatoio squadra: %s svegliato\n", squadraString);
            aggiornamentoListe = 1;
        }

        pthread_mutex_lock(&mutexListaSquadre);

        // Creazione dei due array JSON di stringhe
        struct json_object *json_array_richieste = json_object_new_array();
        struct json_object *json_array_accettati = json_object_new_array();

        for(int i=0; i<50; i++){

            if(squadreInCostruzione[i]!= NULL){

                if(strcmp(squadreInCostruzione[i]->nomeSquadra,squadraString) == 0){

                    for(int j=0; j<50; j++){

                        if(strcmp(squadreInCostruzione[i]->richiestePartecipazione[j],"null") != 0){
                            char *richiesta;
                            int lunghezzaStringa = strlen(squadreInCostruzione[i]->richiestePartecipazione[j]);
                            richiesta = malloc(sizeof(lunghezzaStringa));
                            strcpy(richiesta,squadreInCostruzione[i]->richiestePartecipazione[j]);
                            json_object_array_add(json_array_richieste, json_object_new_string(richiesta));
                            printf("Richiesta player: %s aggiunta\n", richiesta);
                            break;
                        }
                    }

                    for(int j=0; j<4; j++){

                        if(strcmp(squadreInCostruzione[i]->players[j],"null") != 0){

                            char *player;
                            int lunghezzaStringa = strlen(squadreInCostruzione[i]->players[j]);
                            player = malloc(sizeof(lunghezzaStringa));
                            strcpy(player,squadreInCostruzione[i]->players[j]);
                            json_object_array_add(json_array_accettati, json_object_new_string(player));
                        }
                    }
                }
            }
        }

        // Creazione dell'oggetto principale che contiene i due array
        struct json_object *root = json_object_new_object();
        json_object_object_add(root, "richieste", json_array_richieste);
        json_object_object_add(root, "accettati", json_array_accettati);

        // Serializza l'oggetto JSON in una stringa
        const char *json_string = json_object_to_json_string(root);
        char delimitatore[] = "\n";
        strcat(json_string,delimitatore);

        // Invia la stringa JSON al client
        char tipoMessaggio[50];
        strcpy(tipoMessaggio,"AggiornamentoComposizioneSquadra\n");
        send(client_socket, tipoMessaggio, strlen(tipoMessaggio), 0);
        printf("TipoDiMessaggio: %s\n", tipoMessaggio);
        send(client_socket, json_string, strlen(json_string), 0);
        printf("Inviato aggiornamento composizione squadra: %s\n",json_string);

        //Libera mutex delle squadre
        pthread_mutex_unlock(&mutexListaSquadre);

        //Metti il thread in attesa
        aggiornamentoListe = 0;
    }
}

