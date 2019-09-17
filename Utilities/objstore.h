#include <stdio.h>
#ifndef OBJSTORE_H_
#define OBJISTORE_H_

//funzione che ha lo scopo di tokenizzare i messaggi di reesponso delle operazioni dal server e ritorna un booleano
int checkResult(char *buff);
//funzione che ha lo scopo di registrare un utente di nome "name" all'objectStore
int os_connect(char *name);
//funzione che ha lo scopo di salvare un blocco dati di dimensione "len" con nome "name" all'interno dell'objectStore
int os_store(char *name,void *block, size_t len);
//funzione che ha lo scopo di scaricare un blocco dati di nome "name" dall'objectStore
void *os_retrieve(char *name);
//fuznione che ha lo scopo di eliminare un blocco dati di nome "name" dall'objectStore
int os_delete(char *name);
//funzione che ha lo scopo di disconnettere l'utente che la chiama dall'objectStore
int os_disconnect();
#endif