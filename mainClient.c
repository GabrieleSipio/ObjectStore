#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include "./Utilities/objstore.h"
#include "./Utilities/check.h"
#include "./Utilities/rep.h"

struct report rep = {0,0,0};

/*funzione che crea un file di dimesione size*/
char *createFile(int size);

int main(int argc, char* argv[]){
    if(argc!=3){
        /*se argc è diverso da 3 sto avendo un input errato*/
        perror("<nome programma> <nome utente> <numero operazione>");
        return 1;
    }
    char *name=NULL;
    int op=atoi(argv[2]),error=0;
    /*alloco il buffer che contiene il nome dell'utente che vuole iniziare una nuova sessione*/
    CHECK_PTR((name=calloc(WORDSIZE,sizeof(char))),"allocazione della memoria per name fallita!");
    strcpy(name,argv[1]);
    /*aumento il numero di operazioni eseguite ed eseguo la os_connect sul nome*/
    updateOpNo();
    if (os_connect(name)==0){
        /*l'os_connect è fallita quindi aumento il numero delle operazioni fallite nella sessione per il report*/
        updateFailedOpNo();
        perror("Connection failed!");
        /*eseguo una exit con un failure*/
        exit(EXIT_FAILURE);
    }
    printf("CONNECTED TO THE OBJECTSTORE\n");
    /*l'os_connect è andata abuon fine quindi aumento il numero di operazioni eseguite con sucesso*/
    updateSuccOpNo();
    /*eseguo uno switch sull'operazione da ricevuta tramite riga id comando*/
    switch (op){
    case 1:
        printf("Operazione richiesta: STORE\n");
        
        int size=100;
        /*richiamo la funzione os_store 20 volte*/
        for (int k = 1; k <= 20; k++){
              char *name=NULL,*file=NULL;
              /*alloco il buffer che conterrà il nome del file da mandare */
              CHECK_PTR((name=calloc(12,sizeof(char))),"errore durante l'allocazione della stringa name");
              /*creo il file richiamando la funzione createFile*/
              file=createFile(size);
              /*costruisco il nome del documento utilizzando il valore di k per differenziare i vari
              * documenti e aumento il numero delle operazioni totali eseguite*/
              sprintf(name,"%d",k);
              strcat(name,"doc");
              updateOpNo();
              /*eseguo la os_store su quel documento*/
              if (os_store(name,file,size)==0){
                  /*la os_store è fallita quindi aumento il numero di operazioni fallite*/
                  updateFailedOpNo();
                  perror("errore nella store!");
                  error=1;
              }
              else{
                  /*la os_store è andata a buon fine quindi aumento il numero di operazioni andate a buon fine */
                updateSuccOpNo();
                }
                /*libero le strutture dati che ho usato*/
              free(file);
              free(name);
              /*aumentando size di 5258 alla volta otterrò 20 documenti che avranno una dimensione che va da 100 a 100.002*/
              size+=5258;
          }
          break;
      case 2:
          printf("Operazione richiesta: RETRIEVE\n");
          int sz=100;
          char *file=NULL,*s=NULL,*test=NULL;
          /*alloco il buffer che conterrà il nome del documento*/
          CHECK_PTR((s=calloc(WORDSIZE,sizeof(char))),"errore nell'allocazione di s");
          /*eseguo la funzione os_retrieve 20 volte*/
          for (int i = 1; i <= 20; i++){
            /*aumento il numero delle operazioni effettuate*/  
            updateOpNo();
            /*costruisco il nome del documento*/
            sprintf(s,"%d",i);
            strcat(s,"doc");
            /*eseguo la os_retrieve restituendo*/
            file=(char*) os_retrieve(s);
            if (file==NULL) {
                /*il risultato che mi viene restituito dalla os_retrieve è NULL quindi
                * aumento il numero delle operazioni fallite*/
                updateFailedOpNo();
                perror("retreive fallita!");
                error=1;
            }
            else{
                /*creo un file con lo stesso metodo usato nella store*/
                test=createFile(sz);
                /*eventualmente posso scegliere di stampare il file ricevuto a schermo*/
                //printf("%s\n",file);
                /*confronto i due file*/
                if(strcmp(file,test)==0){
                  /*i file conicidono stampo l'esito sullo stdout e aumento il numero di operazioni concluse con successo*/
                 printf("i file coincidono!\n");
                 updateSuccOpNo();
                }
                else{
                    /*i file non coincidono, stampo l'esito sullo stdout e aumento il numero di operazioni fallite*/
                    printf("i file non coincidono!\n");
                    updateFailedOpNo();
                    error=1;
                }
            }
            /*libero le strutture, e aumento sz per il test successivo*/
            free(file);
            sz+=5258;
            memset(s,0,WORDSIZE);
            free(test);
          }
          /*uscito dal ciclo libero il buffer che contiene il nome della stringa*/
          free(s);
          break;
      case 3:
          printf("Operazione richiesta: DELETE\n");
          char *name=NULL;
          /*alloco il buffer che ha il nome del documento*/
          CHECK_PTR((name=calloc(WORDSIZE,sizeof(char))),"errore durante l'allocazione della stringa name");
          /*eseguo la funzione os_delete 20 volte*/
          for (int i = 1; i <= 20; i++){
            /*costruisco il nome da ma manda*/
            sprintf(name,"%d",i);
            strcat(name,"doc");
            /*aumento il numero di operazioni effettuate*/
            updateOpNo();
            if (os_delete(name)==0){
                /*la os_delete ha dato esito negativo quindi aumento il numero di operazioni fallite*/
                updateFailedOpNo();
                perror("delete fallita!");
                error=1;
            }
            else{
                /*la os_delete ha dato esito positivo quindi aumento il numero di operazioni andate a buon fine*/
                updateSuccOpNo();
                memset(name,0,WORDSIZE);
            }
          }
          /*libero la struttura che contiene il nome*/
          free(name);
          break;
      default:
          break;
      }
    if (error==0){
        /*l'error non è settato vuol dire che il test è stato passato*/
        printf("Test %d passed!\n",op);   
    }
    else{
        /*l'error è settato, il test non è stato passato*/
        printf("Test %d not passed!\n",op);
    }
    /*aumento il numero di operazioni effettuate*/
    updateOpNo();
    /*eseguo la os_disconnect*/
    if (os_disconnect()){
        /*la funzione ha ritornato un valore positivo quindi aumento il numero di operazioni eseguite con successo*/
        updateSuccOpNo();
        printf("client disconnesso!\n");
    }
    else{
        /*la funzione ha ritornato un valore negate quindi aumento il numero di operazioni fallite*/
        updateFailedOpNo();
        perror("disconnessione non riuscita");
    }
    /*libero il buffer che contiene il nome dell'utente la cui sessione è stata appena chiusa*/
    free(name);
    /*restituisco il report sulle operazioni appena concluse*/
    printRep();
    return 0;
}

char *createFile(int size){
    char *tmp=NULL;
    /*alloco il buffer che conterrà il file con dimensione size*/
    CHECK_PTR((tmp=calloc((size+1),sizeof(char))),"errore durante l'allocazione della stringa test!")
    /*riempio il file con i caratteri da a...z convertendo gli interi in lettere*/
    int j=97;
    for (int i = 0; i < size; i++){
        tmp[i]=(char)j;
        j++;
        /*se sono dopo 122 sono già arrivato al carattere z quindi risetto j a 97(a)*/
        if (j>122){
            j=97;
        }
    }
    /*restituisco il buffer*/
    return tmp;
}