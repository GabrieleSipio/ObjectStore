#include <stdio.h>
#include <errno.h>
#ifndef CHECK_H_
#define CHECK_H_
/*definisco una grandezza standard che verrà usata come modello per stabilire la dimensione dei buffer*/
#define WORDSIZE 256
/*macro che ha lo scopo di controllare che le operazioni sui puntatori vadano a buon fine*/

#define CHECK_PTR(ptr, string) \
    if (!ptr) { \
        perror("Error during the allocation of ponter: \n");\
        perror(string); \
        EXIT_FAILURE; \
    }
/*macro che ha lo scopo di controllare che le operazioni sui file descriptor vadano a buon fine*/
#define CHECK_OP(opRes,opString) \
    if (opRes==-1){ \
        perror(opString); \
        EXIT_FAILURE; \
    }
/*macro che ha lo scopo di controllare che le operazioni sui thread vadano a buon fine*/
#define CHECK_THREAD(opRes,opString) \
    if (opRes!=0){ \
        perror(opString); \
        EXIT_FAILURE; \
    }


/*da linea 28 a 38 e metodo a riga 42 ricavati da questo link
https://stackoverflow.com/questions/16844728/converting-from-string-to-enum-in-c */
typedef enum {STORE,RETRIEVE,DELETE,LEAVE}operation;

const static struct {
    operation      val;
    const char *str;
} conversion [] = {
    {STORE, "STORE"},
    {RETRIEVE, "RETRIEVE"},
    {DELETE, "DELETE"},
    {LEAVE, "LEAVE"},
};
//funzione che serve a tokenizzare le crichieste scambiate da client e server durante una sessione
char **tokenizer(char *str);
//una funzione che restituisce un tipo enum a partire da una stringa (soluzione presa da stackOverflow)
operation str2enum (const char *str);
//funzione che serve ad effettuare la deallocazione compelta di un doppio puntatore
void cleanPtr(char **ptr);
//funzione che serve a leggere esattamente n caratteri dalla socket
int readn(long fd, void *buf, size_t size);
//funzione che serve a scrivere esattamente n caratteri sulla socket
int writen(long fd, void *buf, size_t size);

#endif