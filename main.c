#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "squadra.h"
#include "player.h"
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

pthread_mutex_t mutexSquadra = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condSquadra = PTHREAD_COND_INITIALIZER;

pthread_mutex_t mutexDecisioneCap = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condDecisioneCap = PTHREAD_COND_INITIALIZER;

void *gestione_richieste_client(void *arg){

    char client_message[2000]; //Messaggio client
    int client_sock = *(int *)arg;
    pthread_t threads[3];
    int threadsAttivati[3];

    while(1){

        //Lettura messaggio del client
            if (recv(client_sock, client_message, 2000, 0) < 0) {
                printf("Ricezione fallita\n");
                close(client_sock);
                printf("Client disconesso\n");
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

                /*thread_data *argThread;
                    argThread = (thread_data *)malloc(sizeof(thread_data));

                    if(argThread != NULL){

                        argThread->clientSock = client_sock;
                        strcpy(argThread->messaggio,client_message);

                        if(pthread_create(&(argThread->thread_id),NULL,send_aggiornamento_composizione_squadre,(void*)argThread) < 0){

                            perror("Errore creazione thread");
                            exit(EXIT_FAILURE);
                        }
                    }*/

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

                aggiungi_richiesta_partecipazione_squadra(client_message,client_sock);

                /*thread_data *argThread;
                argThread = (thread_data *)malloc(sizeof(thread_data));

                if(argThread != NULL){

                    argThread->clientSock = client_sock;
                    strcpy(argThread->messaggio,client_message);

                    if(pthread_create(&(argThread->thread_id),NULL,aggiungi_richiestaPartecipazione_squadra,(void*)argThread) < 0){

                        perror("Errore creazione thread");
                        pthread_exit(NULL);
                    }

                }*/
            }else if(strcmp(tipoRIchiesta,"decisioneCapitano") == 0){

                gestione_decisioneCapitano(client_message);
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

