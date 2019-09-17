#include <stdio.h>
#include <pthread.h>
#ifndef REP_H_
#define REP_H_


struct report {
    unsigned long opNo;                              //n. operazioni effettuate
    unsigned long succOpNo;                          //n. operazioni effettuate con successo
    unsigned long failedOpNo;                        //n. operazioni fallite
};
/*funzione che stampa il report del client*/
static inline void printRep() {
    extern struct report rep;
    printf("\n--------------------REPORT--------------------\n");
    printf("Operazioni Effettuate:%ld\nOperazioni effettuate con Successo:%ld\nOperazioni effettuate senza successo:%ld\n",rep.opNo,rep.succOpNo,rep.failedOpNo);
    printf("--------------------END REPORT--------------------\n");
}
/*funzione che aumenta il numero di operazioni eseguite dal client*/
static inline void updateOpNo() {
    extern struct report rep;
    rep.opNo++;
}
/*funzione che aumenta che il numero di operazioni terminate con successo nel client*/
static inline void updateSuccOpNo() {
    extern struct report rep;
    rep.succOpNo++;
}
/*funzione che aumenta il numero di operazioni fallite nel client*/
static inline void updateFailedOpNo() {
    extern struct report rep;
    rep.failedOpNo++;
}

#endif