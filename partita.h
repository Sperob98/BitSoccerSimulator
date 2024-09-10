#ifndef PARTITA_H
#define PARTITA_H

#include "squadra.h"

typedef struct{

    squadra *squadra_A;
    squadra *squadra_B;
    char inizioTurno[30];
    int finePartita;
    int inizioPartita;

}partita;

typedef struct{

    int indexPartita;
    char *player;

}argomentiThreadInfortunio;


void avvisa_players_stato_match(int indexPartita, char *messaggio);

int get_index_partita(char *messaggio);

void assegna_turno_iniziale_e_avvia_match(char *messaggio);

#endif
