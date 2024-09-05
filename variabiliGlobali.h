#ifndef VARIABILI_GLOBALI_H
#define VARIABILI_GLOBALI_H

#include "squadra.h"
#include "player.h"
#include "partita.h"
#include <pthread.h>

extern pthread_mutex_t mutexListaSquadre;
extern pthread_cond_t condListaSquadre;
extern squadra *squadreInCostruzione[50];

extern player *playersConnessi[50];
extern pthread_mutex_t mutexPlayers;
extern pthread_cond_t condPlayers;

extern squadra *squadreComplete[15];
extern pthread_mutex_t mutexSquadreAttesa;
extern pthread_cond_t condSquadreAttesa;

extern partita *partite[5];
extern pthread_mutex_t mutexPartite;
extern pthread_cond_t condPartite;

#endif
