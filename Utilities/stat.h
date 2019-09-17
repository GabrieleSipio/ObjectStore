#include <stdio.h>
#include <pthread.h>
#ifndef STAT_H_
#define STAT_H_

pthread_mutex_t mux;


struct statistics {
    unsigned long onlineUsr;                      //n. utenti connessi
    unsigned long objNo;                          //n. oggetti nello store
    unsigned long storeSize;                      //grandezza store
};
/* funzione per stampare il report del server*/
static inline void printStats() {
    pthread_mutex_lock(&mux);
    extern struct statistics stats;
    printf("\n--------------------REPORT--------------------\n");
    printf("Time: %ld\nN. client Online: %ld\nN. oggetti nello store: %ld\nGrandezza totale dello store: %ld\n",(unsigned long)time(NULL),
          stats.onlineUsr,
          stats.objNo,
          stats.storeSize);
    printf("--------------------END REPORT--------------------\n");      
    pthread_mutex_unlock(&mux);
}
/*funzione che restituisce il numero di utenti presenti nell'objstore*/
static inline long getUsr(){
    long check;
    pthread_mutex_lock(&mux);
    extern struct statistics stats;
    check=stats.onlineUsr;
    pthread_mutex_unlock(&mux);
    return check;
}
/*funzione che riduce il numero di utenti online*/
static inline void remOnlineUsr(){
    pthread_mutex_lock(&mux);
    extern struct statistics stats;
    stats.onlineUsr--;
    pthread_mutex_unlock(&mux);
}
/*funzione che aumenta il numero di utenti online*/
static inline void addOnlineUsr(){
    pthread_mutex_lock(&mux);
    extern struct statistics stats;
    stats.onlineUsr++;
    pthread_mutex_unlock(&mux);
}
/*funzione che aumenta il numero di oggetti presenti nell'objectstore*/
static inline void addObj(unsigned long newSize){
   pthread_mutex_lock(&mux);
    extern struct statistics stats;
    stats.objNo++;
    stats.storeSize+=newSize;
    pthread_mutex_unlock(&mux);
}
/*funzione che riduce il numero di oggetti presenti nell'objectstore*/
static inline void remObj(unsigned long newSize){
    pthread_mutex_lock(&mux);
    extern struct statistics stats;
    stats.objNo--;
    stats.storeSize-=newSize;
    pthread_mutex_unlock(&mux);
}
/*funzione per rimuovere la lock*/
static inline void dispose(){
    pthread_mutex_destroy(&mux);
}
#endif