#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "squadra.h"
#include "player.h"
#include "partita.h"
#include "gestioneConnessioni.h"
#include "variabiliGlobali.h"
#include "gestioneThread.h"


//Inizializzazione variabili globali
pthread_mutex_t mutexListaSquadre = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condListaSquadre = PTHREAD_COND_INITIALIZER;
squadra *squadreInCostruzione[50]; //Array di tutte le squadre fondate in attesa di essere completate e partipare a un match

player *playersConnessi[50];//Array di tutti gli utenti connnessi
pthread_mutex_t mutexPlayers = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condPlayers = PTHREAD_COND_INITIALIZER;

squadra *squadreComplete[15];//Array delle squadre complete e in attesa di match
pthread_mutex_t mutexSquadreAttesa = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condSquadreAttesa = PTHREAD_COND_INITIALIZER;

partita *partite[5];//Array delle partite in corso
pthread_mutex_t mutexPartite = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condPartite = PTHREAD_COND_INITIALIZER;

void *gestione_richieste_client(void *arg){

    char client_message[2000]; //Messaggio client
    int client_sock = *(int *)arg;
    pthread_t threads[3];
    int threadsAttivati[3];

    while(1){

        //Lettura messaggio del client
            if (recv(client_sock, client_message, 2000, 0) < 0) { //Gestione disconessione client

                printf("Ricezione fallita\n");

                //Acquisizione mutex
                pthread_mutex_lock(&mutexListaSquadre);
                pthread_mutex_lock(&mutexPlayers);

                gestione_disconessione_client(client_sock);

                //Rilascio mutex
                pthread_mutex_unlock(&mutexListaSquadre);
                pthread_mutex_unlock(&mutexPlayers);

                close(client_sock);

                printf("Client disconesso\n");

                pthread_cond_broadcast(&condListaSquadre);
                printf("Avvertiti i client di eventuali squadre sciolte\n");

                pthread_exit(NULL);
            }

            //Decodifica del tipo di richiesta del client
            char *tipoRIchiesta = get_tipo_richiesta(client_message);

            if(strcmp(tipoRIchiesta,"nuovoUtente")==0){

                printf("Richiesta connessione player\n");

                aggiungi_utente_connesso(client_message,client_sock);

            }else if(strcmp(tipoRIchiesta,"newSquadra")==0) {

                printf("Richiesta creazione nuova squadra\n");

                int stato = aggiungi_nuova_squadra(client_message, client_sock);

                if(stato == 1){

                    pthread_cond_broadcast(&condListaSquadre);
                    printf("Avvertiti i client della nuova squadra fondata\n");

                }


            }else if(strcmp(tipoRIchiesta,"getSquadreInCostruzione")==0) {

                printf("Richiesta lista squadre fondate\n");

                int *sock_client_arg = malloc(sizeof(int));
                *sock_client_arg = client_sock;
                if(pthread_create(&threads[0],NULL,send_lista_squadre_client,(void*)sock_client_arg) < 0){

                    perror("Errore creazione thread");
                    exit(EXIT_FAILURE);
                }

            }else if(strcmp(tipoRIchiesta,"partecipazioneSquadra") == 0){

                printf("Richiesta di parcetipazione a una squadra\n");

                //Acquisizione mutex
                pthread_mutex_lock(&mutexListaSquadre);
                pthread_mutex_lock(&mutexPlayers);

                aggiungi_richiesta_partecipazione_squadra(client_message,client_sock);

                //Rilascio mutex
                pthread_mutex_unlock(&mutexListaSquadre);
                pthread_mutex_unlock(&mutexPlayers);

            }else if(strcmp(tipoRIchiesta,"decisioneCapitano") == 0){

                printf("Richiesta decisione capitano\n");

                //Acquisizione mutex
                pthread_mutex_lock(&mutexListaSquadre);
                pthread_mutex_lock(&mutexPlayers);

                aggiornamento_composizione_squadra(client_message);

                //Estrazione della squadra dal messaggio
                struct json_object *parsed_json;
                parsed_json = json_tokener_parse(client_message);
                json_object *nomeSquadra;
                json_object_object_get_ex(parsed_json, "squadra", &nomeSquadra);

                //Avverti i client della squadra dell'aggiornamento
                send_aggiornamento_composizione_squadra(json_object_get_string(nomeSquadra));

                //Rilascio mutex
                pthread_mutex_unlock(&mutexListaSquadre);
                pthread_mutex_unlock(&mutexPlayers);

                //Sveglia il thread aggiornamenti squadre per aggiornare il numero di partecipanti
                pthread_cond_broadcast(&condListaSquadre);
                printf("Avvertiti i client di eventuali aggiornamento del numero di partecianti\n");

            }else if(strcmp(tipoRIchiesta,"cercaMatch") == 0){

                printf("Richiesta cerca match\n");

                //Acquisizione mutex
                pthread_mutex_lock(&mutexListaSquadre);
                pthread_mutex_lock(&mutexPlayers);
                pthread_mutex_lock(&mutexSquadreAttesa);
                pthread_mutex_lock(&mutexPartite);

                //Cerca squadra avversaria e avvisa l client della ricerca match
                int indexPartita = cerca_squadra_match(client_message,client_sock);
                avvisa_players_stato_match(indexPartita,client_message);

                //Rilascio mutex
                pthread_mutex_unlock(&mutexListaSquadre);
                pthread_mutex_unlock(&mutexPlayers);
                pthread_mutex_unlock(&mutexSquadreAttesa);
                pthread_mutex_unlock(&mutexPartite);

            }else if(strcmp(tipoRIchiesta,"inizioTurno") == 0){

                int indexPartita = get_index_partita(client_message);

                pthread_mutex_lock(&mutexPartite);

                partita *match = partite[indexPartita];

                //Avvia il processo di nuova partita una sola volta
                if(match->inizioPartita != 1){

                    match->inizioPartita = 1;

                    pthread_mutex_unlock(&mutexPartite);

                    pid_t pid = fork();

                    if(pid < 0){

                        printf("Errore creazione processo partita\n");
                    }

                    if(pid == 0){

                    printf("sono nel processo figlio\n");

                    //Inizializza il seed con l'ora corrente
                    srand(time(NULL));

                    assegna_turno_iniziale_e_avvia_match(client_message);

                    }else if(pid > 0){

                        wait(NULL);
                        free(match);
                        partite[indexPartita] = NULL;
                    }

                }else{

                    pthread_mutex_unlock(&mutexPartite);
                }

            }
    }
}

void avvia_server(int porta) {

    //Dichiarazione variabili
    int server_sock, client_sock, c; //descrittori socket
    struct sockaddr_in server, client; //Informazioni server e client
    for(int i = 0; i<50; i++) //Inizializzaione arraySuqadre a NULL
        squadreInCostruzione[i] = NULL;
    pthread_t newThread;

    //Creazione socket del server
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Impossibile creare socket");
        exit(EXIT_FAILURE);
    }

    //Inizializzazione info server
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(porta);

    //Bind del server alla porta 8080
    if (bind(server_sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("Bind fallito");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server avviato..\n");
    printf("In attesa di richieste\n");

    //Ascolto delle richieste
    if(listen(server_sock,10) < 0){

        perror("listen fallito");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    //qui

    c = sizeof(struct sockaddr_in);
    while(1) {

        //Attesa di nuovi client
        client_sock = accept(server_sock, (struct sockaddr*)&client, (socklen_t*)&c); //Richiesta dal client ricevuta
        if (client_sock < 0) {
            perror("Accettazione fallita");
            close(server_sock);
            exit(EXIT_FAILURE);
        }

        printf("Nuova connessione accettata\n");

        //Avvia un nuovo thread, per il nuovo client connesso, che resta in ascolto di nuovo richieste
        int *arg = (int *)malloc(sizeof(int));
        *arg = client_sock;
        pthread_create(&newThread,NULL,gestione_richieste_client,(void*)arg);

        printf("In attesa di nuove connessioni\n");
    }

    close(server_sock);
}

int main() {
    avvia_server(8080);
    return 0;
}

