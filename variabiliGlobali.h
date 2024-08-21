#ifndef VARIABILI_GLOBALI_H
#define VARIABILI_GLOBALI_H

#include "squadra.h"
#include "player.h"
#include <pthread.h>

extern pthread_mutex_t mutexListaSquadre;
extern pthread_cond_t condListaSquadre;
extern squadra *squadreInCostruzione[50];

extern player *playersConnessi[50];
extern pthread_mutex_t mutexPlayers;
extern pthread_cond_t condPlayers;

extern pthread_mutex_t mutexSquadra;
extern pthread_cond_t condSquadra;

extern pthread_mutex_t mutexDecisioneCap;
extern pthread_cond_t condDecisioneCap;
#endif
