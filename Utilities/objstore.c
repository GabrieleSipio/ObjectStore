#include "objstore.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/time.h>
#include "check.h"

int sock,res;

int checkResult(char *buff){
    char **param=NULL;
    param=tokenizer(buff);
    printf("%s\n",buff);
    if (strcmp(param[0],"OK")==0){
        cleanPtr(param);
        return 1;
    }
    if (strcmp(param[0],"KO")==0){
        cleanPtr(param);
        return 0;
    }
    return -1; 
}

int os_connect(char *name){
    /*creo la socket ed eseguo la connect*/
    struct sockaddr_un sa;
    strncpy(sa.sun_path,"objstore.sock",sizeof(sa.sun_path));
    sa.sun_family=AF_UNIX;
    CHECK_OP((sock=socket(AF_UNIX,SOCK_STREAM,0)),"Errore durante la creazione della socket!\n");
    while (connect(sock,(struct sockaddr*) &sa,sizeof(sa))==-1){
        if (errno==ENONET){
            sleep(1);
        }
        else{
            exit(EXIT_FAILURE);
        }
    }
    /*costruisco il messaggio da mandare al server*/
    char msg[WORDSIZE]="REGISTER ";
    strcat(msg, name);
    strcat(msg, "\n");
    /*mando il messaggio attraverso la socket 
    * se il risultato è <0 esco con false*/
    if (write(sock,msg,strlen(msg)+1)<0){
       perror("write failed!\n");
       return 0;
    }
    /*svuoto la stringa per prepararla per la ricezione dell'esito
    * dell'operazione da parte del server*/
    memset(msg,0,WORDSIZE);
    /*leggo da parte del server l'esito dell'operazione
    * se la read restituisce un valore <0 esco con false*/
    if(read(sock,msg,WORDSIZE)<0){
        perror("read failed in os_connect!\n");
        return 0;
    }
    /*mando l'esito alla check result per il controllo*/
    return checkResult(msg);
}

int os_store(char *name,void *block, size_t len){
    char *buff=NULL,*stringLen=NULL;
    /*alloco la stringa per contenere la lunghezza del file da salvare*/
    CHECK_PTR((stringLen=calloc(WORDSIZE,sizeof(char))),"errore durante l'allocazione della stringLen nella os_store!")
    /*alloco la stringa che conterrà la richiesta da mandare al server*/
    CHECK_PTR((buff=calloc(WORDSIZE,sizeof(char))),"errore durante l'allocazione del buffer nella os_store!");
    sprintf(stringLen,"%ld",len);
    /*costruisco la richiesta da mandare al server*/
    strcpy(buff,"STORE ");
    strcat(buff,name);
    strcat(buff," ");
    strcat(buff, stringLen);
    strcat(buff,"\n");
    /*mando la richiesta al server*/
    if (write(sock,buff,WORDSIZE)<0){
        perror("write alla socket in os_store fallita all'interno della os_store");
        return 0;
    }
    /*mando il file con la writen per essere sicuro di
    * scrivere sulla socket esattamente len caratteri*/
    if (writen(sock,block,len)<=0){
        perror("write del file sulla socket di os_store fallita!");
    }
    /*svuoto il buffer per mettermi in attesa di ricevere l'esito dell'operazione dal server*/
    memset(buff,0,WORDSIZE);
    if (read(sock,buff,WORDSIZE)<0){
        perror("errore nel leggere la risposta ll'interno della os_store");
    }
    /*libero le varie strutture e assegno all'intero res il risultato della checkResult di modo 
    * da poter libreare anche buff*/
    free(stringLen);
    res=checkResult(buff);
    free(buff);
    return res;
}

void *os_retrieve(char *name){
    char *buff=NULL,**tokMsg=NULL;
    /*alloco il buffer per la richiesta da inoltrare al server*/
    CHECK_PTR((buff=calloc(WORDSIZE,sizeof(char))),"errore durante l'allocazione del buffer in os_retrieve");
    /*costruisco la richiesta*/
    strcpy(buff,"RETRIEVE ");
    strcat(buff,name);
    /*mando la richiesta al server*/
    if (write(sock,buff,WORDSIZE)<0){
        perror("os_retireve fallita!(write1)");
        return NULL;
    }
    /*svuoto il buffer e mi metto in attesa di rcevere una risposta dal server*/
    memset(buff,0,WORDSIZE);
    if (read(sock,buff,WORDSIZE)<0){
        perror("os_retrieve fallita!(read1)");
        return NULL;
        
    }
    printf("%s\n",buff);
    tokMsg=tokenizer(buff);
    /*tokenizzo il buffer e controllo il contenuto della prima cella*/
    if (strcmp(tokMsg[0],"DATA")==0){
        /*ho ricevuto un messaggio positivo.*/
        char *rFile=NULL;
        int len=atoi(tokMsg[1]);
        /*alloco il buffer su cui salverò il file*/
        CHECK_PTR((rFile=calloc((len+1),sizeof(char))),"errore durante l'allocazione di rFilen nella os_retrieve!");
        /*libero la struttura dati che ho usato per ricevere la risposta inizale da parte del server*/
        memset(rFile,0,len);
        /*leggo dal server tramite readn per essere sicuro di leggere esattamente len caratteri dalla socket*/
        if (readn(sock,rFile,len)<len){
            perror("read da server fallita in os_retrieve!");
            return NULL;
        }
        /*libero il buffer e restituisco il file*/
        cleanPtr(tokMsg);
        free(buff);
        return rFile;
    }
    cleanPtr(tokMsg);
    /*il messaggio che ho ricevuto dal server era un messaggio di KO quindi libero il buffer
    * e esco con false*/
    free(buff);
    return NULL;
}

int os_delete(char *name){
    char *buff=NULL;
    /*alloco il buffer per mandare al richiesta al server*/
    CHECK_PTR((buff=calloc(WORDSIZE,sizeof(char))),"allocazione del buffer nella os_delete fallita!");
    /*costruisco la richiesta da mandare al server*/
    strcpy(buff,"DELETE ");
    strcat(buff,name);
    /*mando la richesta al server*/
    if (write(sock,buff,strlen(buff))<0){
        perror("write del messaggio di delete alla socket fallita in os_delete!");
        return 0;
    }
    /*svuoto il buffer per mettermi in attesa dell'esito dell'operazione
    * da parte del server*/
    memset(buff,0,WORDSIZE);
    if (read(sock,buff,WORDSIZE)<0){
        perror("read dalla socket per il risultato della os_delete fallita!");
        return 0;
    }
    /*assegno alla variabile res il risultato di checkResult di modo da poter liberare il buffer*/
    res=checkResult(buff);
    free(buff);
    return res;
}

int os_disconnect(){
    char *msg=NULL;
    /*alloco la struttura per mandare il messaggio al server*/
    CHECK_PTR((msg=calloc(WORDSIZE,sizeof(char))),"errore durante l'allocazione della stringa all'interno di os_disconnect!");
    /*costruisco il messaggio da mandare*/
    strcpy(msg,"LEAVE");
    /*mando il messaggio attraverso la socket*/
    if (write(sock,msg,strlen(msg)+1)<0){
        perror("scrittura di os_disconnect sulla socket fallita!");
        return 0;
    }
    /*svuoto il buffer per mettermi in attesa dell'esito dell'operazione*/
    memset(msg,0,WORDSIZE);
    if (read(sock,msg,WORDSIZE)<0){
        perror("lettura dalla socket in os_disconnect fallita!");
        return 0;
    }
    printf("%s\n",msg);
    /*libero il buffer e chiudo la socket*/
    free(msg);
    close(sock);
    return 1;
}

