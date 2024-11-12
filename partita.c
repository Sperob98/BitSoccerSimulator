#include "partita.h"
#include "variabiliGlobali.h"
#include "player.h"
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

char *serializza_oggetto_info_match(int indexPartita){

    // Creazione array JSON
    struct json_object *json_array_playersA = json_object_new_array();
    struct json_object *json_array_playersB = json_object_new_array();


    //Inizializzi array JSON
    for(int i=0; i<SIZE_ARRAY_PLAYER_PARTECIPANTI; i++){

        char *playerA = partite[indexPartita]->squadra_A->players[i]->nome_player;
        char *playerB = partite[indexPartita]->squadra_B->players[i]->nome_player;
        json_object_array_add(json_array_playersA,json_object_new_string(playerA));
        json_object_array_add(json_array_playersB,json_object_new_string(playerB));

    }

    //Creazione stignhe JSON
    char *capitanoA = partite[indexPartita]->squadra_A->capitano->nome_player;
    char *capitanoB = partite[indexPartita]->squadra_B->capitano->nome_player;
    char *squadraA = partite[indexPartita]->squadra_A->nome_squadra;
    char *squadraB = partite[indexPartita]->squadra_B->nome_squadra;

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

        printf("Avvisato il capitano %s della squadra %s dell'avvio del match\n",partitaInAvvio->squadra_A->capitano->nome_player,partitaInAvvio->squadra_A->nome_squadra);

        //Avverti capitano squadra B
        int clientCapitanoB = partitaInAvvio->squadra_B->capitano->socket;
        send(clientCapitanoB,"rispostaMatch\n", strlen("rispostaMatch\n"),0);
        send(clientCapitanoB, "avvioMatch\n", strlen("avvioMatch\n"),0);
        send(clientCapitanoB,jsonInfoMatch,strlen(jsonInfoMatch),0);

        printf("Avvisato il capitano %s della squadra %s dell'avvio del match\n",partitaInAvvio->squadra_B->capitano->nome_player,partitaInAvvio->squadra_B->nome_squadra);

        //Avverti i player accettati
        for(int i=0; i<SIZE_ARRAY_PLAYER_PARTECIPANTI; i++){

            int playerA = partitaInAvvio->squadra_A->players[i]->socket;
            int playerB = partitaInAvvio->squadra_B->players[i]->socket;


            send(playerA,"rispostaMatch\n", strlen("rispostaMatch\n"),0);
            send(playerA, "avvioMatch\n", strlen("avvioMatch\n"),0);
            send(playerA,jsonInfoMatch,strlen(jsonInfoMatch),0);

            printf("Avvisato il player %s della squadra %s dell'avvio del match\n",partitaInAvvio->squadra_A->players[i]->nome_player,partitaInAvvio->squadra_A->nome_squadra);

            send(playerB,"rispostaMatch\n", strlen("rispostaMatch\n"),0);
            send(playerB, "avvioMatch\n", strlen("avvioMatch\n"),0);
            send(playerB,jsonInfoMatch,strlen(jsonInfoMatch),0);

            printf("Avvisato il player %s della squadra %s dell'avvio del match\n",partitaInAvvio->squadra_B->players[i]->nome_player,partitaInAvvio->squadra_B->nome_squadra);

        }

        //Avvia processo match

    //CASO: ATTESA SQUADRA AVVERSARIA
    }else if(indexPartita == -1){

        //Deserializzazione del messaggio per estrarre la squdra che ha chiesto il match
        struct json_object *parsed_json;
        parsed_json = json_tokener_parse(messaggio);

        json_object *nomeSquadra;

        json_object_object_get_ex(parsed_json, "squadra", &nomeSquadra);

        char squadraString[SIZE_NAME_TEAM];
        strcpy(squadraString,json_object_get_string(nomeSquadra));


        //Cerca squadra nell'array delle squadre pronte
        int i;
        for(i=0; i<15;i++){

            if(squadreComplete[i] != NULL){

                if(strcmp(squadreComplete[i]->nome_squadra,squadraString) == 0) break;
            }
        }

        if(i<15){

            //Avverti capitano
            int clientCapitano = squadreComplete[i]->capitano->socket;
            send(clientCapitano,"rispostaMatch\n", strlen("rispostaMatch\n"),0);
            send(clientCapitano, "attesaMatch\n", strlen("attesaMatch\n"),0);

            printf("Avvisato il capitano %s della squadra %s di in attesa di squadra avversaria\n",squadreComplete[i]->capitano->nome_player,squadreComplete[i]->nome_squadra);

            //Avverti i player
            for(int j=0; j<SIZE_ARRAY_PLAYER_PARTECIPANTI; j++){

                int clientPlayer = squadreComplete[i]->players[j]->socket;
                send(clientPlayer,"rispostaMatch\n", strlen("rispostaMatch\n"),0);
                send(clientPlayer, "attesaMatch\n", strlen("attesaMatch\n"),0);

                printf("Avvisato il player %s della squadra %s di in attesa di squadra avversaria\n",squadreComplete[i]->players[j]->nome_player,squadreComplete[i]->nome_squadra);

            }
        }
    }
}

void *avviaTimer(void *arg){

    int indexPartita = *(int*) arg;

    partita *match = partite[indexPartita];

    json_object *jobj;

    char *messaggioJSON;

    for(int i=0; i<5; i++){

        sleep(60);

        switch(i){

            case 0:

                //Costruisci messaggio e invia messaggio ai partecipanti
                jobj = json_object_new_object();
                json_object_object_add(jobj, "tipoEvento", json_object_new_string("refreshTime"));
                json_object_object_add(jobj, "countDown", json_object_new_string("4m"));
                const char *json_str = json_object_to_json_string(jobj);

                messaggioJSON = strdup(json_str); // Copia la stringa JSON per restituirla
                json_object_put(jobj); // Dealloca l'oggetto JSON
                strcat(messaggioJSON,"\n");

                sendEventoPartecipantiMatch(messaggioJSON,indexPartita);

                break;

            case 1:

                //Costruisci messaggio e invia messaggio ai partecipanti
                jobj = json_object_new_object();
                json_object_object_add(jobj, "tipoEvento", json_object_new_string("refreshTime"));
                json_object_object_add(jobj, "countDown", json_object_new_string("3m"));
                const char *json_str1 = json_object_to_json_string(jobj);

                messaggioJSON = strdup(json_str1); // Copia la stringa JSON per restituirla
                json_object_put(jobj); // Dealloca l'oggetto JSON
                strcat(messaggioJSON,"\n");

                sendEventoPartecipantiMatch(messaggioJSON,indexPartita);

                break;

            case 2:

                //Costruisci messaggio e invia messaggio ai partecipanti
                jobj = json_object_new_object();
                json_object_object_add(jobj, "tipoEvento", json_object_new_string("refreshTime"));
                json_object_object_add(jobj, "countDown", json_object_new_string("2m"));
                const char *json_str2 = json_object_to_json_string(jobj);

                messaggioJSON = strdup(json_str2); // Copia la stringa JSON per restituirla
                json_object_put(jobj); // Dealloca l'oggetto JSON
                strcat(messaggioJSON,"\n");

                sendEventoPartecipantiMatch(messaggioJSON,indexPartita);

                break;

            case 3:

                //Costruisci messaggio e invia messaggio ai partecipanti
                jobj = json_object_new_object();
                json_object_object_add(jobj, "tipoEvento", json_object_new_string("refreshTime"));
                json_object_object_add(jobj, "countDown", json_object_new_string("1m"));
                const char *json_str3 = json_object_to_json_string(jobj);

                messaggioJSON = strdup(json_str3); // Copia la stringa JSON per restituirla
                json_object_put(jobj); // Dealloca l'oggetto JSON
                strcat(messaggioJSON,"\n");

                sendEventoPartecipantiMatch(messaggioJSON,indexPartita);

                break;
        }

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

                printf("punto per la squadra %s\n",match->squadra_A->nome_squadra);
                (*scoreA)++;

            }else{

                printf("punto per la squadra %s\n",match->squadra_B->nome_squadra);
                (*scoreB)++;
            }

            printf("nuovo risultato: %d-%d\n",*scoreA,*scoreB);
            printf("\n");

        }
}

void dribbling(char *player,int indexPartita, int *scoreA, int *scoreB){

    printf("%s tenta il dribbling\n", player);

    int esito = esitoDribbling();

    if(esito == 0){

        printf("dribbling fallito, il possesso palla passa all'avversario");
        printf("\n");

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

        tira(player,indexPartita, scoreA, scoreB);
    }
}

void *penalizzazione(void *infoThread){

    //Estrazione parametri thread
    argomentiThreadPenalizzazione *arg = (argomentiThreadPenalizzazione*) infoThread;
    int indexPartita = arg->indexPartita;
    int time = arg->timeP;
    char *playerString = malloc(SIZE_NAME_PLAYER * sizeof(char));
    strcpy(playerString,arg->player);
    partita *match = partite[indexPartita];
    player *playerP = NULL;

    printf("Test thread penalizzazione player penalizzato: %s\n",playerString);

    sleep(time * 60);

    //Cerca player
    if(strcmp(playerString,match->squadra_A->capitano->nome_player) == 0){

        playerP = match->squadra_A->capitano;

        //Rendi player disponibile
        pthread_mutex_lock(&mutexPartite);
        playerP->penalizzato = 0;
        printf("Il player %s ha scontato la penalizzazione\n",playerP->nome_player);
        pthread_mutex_unlock(&mutexPartite);

        //Costruisci messaggio e invia messaggio ai partecipanti
        json_object *jobj = json_object_new_object();
        json_object_object_add(jobj, "tipoEvento", json_object_new_string("ritornoPenalizzazione"));
        json_object_object_add(jobj, "playerRitornato", json_object_new_string(playerString));
        const char *json_str = json_object_to_json_string(jobj);

        char *messaggioJSON = strdup(json_str); // Copia la stringa JSON per restituirla
        json_object_put(jobj); // Dealloca l'oggetto JSON
        strcat(messaggioJSON,"\n");

        sendEventoPartecipantiMatch(messaggioJSON,indexPartita);

        return;
    }

    if(strcmp(playerString,match->squadra_B->capitano->nome_player) == 0){

        playerP = match->squadra_B->capitano;

        //Rendi player disponibile
        playerP->penalizzato = 0;
        printf("Il player %s ha scontato la penalizzazione\n",playerP->nome_player);

        //Invia messaggio ritorno player dalla penalizzazione
        //Costruisci messaggio e invia messaggio ai partecipanti
        json_object *jobj = json_object_new_object();
        json_object_object_add(jobj, "tipoEvento", json_object_new_string("ritornoPenalizzazione"));
        json_object_object_add(jobj, "playerRitornato", json_object_new_string(playerString));
        const char *json_str = json_object_to_json_string(jobj);

        char *messaggioJSON = strdup(json_str); // Copia la stringa JSON per restituirla
        json_object_put(jobj); // Dealloca l'oggetto JSON
        strcat(messaggioJSON,"\n");

        sendEventoPartecipantiMatch(messaggioJSON,indexPartita);


        return;
    }

    for(int i=0; i<SIZE_ARRAY_PLAYER_PARTECIPANTI; i++){

        if(strcmp(playerString,match->squadra_A->players[i]->nome_player) == 0){

            playerP = match->squadra_A->players[i];

            //Rendi player disponibile
            playerP->penalizzato = 0;
            printf("Il player %s ha scontato la penalizzazione\n",playerP->nome_player);

            //Invia messaggio ritorno player dalla penalizzazione

            //Costruisci messaggio e invia messaggio ai partecipanti
            json_object *jobj = json_object_new_object();
            json_object_object_add(jobj, "tipoEvento", json_object_new_string("ritornoPenalizzazione"));
            json_object_object_add(jobj, "playerRitornato", json_object_new_string(playerString));
            const char *json_str = json_object_to_json_string(jobj);

            char *messaggioJSON = strdup(json_str); // Copia la stringa JSON per restituirla
            json_object_put(jobj); // Dealloca l'oggetto JSON
            strcat(messaggioJSON,"\n");

            sendEventoPartecipantiMatch(messaggioJSON,indexPartita);

            return;
        }

         if(strcmp(playerString,match->squadra_B->players[i]->nome_player) == 0){

            playerP = match->squadra_B->players[i];

            //Rendi player disponibile
            playerP->penalizzato = 0;
            printf("Il player %s ha scontato la penalizzazione\n",playerP->nome_player);

            //Invia messaggio ritorno player dalla penalizzazione
            //Costruisci messaggio e invia messaggio ai partecipanti
            json_object *jobj = json_object_new_object();
            json_object_object_add(jobj, "tipoEvento", json_object_new_string("ritornoPenalizzazione"));
            json_object_object_add(jobj, "playerRitornato", json_object_new_string(playerString));
            const char *json_str = json_object_to_json_string(jobj);

            char *messaggioJSON = strdup(json_str); // Copia la stringa JSON per restituirla
            json_object_put(jobj); // Dealloca l'oggetto JSON
            strcat(messaggioJSON,"\n");

            sendEventoPartecipantiMatch(messaggioJSON,indexPartita);

            return;
        }
    }

}

void *infortunio(void *infoThread){

    //Esrazione parametri thread
    argomentiThreadInfortunio *arg = (argomentiThreadInfortunio*) infoThread;
    int indexPartita = arg->indexPartita;
    char *playerString = malloc(SIZE_NAME_PLAYER * sizeof(char));
    strcpy(playerString,arg->player_name);

    partita *match = partite[indexPartita];
    player *playerInfortunato = NULL;

    printf("Test Thread infotunio: player infortunato %s\n",playerString);

    //Cerca player infortunato
    char *capitanoA = match->squadra_A->capitano->nome_player;
    char *capitanoB = match->squadra_B->capitano->nome_player;

    if(strcmp(capitanoA,playerString) == 0)
        playerInfortunato = match->squadra_A->capitano;

    if(playerInfortunato == NULL){

        if(strcmp(capitanoB,playerString) == 0)
            playerInfortunato = match->squadra_B->capitano;
    }

    if(playerInfortunato == NULL){

        for(int i=0; i<SIZE_ARRAY_PLAYER_PARTECIPANTI; i++){

            char *playerA = match->squadra_A->players[i]->nome_player;
            char *playerB = match->squadra_B->players[i]->nome_player;

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
    int random_player;
    int random_time_p = (rand() % 4) + 1;
    player *playerPenalizzato;
    int trovato_player_da_penalizzare = 0;

    while(trovato_player_da_penalizzare != 1){

        random_player = rand() % 5;
        if(squadra == 0){ //Penalliza squadra B

            if(random_player == 4){ //Penalliza il capitano della squadra 1

                playerPenalizzato = match->squadra_B->capitano;
                pthread_mutex_lock(&mutexPartite);
                if(playerPenalizzato->infortunato != 1 && playerPenalizzato->penalizzato != 1){

                    playerPenalizzato->penalizzato = 1;
                    printf("Fallo commesso da %s penalizzato per %d min\n",playerPenalizzato->nome_player,random_time_p);
                    pthread_mutex_unlock(&mutexPartite);
                    //Crea thread penalizzazione
                    pthread_t thread;
                    argomentiThreadPenalizzazione *infoThreadPenalizzazione = malloc(sizeof(argomentiThreadPenalizzazione));
                    strcpy(infoThreadPenalizzazione->player,playerPenalizzato->nome_player);
                    infoThreadPenalizzazione->indexPartita = indexPartita;
                    infoThreadPenalizzazione->timeP = random_time_p;
                    // Crea il thread e passa il puntatore alla struct come argomento
                    if (pthread_create(&thread, NULL, penalizzazione, (void*)infoThreadPenalizzazione) != 0) {

                        printf(stderr, "Errore nella creazione del thread infortunio\n");
                    }

                    trovato_player_da_penalizzare = 1;
                }

            }else{ //penalizza un player della squadra B

                playerPenalizzato = match->squadra_B->players[random_player];
                pthread_mutex_lock(&mutexPartite);
                if(playerPenalizzato->infortunato != 1 && playerPenalizzato->penalizzato != 1){
                    playerPenalizzato->penalizzato = 1;
                    printf("Fallo commesso da %s penalizzato per %d min\n",playerPenalizzato->nome_player,random_time_p);
                    pthread_mutex_unlock(&mutexPartite);

                    //Crea thread penalizzazione
                    pthread_t thread;
                    argomentiThreadPenalizzazione *infoThreadPenalizzazione = malloc(sizeof(argomentiThreadPenalizzazione));
                    strcpy(infoThreadPenalizzazione->player,playerPenalizzato->nome_player);
                    infoThreadPenalizzazione->indexPartita = indexPartita;
                    infoThreadPenalizzazione->timeP = random_time_p;
                    // Crea il thread e passa il puntatore alla struct come argomento
                    if (pthread_create(&thread, NULL, penalizzazione, (void*)infoThreadPenalizzazione) != 0) {

                        printf(stderr, "Errore nella creazione del thread infortunio\n");
                    }

                    trovato_player_da_penalizzare = 1;
                }
            }

        }else{ //Penaliza squadra A

            if(random_player == 4){ //Penalliza il capitano della squadra 1

                playerPenalizzato = match->squadra_A->capitano;
                pthread_mutex_lock(&mutexPartite);
                if(playerPenalizzato->infortunato != 1 && playerPenalizzato->penalizzato != 1){
                    playerPenalizzato->penalizzato = 1;
                    printf("Fallo commesso da %s penalizzato per %d min\n",playerPenalizzato->nome_player,random_time_p);
                    pthread_mutex_unlock(&mutexPartite);

                    //Crea thread penalizzazione
                    pthread_t thread;
                    argomentiThreadPenalizzazione *infoThreadPenalizzazione = malloc(sizeof(argomentiThreadPenalizzazione));
                    strcpy(infoThreadPenalizzazione->player,playerPenalizzato->nome_player);
                    infoThreadPenalizzazione->indexPartita = indexPartita;
                    infoThreadPenalizzazione->timeP = random_time_p;
                    // Crea il thread e passa il puntatore alla struct come argomento
                    if (pthread_create(&thread, NULL, penalizzazione, (void*)infoThreadPenalizzazione) != 0) {

                        printf(stderr, "Errore nella creazione del thread infortunio\n");
                    }

                    trovato_player_da_penalizzare = 1;
                }

            }else{ //penalizza un player della squadra A

                playerPenalizzato = match->squadra_A->players[random_player];
                pthread_mutex_lock(&mutexPartite);
                if(playerPenalizzato->infortunato != 1 && playerPenalizzato->penalizzato != 1){
                    playerPenalizzato->penalizzato = 1;
                    pthread_mutex_unlock(&mutexPartite);
                    printf("Fallo commesso da %s penalizzato per %d min\n",playerPenalizzato->nome_player,random_time_p);

                    //Crea thread penalizzazione
                    pthread_t thread;
                    argomentiThreadPenalizzazione *infoThreadPenalizzazione = malloc(sizeof(argomentiThreadPenalizzazione));
                    strcpy(infoThreadPenalizzazione->player,playerPenalizzato->nome_player);
                    infoThreadPenalizzazione->indexPartita = indexPartita;
                    infoThreadPenalizzazione->timeP = random_time_p;
                    // Crea il thread e passa il puntatore alla struct come argomento
                    if (pthread_create(&thread, NULL, penalizzazione, (void*)infoThreadPenalizzazione) != 0) {

                        printf(stderr, "Errore nella creazione del thread infortunio\n");
                    }

                    trovato_player_da_penalizzare = 1;
                }
            }
        }

        if(trovato_player_da_penalizzare == 0)
            pthread_mutex_unlock(&mutexPartite);
    }

    //Rendi indisponibile il player
    pthread_mutex_lock(&mutexPartite);
    playerInfortunato->infortunato = 1;
    pthread_mutex_unlock(&mutexPartite);

    //Costruisci messaggio
    json_object *jobj = json_object_new_object();
    json_object_object_add(jobj, "tipoEvento", json_object_new_string("infortunio"));
    json_object_object_add(jobj, "turnoPlayer", json_object_new_string(playerString));
    json_object_object_add(jobj, "minuti", json_object_new_int(random_number));
    json_object_object_add(jobj, "playerPenalizzato", json_object_new_string(playerPenalizzato->nome_player));
    json_object_object_add(jobj, "minutiP", json_object_new_int(random_time_p));


    const char *json_str = json_object_to_json_string(jobj);
    char *messaggioJSON = strdup(json_str); // Copia la stringa JSON per restituirla
    json_object_put(jobj); // Dealloca l'oggetto JSON
    strcat(messaggioJSON,"\n");

    sendEventoPartecipantiMatch(messaggioJSON,indexPartita);

    sleep(secondiInfortunio);

    //Ritorno del player
    pthread_mutex_lock(&mutexPartite);
    playerInfortunato->infortunato = 0;
    pthread_mutex_unlock(&mutexPartite);

    printf("%s è ritornato dall'infortunio\n",playerString);

    //Costruisci messaggio
    json_object *jobj2 = json_object_new_object();
    json_object_object_add(jobj2, "tipoEvento", json_object_new_string("ritornoInfortunio"));
    json_object_object_add(jobj2, "turnoPlayer", json_object_new_string(playerString));

    const char *json_str2 = json_object_to_json_string(jobj2);
    char *messaggioJSON2 = strdup(json_str2); // Copia la stringa JSON per restituirla
    json_object_put(jobj2); // Dealloca l'oggetto JSON
    strcat(messaggioJSON2,"\n");

    sendEventoPartecipantiMatch(messaggioJSON2,indexPartita);
}

int getSquadraFromPlayer(char *player, int indexPartita){

    partita *match = partite[indexPartita];

    if(strcmp(player,match->squadra_A->capitano->nome_player) == 0) return 0;

    if(strcmp(player,match->squadra_B->capitano->nome_player) == 0) return 1;

    for(int i=0; i<SIZE_ARRAY_PLAYER_PARTECIPANTI; i++){

        if(strcmp(player,match->squadra_A->players[i]->nome_player) == 0) return 0;

        if(strcmp(player,match->squadra_B->players[i]->nome_player) == 0) return 1;
    }

    return -1;
}

int getIndexPlayer(char *player, int indexPartita){

    partita *match = partite[indexPartita];

    if(strcmp(player,match->squadra_A->capitano->nome_player) == 0) return 4;

    if(strcmp(player,match->squadra_B->capitano->nome_player) == 0) return 4;

    for(int i=0; i<SIZE_ARRAY_PLAYER_PARTECIPANTI; i++){

        if(strcmp(player,match->squadra_A->players[i]->nome_player) == 0) return i;

        if(strcmp(player,match->squadra_B->players[i]->nome_player) == 0) return i;
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
        player *playerInfo;

        while(1){

            //Assegna turno al capitano se non è infortunato
            if(indiceTurnoModulato == 4){
                playerInfo = match->squadra_B->capitano;
                if(playerInfo->infortunato != 1 && playerInfo->penalizzato != 1){

                    turnoPlayer = malloc(SIZE_NAME_PLAYER * sizeof(char));
                    strcpy(turnoPlayer,playerInfo->nome_player);

                    //Costruisci messaggio e invia messaggio ai partecipanti
                    json_object *jobj = json_object_new_object();
                    json_object_object_add(jobj, "tipoEvento", json_object_new_string("assegnazioneTurno"));
                    json_object_object_add(jobj, "turnoPlayer", json_object_new_string(turnoPlayer));
                    const char *json_str = json_object_to_json_string(jobj);
                    char *messaggioJSON = strdup(json_str); // Copia la stringa JSON per restituirla
                    json_object_put(jobj); // Dealloca l'oggetto JSON
                    strcat(messaggioJSON,"\n");

                    sendEventoPartecipantiMatch(messaggioJSON,indexPartita);

                    return turnoPlayer;
                }

            }else{

                playerInfo = match->squadra_B->players[indiceTurnoModulato];

                if(playerInfo->infortunato != 1 && playerInfo->penalizzato != 1){

                    turnoPlayer = malloc(SIZE_NAME_PLAYER * sizeof(char));
                    strcpy(turnoPlayer,playerInfo->nome_player);

                    //Costruisci messaggio e invia messaggio ai partecipanti
                    json_object *jobj = json_object_new_object();
                    json_object_object_add(jobj, "tipoEvento", json_object_new_string("assegnazioneTurno"));
                    json_object_object_add(jobj, "turnoPlayer", json_object_new_string(turnoPlayer));
                    const char *json_str = json_object_to_json_string(jobj);
                    char *messaggioJSON = strdup(json_str); // Copia la stringa JSON per restituirla
                    json_object_put(jobj); // Dealloca l'oggetto JSON
                    strcat(messaggioJSON,"\n");

                    sendEventoPartecipantiMatch(messaggioJSON,indexPartita);

                    return turnoPlayer;
                }
            }

            //Se arriva qui non è stato assegnato il turno quindi fa una nuova iterazione con il turno successivo
            (*indiceTurnoB)++;
            indiceTurnoModulato = (*indiceTurnoB)%5;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////

   //--------------------------------ASSENAZIONE TURNO PLYER SQUADRA A---------------------------------------------------
   (*indiceTurnoA)++;
    int indiceTurnoModulato = (*indiceTurnoA)%5;
    char *turnoPlayer;
    player *playerInfo;

    while(1){

        //Assegna turno al capitano se non è infortunato
        if(indiceTurnoModulato == 4){

            playerInfo = match->squadra_A->capitano;

            if(playerInfo->infortunato != 1 && playerInfo->penalizzato != 1){

                turnoPlayer = malloc(SIZE_NAME_PLAYER * sizeof(char));
                strcpy(turnoPlayer,playerInfo->nome_player);

                //Costruisci messaggio e invia messaggio ai partecipanti
                json_object *jobj = json_object_new_object();
                json_object_object_add(jobj, "tipoEvento", json_object_new_string("assegnazioneTurno"));
                json_object_object_add(jobj, "turnoPlayer", json_object_new_string(turnoPlayer));
                const char *json_str = json_object_to_json_string(jobj);
                char *messaggioJSON = strdup(json_str); // Copia la stringa JSON per restituirla
                json_object_put(jobj); // Dealloca l'oggetto JSON
                strcat(messaggioJSON,"\n");

                sendEventoPartecipantiMatch(messaggioJSON,indexPartita);

                return turnoPlayer;
            }

        }else{

            playerInfo = match->squadra_A->players[indiceTurnoModulato];

            if(playerInfo->infortunato != 1 && playerInfo->penalizzato != 1){

                turnoPlayer = malloc(SIZE_NAME_PLAYER * sizeof(char));
                strcpy(turnoPlayer,playerInfo->nome_player);

                //Costruisci messaggio e invia messaggio ai partecipanti
                json_object *jobj = json_object_new_object();
                json_object_object_add(jobj, "tipoEvento", json_object_new_string("assegnazioneTurno"));
                json_object_object_add(jobj, "turnoPlayer", json_object_new_string(turnoPlayer));
                const char *json_str = json_object_to_json_string(jobj);

                char *messaggioJSON = strdup(json_str); // Copia la stringa JSON per restituirla
                json_object_put(jobj); // Dealloca l'oggetto JSON
                strcat(messaggioJSON,"\n");

                sendEventoPartecipantiMatch(messaggioJSON,indexPartita);

                return turnoPlayer;
            }
        }

        //Se arriva qui non è stato assegnato il turno quindi fa una nuova iterazione con il turno successivo
        (*indiceTurnoA)++;
        indiceTurnoModulato = (*indiceTurnoA)%5;
    }
}

void simulaMatch(int indexPartita){

    partita *match = partite[indexPartita];

    //-----------------------------Cerca il player che inizia il turno-----------------------------------------
    int playerTrovato = 0;
    char *playerInizioTurnoString = match->inizioTurno;
    player *playerInizioTurno;

    if(strcmp(playerInizioTurnoString,match->squadra_A->capitano->nome_player) == 0){

        playerInizioTurno = match->squadra_A->capitano;
        playerTrovato = 1;
    }

    if(playerTrovato != 1){

        if(strcmp(playerInizioTurnoString,match->squadra_B->capitano->nome_player) == 0){

            playerInizioTurno = match->squadra_B->capitano;
            playerTrovato = 1;
        }

    }

    if(playerTrovato != 1){

        for(int i=0; i<SIZE_ARRAY_PLAYER_PARTECIPANTI; i++){

            if(strcmp(playerInizioTurno,match->squadra_A->players[i]->nome_player) == 0){

                playerInizioTurno = match->squadra_A->players[i];
                break;
            }

            if(strcmp(playerInizioTurno,match->squadra_B->players[i]->nome_player) == 0){

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


    if(evento == 0){

        tira(playerInizioTurnoString,indexPartita,&scoreA,&scoreB);

    }else if(evento == 1){

        dribbling(playerInizioTurnoString,indexPartita,&scoreA,&scoreB);

    }else{

        pthread_t thread;
        argomentiThreadInfortunio *infoThread = malloc(sizeof(argomentiThreadInfortunio));
        strcpy(infoThread->player_name,playerInizioTurno);
        infoThread->indexPartita = indexPartita;


        // Crea il thread e passa il puntatore alla struct come argomento
        if (pthread_create(&thread, NULL, infortunio, (void*)infoThread) != 0) {

            printf(stderr, "Errore nella creazione del thread infortunio\n");
        }

    }


    while(match->finePartita != 1){

        pthread_mutex_lock(&mutexPartite);
        char *turnoPlayer;
        turnoPlayer = assegna_turno(turnoSquadra,indexPartita,&indiceTurnoA,&indiceTurnoB);
        pthread_mutex_unlock(&mutexPartite);

        if(turnoSquadra == 0) turnoSquadra = 1;
        else turnoSquadra = 0;

        printf("%s ottiene il possesso palla\n",turnoPlayer);

        evento = getEvento();

        if(evento == 0){

            tira(turnoPlayer,indexPartita,&scoreA,&scoreB);

        }else if(evento == 1){

            dribbling(turnoPlayer,indexPartita,&scoreA,&scoreB);

        }else{

            pthread_t thread;
            argomentiThreadInfortunio *infoThread = malloc(sizeof(argomentiThreadInfortunio));
            strcpy(infoThread->player_name,turnoPlayer);
            infoThread->indexPartita = indexPartita;


            // Crea il thread e passa il puntatore alla struct come argomento
            if (pthread_create(&thread, NULL, infortunio, (void*)infoThread) != 0) {

                printf(stderr, "Errore nella creazione del thread infortunio\n");
            }
        }

        free(turnoPlayer);
        sleep(10);
    }

    printf("\n tempo scaduto, match concluso\n");

    //Avvertiti i players della fine del match
    //Costruisci messaggio e invia messaggio ai partecipanti
    json_object *jobj = json_object_new_object();
    json_object_object_add(jobj, "tipoEvento", json_object_new_string("fineMatch"));
    const char *json_str = json_object_to_json_string(jobj);

    char *messaggioJSON = strdup(json_str); // Copia la stringa JSON per restituirla
    json_object_put(jobj); // Dealloca l'oggetto JSON
    strcat(messaggioJSON,"\n");

    sendEventoPartecipantiMatch(messaggioJSON,indexPartita);
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

    int indexPartita = json_object_get_int(indexPartitaJSON);
    char playerInizioTurno[SIZE_NAME_PLAYER];
    strcpy(playerInizioTurno,json_object_get_string(playerInizioTurnoJSON));
    char squadraInizioTurno[SIZE_NAME_TEAM];
    strcpy(squadraInizioTurno,json_object_get_string(squadraInizioTurnoJSON));

    //Assegna inizio turno al player e avvia la simulazione
    if(strcmp(partite[indexPartita]->inizioTurno,"null") == 0){

        strcpy(partite[indexPartita]->inizioTurno,playerInizioTurno);

        partita *nuovaPartita = partite[indexPartita];

        //Costruisci messaggio e invia messaggio ai partecipanti
        json_object *jobj = json_object_new_object();
        json_object_object_add(jobj, "tipoEvento", json_object_new_string("inizioMatch"));
        json_object_object_add(jobj, "playerInizioTurno", json_object_new_string(playerInizioTurno));
        const char *json_str = json_object_to_json_string(jobj);

        char *messaggioJSON = strdup(json_str); // Copia la stringa JSON per restituirla
        json_object_put(jobj); // Dealloca l'oggetto JSON
        strcat(messaggioJSON,"\n");

        sendEventoPartecipantiMatch(messaggioJSON,indexPartita);
        printf("Avvertiti i player delle squadre %s e %s dell'inizio del match: ",nuovaPartita->squadra_A->nome_squadra,nuovaPartita->squadra_B->nome_squadra);
        printf("il primo turno è di %s della squadra %s\n",playerInizioTurno,squadraInizioTurno);

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
