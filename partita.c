#include "partita.h"
#include "variabiliGlobali.h"
#include "player.h"
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

static pthread_mutex_t mutexMsgEvento = PTHREAD_MUTEX_INITIALIZER;

char *serializza_oggetto_info_match(int indexPartita){

    // Creazione array JSON
    struct json_object *json_array_playersA = json_object_new_array();
    struct json_object *json_array_playersB = json_object_new_array();


    //Inizializzi array JSON
    for(int i=0; i<4; i++){

        char *playerA = partite[indexPartita]->squadra_A->players[i]->nomePlayer;
        char *playerB = partite[indexPartita]->squadra_B->players[i]->nomePlayer;
        json_object_array_add(json_array_playersA,json_object_new_string(playerA));
        json_object_array_add(json_array_playersB,json_object_new_string(playerB));

    }

    //Creazione stignhe JSON
    char *capitanoA = partite[indexPartita]->squadra_A->capitano->nomePlayer;
    char *capitanoB = partite[indexPartita]->squadra_B->capitano->nomePlayer;
    char *squadraA = partite[indexPartita]->squadra_A->nomeSquadra;
    char *squadraB = partite[indexPartita]->squadra_B->nomeSquadra;

    struct json_object *json_capitanoA = json_object_new_string(capitanoA);
    struct json_object *json_capitanoB = json_object_new_string(capitanoB);
    struct json_object *json_squadraA = json_object_new_string(squadraA);
    struct json_object *json_squadraB = json_object_new_string(squadraB);
    struct json_object *json_indexPartita = json_object_new_int(indexPartita);

    //Creazione dell'oggetto principale che contiene i due array e le stringhe
    struct json_object *root = json_object_new_object();
    json_object_object_add(root, "playersA", json_array_playersA);
    json_object_object_add(root, "playersB", json_array_playersB);
    json_object_object_add(root, "capitanoA", json_capitanoA);
    json_object_object_add(root, "capitanoB", json_capitanoB);
    json_object_object_add(root, "squadraA", json_squadraA);
    json_object_object_add(root, "squadraB", json_squadraB);
    json_object_object_add(root, "indexPartita", json_indexPartita);

    // Serializza l'oggetto JSON in una stringa
    char *json_string = json_object_to_json_string(root);
    char delimitatore[] = "\n";
    strcat(json_string,delimitatore);

    return json_string;
}

void avvisa_players_stato_match(int indexPartita, char *messaggio){


    //CASO: AVVIO PARTITA
    if(indexPartita > -1){

        //serializza info partita
        partita *partitaInAvvio = partite[indexPartita];
        char *jsonInfoMatch = serializza_oggetto_info_match(indexPartita);

        //Avverti capitano squadra A
        int clientCapitanoA = partitaInAvvio->squadra_A->capitano->socket;
        send(clientCapitanoA,"rispostaMatch\n", strlen("rispostaMatch\n"),0);
        send(clientCapitanoA, "avvioMatch\n", strlen("avvioMatch\n"),0);
        send(clientCapitanoA,jsonInfoMatch,strlen(jsonInfoMatch),0);

        printf("Avvisato il capitano %s della squadra %s dell'avvio del match\n",partitaInAvvio->squadra_A->capitano->nomePlayer,partitaInAvvio->squadra_A->nomeSquadra);;

        //Avverti capitano squadra B
        int clientCapitanoB = partitaInAvvio->squadra_B->capitano->socket;
        send(clientCapitanoB,"rispostaMatch\n", strlen("rispostaMatch\n"),0);
        send(clientCapitanoB, "avvioMatch\n", strlen("avvioMatch\n"),0);
        send(clientCapitanoB,jsonInfoMatch,strlen(jsonInfoMatch),0);

        printf("Avvisato il capitano %s della squadra %s dell'avvio del match\n",partitaInAvvio->squadra_B->capitano->nomePlayer,partitaInAvvio->squadra_B->nomeSquadra);

        //Avverti i player accettati
        for(int i=0; i<4; i++){

            int playerA = partitaInAvvio->squadra_A->players[i]->socket;
            int playerB = partitaInAvvio->squadra_B->players[i]->socket;


            send(playerA,"rispostaMatch\n", strlen("rispostaMatch\n"),0);
            send(playerA, "avvioMatch\n", strlen("avvioMatch\n"),0);
            send(playerA,jsonInfoMatch,strlen(jsonInfoMatch),0);

            printf("Avvisato il player %s della squadra %s dell'avvio del match\n",partitaInAvvio->squadra_A->players[i]->nomePlayer,partitaInAvvio->squadra_A->nomeSquadra);

            send(playerB,"rispostaMatch\n", strlen("rispostaMatch\n"),0);
            send(playerB, "avvioMatch\n", strlen("avvioMatch\n"),0);
            send(playerB,jsonInfoMatch,strlen(jsonInfoMatch),0);

            printf("Avvisato il player %s della squadra %s dell'avvio del match\n",partitaInAvvio->squadra_B->players[i]->nomePlayer,partitaInAvvio->squadra_B->nomeSquadra);

        }

        //Avvia processo match

    //CASO: ATTESA SQUADRA AVVERSARIA
    }else if(indexPartita == -1){

        //Deserializzazione del messaggio per estrarre la squdra che ha chiesto il match
        struct json_object *parsed_json;
        parsed_json = json_tokener_parse(messaggio);

        json_object *nomeSquadra;

        json_object_object_get_ex(parsed_json, "squadra", &nomeSquadra);

        char *squadraString = malloc(strlen(json_object_get_string(nomeSquadra)) + 1);
        strcpy(squadraString,json_object_get_string(nomeSquadra));


        //Cerca squadra nell'array delle squadre pronte
        int i;
        for(i=0; i<15;i++){

            if(squadreComplete[i] != NULL){

                if(strcmp(squadreComplete[i]->nomeSquadra,squadraString) == 0) break;
            }
        }

        if(i<15){

            //Avverti capitano
            int clientCapitano = squadreComplete[i]->capitano->socket;
            send(clientCapitano,"rispostaMatch\n", strlen("rispostaMatch\n"),0);
            send(clientCapitano, "attesaMatch\n", strlen("attesaMatch\n"),0);

            printf("Avvisato il capitano %s della squadra %s di in attesa di squadra avversaria\n",squadreComplete[i]->capitano->nomePlayer,squadreComplete[i]->nomeSquadra);

            //Avverti i player
            for(int j=0; j<4; j++){

                int clientPlayer = squadreComplete[i]->players[j]->socket;
                send(clientPlayer,"rispostaMatch\n", strlen("rispostaMatch\n"),0);
                send(clientPlayer, "attesaMatch\n", strlen("attesaMatch\n"),0);

                printf("Avvisato il player %s della squadra %s di in attesa di squadra avversaria\n",squadreComplete[i]->players[j]->nomePlayer,squadreComplete[i]->nomeSquadra);

            }
        }
    }
}

void *avviaTimer(void *arg){

    int indexPartita = *(int*) arg;

    partita *match = partite[indexPartita];

    for(int i=0; i<5; i++){

        sleep(60);

        pthread_mutex_lock(&mutexMsgEvento);

        switch(i){

            case 0:
                sendEventoPartecipantiMatch("refreshTime\n",indexPartita);
                sendEventoPartecipantiMatch("4m\n",indexPartita);
                break;

            case 1:
                sendEventoPartecipantiMatch("refreshTime\n",indexPartita);
                sendEventoPartecipantiMatch("3m\n",indexPartita);
                break;

            case 2:
                sendEventoPartecipantiMatch("refreshTime\n",indexPartita);
                sendEventoPartecipantiMatch("2m\n",indexPartita);
                break;

            case 3:
                sendEventoPartecipantiMatch("refreshTime\n",indexPartita);
                sendEventoPartecipantiMatch("1m\n",indexPartita);
                break;
        }

        pthread_mutex_unlock(&mutexMsgEvento);
    }


    match->finePartita = 1;
}

int getEvento(){

    int evento;

    //Numero tra 0 e 99
    int random_number = rand() % 100; // Numero tra 0 e 99

    //Simula una probabilità del 40% per l'evento tiro
    if(random_number < 40) evento = 0;

    //Simula una probabilità del 50% per l'evento dribbling
    else if(random_number < 90) evento = 1;

    //Simula una probabilità del 10% per l'evento infortunio
    else evento = 2;

    return evento;

}

int esitoTiro(){

    int esito;

    //Numero tra 0 e 99
    int random_number = rand() % 100; // Numero tra 0 e 99

    //Simula una probabilità del 40% di fare goal
    if(random_number < 40) esito = 1;

    //Simula una probabilità del 60% per tiro fallito
    else esito = 0;

    return esito;
}

int esitoDribbling(){

    int esito;

    //Numero tra 0 e 99
    int random_number = rand() % 100; // Numero tra 0 e 99

    //Simula una probabilità del 40% di successo
    if(random_number < 40) esito = 1;

    //Simula una probabilità del 30% per dribbling fallito
    else esito = 0;

    return esito;
}

void tira(char *player, int indexPartita, int *scoreA, int *scoreB){


    printf("%s tenta il tiro\n", player);
    sleep(1);

    partita *match = partite[indexPartita];
    int turnoSquadra = getSquadraFromPlayer(player,indexPartita);

    if(esitoTiro() == 0){

        printf("Tiro fallito\n");

        //Costruisci messaggio
        json_object *jobj = json_object_new_object();
        json_object_object_add(jobj, "tipoEvento", json_object_new_string("tiro"));
        json_object_object_add(jobj, "esitoTiro", json_object_new_string("fallito"));
        json_object_object_add(jobj, "turnoPlayer", json_object_new_string(player));
        json_object_object_add(jobj, "turnoSquadra", json_object_new_int(turnoSquadra));


        const char *json_str = json_object_to_json_string(jobj);
        char *messaggioJSON = strdup(json_str); // Copia la stringa JSON per restituirla
        json_object_put(jobj); // Dealloca l'oggetto JSON
        strcat(messaggioJSON,"\n");

        sendEventoPartecipantiMatch(messaggioJSON,indexPartita);

    }else{

            printf("Gooooooal!!\n");

            //Costruisci messaggio
            json_object *jobj = json_object_new_object();
            json_object_object_add(jobj, "tipoEvento", json_object_new_string("tiro"));
            json_object_object_add(jobj, "esitoTiro", json_object_new_string("goal"));
            json_object_object_add(jobj, "turnoPlayer", json_object_new_string(player));
            json_object_object_add(jobj, "turnoSquadra", json_object_new_int(turnoSquadra));


            const char *json_str = json_object_to_json_string(jobj);
            char *messaggioJSON = strdup(json_str); // Copia la stringa JSON per restituirla
            json_object_put(jobj); // Dealloca l'oggetto JSON
            strcat(messaggioJSON,"\n");

            sendEventoPartecipantiMatch(messaggioJSON,indexPartita);

            if(turnoSquadra == 0){

                printf("punto per la squadra %s\n",match->squadra_A->nomeSquadra);
                (*scoreA)++;

            }else{

                printf("punto per la squadra %s\n",match->squadra_B->nomeSquadra);
                (*scoreB)++;
            }

            printf("nuovo risultato: %d-%d\n",*scoreA,*scoreB);
            printf("\n");
            sleep(1);

        }
}

void dribbling(char *player,int indexPartita, int *scoreA, int *scoreB){

    printf("%s tenta il dribbling\n", player);
    sleep(1);

    int esito = esitoDribbling();

    if(esito == 0){

        printf("dribbling fallito, il possesso palla passa all'avversario");
        printf("\n");
        sleep(1);

        //Costruisci messaggio
        json_object *jobj = json_object_new_object();
        json_object_object_add(jobj, "tipoEvento", json_object_new_string("dribbling"));
        json_object_object_add(jobj, "esitoDribbling", json_object_new_string("fallito"));
        json_object_object_add(jobj, "turnoPlayer", json_object_new_string(player));

        const char *json_str = json_object_to_json_string(jobj);
        char *messaggioJSON = strdup(json_str); // Copia la stringa JSON per restituirla
        json_object_put(jobj); // Dealloca l'oggetto JSON
        strcat(messaggioJSON,"\n");

        sendEventoPartecipantiMatch(messaggioJSON,indexPartita);

    }else{

        printf("dribbling fantastisco,ha spazio per un tiro\n");
        sleep(1);

        //Costruisci messaggio
        json_object *jobj = json_object_new_object();
        json_object_object_add(jobj, "tipoEvento", json_object_new_string("dribbling"));
        json_object_object_add(jobj, "esitoDribbling", json_object_new_string("ok"));
        json_object_object_add(jobj, "turnoPlayer", json_object_new_string(player));

        const char *json_str = json_object_to_json_string(jobj);
        char *messaggioJSON = strdup(json_str); // Copia la stringa JSON per restituirla
        json_object_put(jobj); // Dealloca l'oggetto JSON
        strcat(messaggioJSON,"\n");

        sendEventoPartecipantiMatch(messaggioJSON,indexPartita);
        sleep(1);

        sendEventoPartecipantiMatch("evento\n",indexPartita);
        tira(player,indexPartita, scoreA, scoreB);
    }
}

void *penalizzazione(void *infoThread){

    //Estrazione parametri thread
    argomentiThreadPenalizzazione *arg = (argomentiThreadPenalizzazione*) infoThread;
    int indexPartita = arg->indexPartita;
    int time = arg->timeP;
    char *playerString = arg->player;
    partita *match = partite[indexPartita];
    player *playerP = NULL;

    sleep(time * 60);

    //Cerca player
    if(strcmp(playerString,match->squadra_A->capitano->nomePlayer) == 0){

        playerP = match->squadra_A->capitano;
        char playerRitornato[50];
        strcpy(playerRitornato,playerP->nomePlayer);
        strcat(playerRitornato,"\n");

        //Rendi player disponibile
        playerP->penalizzato = 0;
        printf("Il player %s ha scontato la penalizzazione\n",playerP->nomePlayer);

        //Invia messaggio ritorno player dalla penalizzazione
        pthread_mutex_lock(&mutexMsgEvento);
        sendEventoPartecipantiMatch("ritornoPenalizzazione\n",indexPartita);
        sendEventoPartecipantiMatch(playerRitornato,indexPartita);
        pthread_mutex_unlock(&mutexMsgEvento);
        return;
    }

    if(strcmp(playerString,match->squadra_B->capitano->nomePlayer) == 0){

        playerP = match->squadra_B->capitano;
        char playerRitornato[50];
        strcpy(playerRitornato,playerP->nomePlayer);
        strcat(playerRitornato,"\n");

        //Rendi player disponibile
        playerP->penalizzato = 0;
        printf("Il player %s ha scontato la penalizzazione\n",playerP->nomePlayer);

        //Invia messaggio ritorno player dalla penalizzazione
        pthread_mutex_lock(&mutexMsgEvento);
        sendEventoPartecipantiMatch("ritornoPenalizzazione\n",indexPartita);
        sendEventoPartecipantiMatch(playerRitornato,indexPartita);
        pthread_mutex_unlock(&mutexMsgEvento);
        return;
    }

    for(int i=0; i<4; i++){

        if(strcmp(playerString,match->squadra_A->players[i]->nomePlayer) == 0){

            playerP = match->squadra_A->players[i];
            char playerRitornato[50];
            strcpy(playerRitornato,playerP->nomePlayer);
            strcat(playerRitornato,"\n");

            //Rendi player disponibile
            playerP->penalizzato = 0;
            printf("Il player %s ha scontato la penalizzazione\n",playerP->nomePlayer);

            //Invia messaggio ritorno player dalla penalizzazione
            pthread_mutex_lock(&mutexMsgEvento);
            sendEventoPartecipantiMatch("ritornoPenalizzazione\n",indexPartita);
            sendEventoPartecipantiMatch(playerRitornato,indexPartita);
            pthread_mutex_unlock(&mutexMsgEvento);
            return;
        }

         if(strcmp(playerString,match->squadra_B->players[i]->nomePlayer) == 0){

            playerP = match->squadra_B->players[i];
            char playerRitornato[50];
            strcpy(playerRitornato,playerP->nomePlayer);
            strcat(playerRitornato,"\n");

            //Rendi player disponibile
            playerP->penalizzato = 0;
            printf("Il player %s ha scontato la penalizzazione\n",playerP->nomePlayer);

            //Invia messaggio ritorno player dalla penalizzazione
            pthread_mutex_lock(&mutexMsgEvento);
            sendEventoPartecipantiMatch("ritornoPenalizzazione\n",indexPartita);
            sendEventoPartecipantiMatch(playerRitornato,indexPartita);
            pthread_mutex_unlock(&mutexMsgEvento);
            return;
        }
    }

}

void *infortunio(void *infoThread){


    //Esrazione parametri thread
    argomentiThreadInfortunio *arg = (argomentiThreadInfortunio*) infoThread;
    int indexPartita = arg->indexPartita;
    char *playerString = arg->player;

    partita *match = partite[indexPartita];
    player *playerInfortunato = NULL;

    //Cerca player infortunato
    char *capitanoA = match->squadra_A->capitano->nomePlayer;
    char *capitanoB = match->squadra_B->capitano->nomePlayer;

    if(strcmp(capitanoA,playerString) == 0)
        playerInfortunato = match->squadra_A->capitano;

    if(playerInfortunato == NULL){

        if(strcmp(capitanoB,playerString) == 0)
            playerInfortunato = match->squadra_B->capitano;
    }

    if(playerInfortunato == NULL){

        for(int i=0; i<4; i++){

            char *playerA = match->squadra_A->players[i]->nomePlayer;
            char *playerB = match->squadra_B->players[i]->nomePlayer;

            if(strcmp(playerA,playerString) == 0){

                playerInfortunato = match->squadra_A->players[i];
                break;
            }

            if(strcmp(playerB,playerString) == 0){

                playerInfortunato = match->squadra_B->players[i];
                break;
            }
        }
    }


    int random_number = rand() % 4; // Numero tra 0 e 4 che rappresenzano i minuti di infortunio
    random_number++;
    int secondiInfortunio = random_number*60; //converti in secondi

    printf("%s ha avuto un infortunio, sarà indisponibile per %d minuti\n",playerString,random_number);

    //Penalizzazione giocatore avversario coinvolto
    int squadra = getSquadraFromPlayer(playerString,indexPartita); //Ottiene la squadra del giocatore infortunato per penalizzare la squadra avversaria
    int random_player = rand() % 5;
    int random_time_p = (rand() % 4) + 1;
    player *playerPenalizzato;
    if(squadra == 0){ //Penalliza squadra B

        if(random_player == 4){ //Penalliza il capitano della squadra 1

            playerPenalizzato = match->squadra_B->capitano;
            playerPenalizzato->penalizzato = 1;
            printf("Fallo commesso da %s penalizzato per %d min\n",playerPenalizzato->nomePlayer,random_time_p);
            //Crea thread penalizzazione
            pthread_t thread;
            argomentiThreadPenalizzazione *infoThread = malloc(sizeof(argomentiThreadPenalizzazione));
            infoThread->player = playerPenalizzato->nomePlayer;
            infoThread->indexPartita = indexPartita;
            infoThread->timeP = random_time_p;
            // Crea il thread e passa il puntatore alla struct come argomento
            if (pthread_create(&thread, NULL, penalizzazione, (void*)infoThread) != 0) {

                printf(stderr, "Errore nella creazione del thread infortunio\n");
            }

        }else{ //penalizza un player della squadra B

            playerPenalizzato = match->squadra_B->players[random_player];
            playerPenalizzato->penalizzato = 1;
            printf("Fallo commesso da %s penalizzato per %d min\n",playerPenalizzato->nomePlayer,random_time_p);

            //Crea thread penalizzazione
            pthread_t thread;
            argomentiThreadPenalizzazione *infoThread = malloc(sizeof(argomentiThreadPenalizzazione));
            infoThread->player = playerPenalizzato->nomePlayer;
            infoThread->indexPartita = indexPartita;
            infoThread->timeP = random_time_p;
            // Crea il thread e passa il puntatore alla struct come argomento
            if (pthread_create(&thread, NULL, penalizzazione, (void*)infoThread) != 0) {

                printf(stderr, "Errore nella creazione del thread infortunio\n");
            }
        }

    }else{ //Penaliza squadra A

        if(random_player == 4){ //Penalliza il capitano della squadra 1

            playerPenalizzato = match->squadra_A->capitano;
            playerPenalizzato->penalizzato = 1;
            printf("Fallo commesso da %s penalizzato per %d min\n",playerPenalizzato->nomePlayer,random_time_p);

            //Crea thread penalizzazione
            pthread_t thread;
            argomentiThreadPenalizzazione *infoThread = malloc(sizeof(argomentiThreadPenalizzazione));
            infoThread->player = playerPenalizzato->nomePlayer;
            infoThread->indexPartita = indexPartita;
            infoThread->timeP = random_time_p;
            // Crea il thread e passa il puntatore alla struct come argomento
            if (pthread_create(&thread, NULL, penalizzazione, (void*)infoThread) != 0) {

                printf(stderr, "Errore nella creazione del thread infortunio\n");
            }

        }else{ //penalizza un player della squadra B

            playerPenalizzato = match->squadra_A->players[random_player];
            playerPenalizzato->penalizzato = 1;
            printf("Fallo commesso da %s penalizzato per %d min\n",playerPenalizzato->nomePlayer,random_time_p);

            //Crea thread penalizzazione
            pthread_t thread;
            argomentiThreadPenalizzazione *infoThread = malloc(sizeof(argomentiThreadPenalizzazione));
            infoThread->player = playerPenalizzato->nomePlayer;
            infoThread->indexPartita = indexPartita;
            infoThread->timeP = random_time_p;
            // Crea il thread e passa il puntatore alla struct come argomento
            if (pthread_create(&thread, NULL, penalizzazione, (void*)infoThread) != 0) {

                printf(stderr, "Errore nella creazione del thread infortunio\n");
            }
        }
    }

    //Rendi indisponibile il player
    playerInfortunato->infortunato = 1;

    //Costruisci messaggio
    json_object *jobj = json_object_new_object();
    json_object_object_add(jobj, "tipoEvento", json_object_new_string("infortunio"));
    json_object_object_add(jobj, "turnoPlayer", json_object_new_string(playerString));
    json_object_object_add(jobj, "minuti", json_object_new_int(random_number));
    json_object_object_add(jobj, "playerPenalizzato", json_object_new_string(playerPenalizzato->nomePlayer));
    json_object_object_add(jobj, "minutiP", json_object_new_int(random_time_p));


    const char *json_str = json_object_to_json_string(jobj);
    char *messaggioJSON = strdup(json_str); // Copia la stringa JSON per restituirla
    json_object_put(jobj); // Dealloca l'oggetto JSON
    strcat(messaggioJSON,"\n");

    sendEventoPartecipantiMatch(messaggioJSON,indexPartita);
    pthread_mutex_unlock(&mutexMsgEvento);

    sleep(secondiInfortunio);

    //Ritorno del player
    playerInfortunato->infortunato = 0;

    printf("%s è ritornato dall'infortunio\n",playerString);

    //Costruisci messaggio
    json_object *jobj2 = json_object_new_object();
    json_object_object_add(jobj2, "tipoEvento", json_object_new_string("ritornoInfortunio"));
    json_object_object_add(jobj2, "turnoPlayer", json_object_new_string(playerString));

    const char *json_str2 = json_object_to_json_string(jobj2);
    char *messaggioJSON2 = strdup(json_str2); // Copia la stringa JSON per restituirla
    json_object_put(jobj2); // Dealloca l'oggetto JSON
    strcat(messaggioJSON2,"\n");

    pthread_mutex_lock(&mutexMsgEvento);
    sendEventoPartecipantiMatch("evento\n",indexPartita);
    sendEventoPartecipantiMatch(messaggioJSON2,indexPartita);
    pthread_mutex_unlock(&mutexMsgEvento);
}

int getSquadraFromPlayer(char *player, int indexPartita){

    partita *match = partite[indexPartita];

    if(strcmp(player,match->squadra_A->capitano->nomePlayer) == 0) return 0;

    if(strcmp(player,match->squadra_B->capitano->nomePlayer) == 0) return 1;

    for(int i=0; i<4; i++){

        if(strcmp(player,match->squadra_A->players[i]->nomePlayer) == 0) return 0;

        if(strcmp(player,match->squadra_B->players[i]->nomePlayer) == 0) return 1;
    }

    return -1;
}

int getIndexPlayer(char *player, int indexPartita){

    partita *match = partite[indexPartita];

    if(strcmp(player,match->squadra_A->capitano->nomePlayer) == 0) return 4;

    if(strcmp(player,match->squadra_B->capitano->nomePlayer) == 0) return 4;

    for(int i=0; i<4; i++){

        if(strcmp(player,match->squadra_A->players[i]->nomePlayer) == 0) return i;

        if(strcmp(player,match->squadra_B->players[i]->nomePlayer) == 0) return i;
    }

    return -1;
}

void sendEventoPartecipantiMatch(char *messaggio, int indexPartita){

    partita *match = partite[indexPartita];

    int sockCapitanoA = match->squadra_A->capitano->socket;
    int sockCapitanoB = match->squadra_B->capitano->socket;

    send(sockCapitanoA,messaggio,strlen(messaggio),0);
    send(sockCapitanoB,messaggio,strlen(messaggio),0);

    for(int i=0; i<4; i++){

        int sockPlayerA = match->squadra_A->players[i]->socket;
        int sockPlayerB = match->squadra_B->players[i]->socket;

        send(sockPlayerA,messaggio,strlen(messaggio),0);
        send(sockPlayerB,messaggio,strlen(messaggio),0);
    }
}

char *assegna_turno(int turnoSquadraAttuale, int indexPartita, int *indiceTurnoA,int *indiceTurnoB){

    partita *match = partite[indexPartita];

    //-----------Assegnare il turno alla squadra B poiché il turno attuale è della squadra A---------------------

    if(turnoSquadraAttuale == 0){

        (*indiceTurnoB)++;
        int indiceTurnoModulato = (*indiceTurnoB)%5;
        char *turnoPlayer;
        char *turnoPlayerReturn;
        player *playerInfo;

        while(1){

            //Assegna turno al capitano se non è infortunato
            if(indiceTurnoModulato == 4){

                playerInfo = match->squadra_B->capitano;

                if(playerInfo->infortunato != 1 && playerInfo->penalizzato != 1){

                    turnoPlayer = malloc(strlen(playerInfo->nomePlayer)+1);
                    turnoPlayerReturn = malloc(strlen(playerInfo->nomePlayer)+1);
                    strcpy(turnoPlayer,playerInfo->nomePlayer);
                    strcpy(turnoPlayerReturn,playerInfo->nomePlayer);

                    strcat(turnoPlayer,"\n");

                    sendEventoPartecipantiMatch("assegnazioneTurno\n",indexPartita);
                    sendEventoPartecipantiMatch(turnoPlayer,indexPartita);

                    return turnoPlayerReturn;
                }

            }else{

                playerInfo = match->squadra_B->players[indiceTurnoModulato];

                if(playerInfo->infortunato != 1 && playerInfo->penalizzato != 1){

                    turnoPlayer = malloc(strlen(playerInfo->nomePlayer)+1);
                    turnoPlayerReturn = malloc(strlen(playerInfo->nomePlayer)+1);
                    strcpy(turnoPlayer,playerInfo->nomePlayer);
                    strcpy(turnoPlayerReturn,playerInfo->nomePlayer);

                    strcat(turnoPlayer,"\n");

                    sendEventoPartecipantiMatch("assegnazioneTurno\n",indexPartita);
                    sendEventoPartecipantiMatch(turnoPlayer,indexPartita);

                    return turnoPlayerReturn;
                }
            }

            //Se arriva qui non è stato assegnato il turno quindi fa una nuova iterazione con il turno successivo
            *indiceTurnoB++;
            indiceTurnoModulato = (*indiceTurnoB)%5;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////

   //--------------------------------ASSENAZIONE TURNO PLYER SQUADRA A---------------------------------------------------

   *indiceTurnoA++;
    int indiceTurnoModulato = (*indiceTurnoA)%5;
    char *turnoPlayer;
    char *turnoPlayerReturn;
    player *playerInfo;

    while(1){

        //Assegna turno al capitano se non è infortunato
        if(indiceTurnoModulato == 4){

            playerInfo = match->squadra_A->capitano;

            if(playerInfo->infortunato != 1 && playerInfo->penalizzato != 1){

                turnoPlayer = malloc(strlen(playerInfo->nomePlayer)+1);
                turnoPlayerReturn = malloc(strlen(playerInfo->nomePlayer)+1);
                strcpy(turnoPlayer,playerInfo->nomePlayer);
                strcpy(turnoPlayerReturn,playerInfo->nomePlayer);

                strcat(turnoPlayer,"\n");

                sendEventoPartecipantiMatch("assegnazioneTurno\n",indexPartita);
                sendEventoPartecipantiMatch(turnoPlayer,indexPartita);

                return turnoPlayerReturn;
            }

        }else{

            playerInfo = match->squadra_A->players[indiceTurnoModulato];

            if(playerInfo->infortunato != 1 && playerInfo->penalizzato != 1){

                turnoPlayer = malloc(strlen(playerInfo->nomePlayer)+1);
                turnoPlayerReturn = malloc(strlen(playerInfo->nomePlayer)+1);
                strcpy(turnoPlayer,playerInfo->nomePlayer);
                strcpy(turnoPlayerReturn,playerInfo->nomePlayer);

                strcat(turnoPlayer,"\n");

                sendEventoPartecipantiMatch("assegnazioneTurno\n",indexPartita);
                sendEventoPartecipantiMatch(turnoPlayer,indexPartita);

                return turnoPlayerReturn;
            }
        }

        //Se arriva qui non è stato assegnato il turno quindi fa una nuova iterazione con il turno successivo
        *indiceTurnoB++;
        indiceTurnoModulato = (*indiceTurnoB)%5;
    }
}

void simulaMatch(int indexPartita){

    partita *match = partite[indexPartita];

    //-----------------------------Cerca il player che inizia il turno-----------------------------------------
    int playerTrovato = 0;
    char *playerInizioTurnoString = match->inizioTurno;
    player *playerInizioTurno;

    if(strcmp(playerInizioTurnoString,match->squadra_A->capitano->nomePlayer) == 0){

        playerInizioTurno = match->squadra_A->capitano;
        playerTrovato = 1;
    }

    if(playerTrovato != 1){

        if(strcmp(playerInizioTurnoString,match->squadra_B->capitano->nomePlayer) == 0){

            playerInizioTurno = match->squadra_B->capitano;
            playerTrovato = 1;
        }

    }

    if(playerTrovato != 1){

        for(int i=0; i<4; i++){

            if(strcmp(playerInizioTurno,match->squadra_A->players[i]->nomePlayer) == 0){

                playerInizioTurno = match->squadra_A->players[i];
                break;
            }

            if(strcmp(playerInizioTurno,match->squadra_B->players[i]->nomePlayer) == 0){

                playerInizioTurno = match->squadra_B->players[i];
                break;
            }
        }
    }

    //--------------------------------Inizia simulazione eventi---------------------------------------------

    //Variabili Match
    int fineMatch = 0;
    int scoreA = 0;
    int scoreB = 0;
    int evento;
    int turnoSquadra = getSquadraFromPlayer(playerInizioTurnoString,indexPartita);
    int indiceTurnoA;
    int indiceTurnoB;

    if(turnoSquadra == 0){

        indiceTurnoB = 4;
        indiceTurnoA = getIndexPlayer(playerInizioTurno,indexPartita);

    }else{

        indiceTurnoA = 4;
        indiceTurnoB = getIndexPlayer(playerInizioTurno,indexPartita);
    }

    //Gestione timer
    pthread_t thread;
    int *indexPartitaThreadTime = malloc(sizeof(int));
    *indexPartitaThreadTime = indexPartita;

    //Avvia timer
    if (pthread_create(&thread, NULL, avviaTimer, (void*)indexPartitaThreadTime) != 0) {

        printf(stderr, "Errore nella creazione del thread time\n");
    }

    printf("Match iniziato, 5 minuti per la fine\n");
    printf("Possesso palla di %s\n", playerInizioTurnoString);

    evento = getEvento();

    pthread_mutex_lock(&mutexMsgEvento);
    sendEventoPartecipantiMatch("evento\n",indexPartita);

    if(evento == 0){

        tira(playerInizioTurnoString,indexPartita,&scoreA,&scoreB);
        pthread_mutex_unlock(&mutexMsgEvento);

    }else if(evento == 1){

        dribbling(playerInizioTurnoString,indexPartita,&scoreA,&scoreB);
        pthread_mutex_unlock(&mutexMsgEvento);

    }else{

        pthread_t thread;
        argomentiThreadInfortunio *infoThread = malloc(sizeof(argomentiThreadInfortunio));
        infoThread->player = playerInizioTurnoString;
        infoThread->indexPartita = indexPartita;


        // Crea il thread e passa il puntatore alla struct come argomento
        if (pthread_create(&thread, NULL, infortunio, (void*)infoThread) != 0) {

            printf(stderr, "Errore nella creazione del thread infortunio\n");
        }

    }

    sleep(10);

    while(match->finePartita != 1){

        char *turnoPlayer;
        turnoPlayer = assegna_turno(turnoSquadra,indexPartita,&indiceTurnoA,&indiceTurnoB);

        if(turnoSquadra == 0) turnoSquadra = 1;
        else turnoSquadra = 0;

        printf("%s ottiene il possesso palla\n",turnoPlayer);

        evento = getEvento();

        pthread_mutex_lock(&mutexMsgEvento);
        sendEventoPartecipantiMatch("evento\n",indexPartita);

        if(evento == 0){

            tira(turnoPlayer,indexPartita,&scoreA,&scoreB);
            pthread_mutex_unlock(&mutexMsgEvento);

        }else if(evento == 1){

            dribbling(turnoPlayer,indexPartita,&scoreA,&scoreB);
            pthread_mutex_unlock(&mutexMsgEvento);

        }else{

            pthread_t thread;
            argomentiThreadInfortunio *infoThread = malloc(sizeof(argomentiThreadInfortunio));
            infoThread->player = turnoPlayer;
            infoThread->indexPartita = indexPartita;


            // Crea il thread e passa il puntatore alla struct come argomento
            if (pthread_create(&thread, NULL, infortunio, (void*)infoThread) != 0) {

                printf(stderr, "Errore nella creazione del thread infortunio\n");
            }
        }

        sleep(10);
    }

    printf("\n tempo scaduto, match concluso\n");

    //Avvertiti i players della fine del match
    sendEventoPartecipantiMatch("fineMatch\n",indexPartita);
}

void assegna_turno_iniziale_e_avvia_match(char *messaggio){

    //Deserializzazione del messaggio
    struct json_object *parsed_json;
    parsed_json = json_tokener_parse(messaggio);

    json_object *indexPartitaJSON;
    json_object *playerInizioTurnoJSON;
    json_object *squadraInizioTurnoJSON;

    json_object_object_get_ex(parsed_json, "indexPartita", &indexPartitaJSON);
    json_object_object_get_ex(parsed_json, "player", &playerInizioTurnoJSON);
    json_object_object_get_ex(parsed_json, "squadra", &squadraInizioTurnoJSON);

    int indexPartita = json_object_get_int(indexPartita);
    char playerInizioTurno[30];
    strcpy(playerInizioTurno,json_object_get_string(playerInizioTurnoJSON));
    char squadraInizioTurno[30];
    strcpy(squadraInizioTurno,json_object_get_string(squadraInizioTurnoJSON));
    strcat(squadraInizioTurno,"\n");

    //Assegna inizio turno al player e avvia la simulazione
    if(strcmp(partite[indexPartita]->inizioTurno,"null") == 0){

        strcpy(partite[indexPartita]->inizioTurno,playerInizioTurno);
        strcat(playerInizioTurno,"\n");

        partita *nuovaPartita = partite[indexPartita];

        for(int i=0; i<4; i++){

            player *playerA = nuovaPartita->squadra_A->players[i];
            player *playerB = nuovaPartita->squadra_B->players[i];

            send(playerA->socket,"inizioMatch\n",strlen("inizioMatch\n"),0);
            send(playerA->socket,playerInizioTurno,strlen(playerInizioTurno),0);
            printf("Avvertito il player %s della squadra %s che il player %s della squadra %s inizia il match\n",playerA->nomePlayer,nuovaPartita->squadra_A->nomeSquadra,playerInizioTurno,squadraInizioTurno);

            send(playerB->socket,"inizioMatch\n",strlen("inizioMatch\n"),0);
            send(playerB->socket,playerInizioTurno,strlen(playerInizioTurno),0);
            printf("Avvertito il player %s della squadra %s che il player %s della squadra %s inizia il match\n",playerB->nomePlayer,nuovaPartita->squadra_B->nomeSquadra,playerInizioTurno,squadraInizioTurno);
        }

        player *capitanoA = nuovaPartita->squadra_A->capitano;
        player *capitanoB = nuovaPartita->squadra_B->capitano;

        send(capitanoA->socket,"inizioMatch\n",strlen("inizioMatch\n"),0);
        send(capitanoA->socket,playerInizioTurno,strlen(playerInizioTurno),0);
        printf("Avvertito il player %s della squadra %s che il player %s della squadra %s inizia il match\n",capitanoA->nomePlayer, nuovaPartita->squadra_A->nomeSquadra,playerInizioTurno,squadraInizioTurno);

        send(capitanoB->socket,"inizioMatch\n",strlen("inizioMatch\n"),0);
        send(capitanoB->socket,playerInizioTurno,strlen(playerInizioTurno),0);
        printf("Avvertito il player %s della squadra %s che il player %s della squadra %s inizia il match\n",capitanoB->nomePlayer,nuovaPartita->squadra_B->nomeSquadra,playerInizioTurno,squadraInizioTurno);

        simulaMatch(indexPartita);
    }

}

int get_index_partita(char *messaggio){

    //Deserializzazione del messaggio
    struct json_object *parsed_json;
    parsed_json = json_tokener_parse(messaggio);

    json_object *indexPartitaJSON;

    json_object_object_get_ex(parsed_json, "indexPartita", &indexPartitaJSON);

    int indexPartita = json_object_get_int(indexPartita);

    return indexPartita;
}
