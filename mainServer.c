#define _POSIX_C_SOURCE  200112L
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <pthread.h>
#include <string.h>
#include "./Utilities/objstore.h"
#include "./Utilities/check.h"
#include "./Utilities/stat.h"

/*struct che serve a mantenere una lista che contiene l'associazione tra nome dell'utente 
* e indirizzo della socket*/
typedef struct uD{
    int sock;
    char *name;
    struct uD* next;
    struct uD* prev;
    struct uD* last;
}userDesc;

/*flag ausiliario per la chiusura. Indica quando non ci sono più
* sessioni attive nel server*/
volatile int terminated=0;

volatile sig_atomic_t sigFlag;

/*struttura che mantiene la diagnostica del server*/
struct statistics  stats = { 0,0,0 };

/*funzione che serve ad aggiungere una nuova associazione tra utente e socket */
void addEl(userDesc **list, char* name, int sock,userDesc **last);
/*funzione che mi serve per liberare tutta la lista che mantiene le associazioni tra utente e socket*/
void freeList(userDesc **list);
/*funzione che serve registrare l'utente all'object store andando a creare, in caso non esista già,
* la sua cartella personale*/
int receive(int socket, char **usrname);
/*handler che gestisce i segnali*/
void signalHandler(int sig);
/*detector per i segnali*/
void signalDetector();
/*task da assegnare ai nuovi thread creati*/
void *newSocket(void *val);

int main(int argc, char *argv[]){
    unlink("objstore.sock");
    struct sockaddr_un sa;
    /*dichiaro la socket del server che si mette in asolto, la socket del client con cui avrò le interazioni e la dimensione del threadpool che viene 
    * inizializzzato ad 1*/
    int sSock,cSock,poolSize=0;
    /*dichiaro il threadpool*/
    pthread_t *threadPool=NULL;
    /*dichiaro due puntatori di tipo userDesc che hanno ruoli di head(usr) e tail(last)*/
    userDesc *usr=NULL,*last=NULL;
    /*dichiaro i buffer che conterranno il nome dell'utente che vuole registrarsi all'objectstore(name) e i messaggi di OK  o KO riguradanti 
    * la fase di registrazione(msg)*/
    char *msg=NULL,*name=NULL;
    /*installo i signal handler*/
    signalDetector();
    /*alloco i buffer e inizio la sequenza di setupe e ascolto da parte del server di nuove connessioni*/
    CHECK_PTR((name=calloc(WORDSIZE,sizeof(char))),"errore durante l'allocazione della stringa!");
    CHECK_PTR((msg=calloc(WORDSIZE,sizeof(char))),"errore durante l'allocazione della risposta");
    strncpy(sa.sun_path,"objstore.sock",sizeof(sa.sun_path));
    sa.sun_family=AF_UNIX;
    CHECK_OP((sSock=socket(AF_UNIX,SOCK_STREAM,0)),"Errore durante la creazione del server Socket!\n");
    printf("[SERVER] I'm ready!\n");
    CHECK_OP(bind(sSock,(struct sockaddr*) &sa, sizeof(sa)),"Errore durante la bind!\n");
    CHECK_OP(listen(sSock,SOMAXCONN),"Errore durante la listen!\n");
    /*avvio un ciclo che dura fino a che non rilevo un segnale di terminazione*/
    while (!((sigFlag==SIGINT) || (sigFlag==SIGQUIT) || (sigFlag==SIGTERM))){
        /*se ricevo il segnale SIGUSR1 stampo la diagostica del server e resetto sigflag*/
        if (sigFlag==SIGUSR1){
            printStats();
            sigFlag=0;
        }
        cSock=accept(sSock,NULL,0);
        if(cSock>=0){   
            /*se dopo la accept cSock è >=0 vuol dire che il client si è connesso correttamente*/
            printf("[SERVER] Il client %d si è connesso!\n",cSock);
            /*procedo dunque alla fase vera e propria di registrazione eseguendo la recieve*/
            if (receive(cSock,&(name))){
                /*la funzione ha dato un risultato positivo quindi costruisco il messaggio di OK e lo mando sulla
                * socket*/
                strcpy(msg,"OK\n");
                write(cSock,msg,strlen(msg));
                /*aumento il poolsize*/
                poolSize++;
                /*inserisco la nuova associazione <nome utente - socket> nella lista*/
                addEl(&usr,name,cSock,(&last));
                if (threadPool==NULL){
                    /*il puntatore è NULL quindi eseguo una calloc per istanziare il threadPool*/
                    CHECK_PTR((threadPool=calloc(poolSize,sizeof(pthread_t))),"errore durante l'allocazione del threadpool!"); 
                }
                else{
                    /*l'array è già stato istanziato quindi lo rialloco con la nuova dimesnione*/
                    CHECK_PTR((threadPool=realloc(threadPool,poolSize*sizeof(pthread_t))),"errore durante la riallocazione del threadpool");
                    
                }
                /*eseguo la pthread create passando il task e il puntatore last che punta all'ultimo elemento della lista quindi al nuovo utente*/
                CHECK_THREAD(pthread_create(&threadPool[poolSize-1],NULL,newSocket,(void*) last),"Errore durante la creazione del thread!\n");
                addOnlineUsr();
            }
            else{
                /*la funzione recieve ha dato esito negativo quindi mando il messaggio di failure attraverso la socket*/
                strcpy(msg,"KO Register failed \n");
                write(cSock,msg,strlen(msg));
            }
            /*svuoto il buffer dei messaggi*/
            memset(msg,0,WORDSIZE); 
        }
    }
    /*uscito dal while avvio la fase di chiusura */
    printf("\n//----------FASE DI CHIUSURA----------\\\n");
    /*libero la lista che mantiene l'associazione di tra <nome utente - socket>*/
    freeList(&usr);
    printf("\n//----------HO LIBERATO LA LISTA----------\\\n");
    if (threadPool!=NULL){
        /*se il threadpool è stato inizializzato mi metto in attesa della terminazione dei thread ancora attivi*/
        printf("\n//----------ATTENDO LA TERMINAZIONE DEI THREAD ANCORA IN ESECUZIONE----------\\\n");
        for (int i = 0; i < poolSize; i++) {
            pthread_join(threadPool[i], NULL);
        }
        /*terminate le attività dei thread libero il puntatore*/
        free(threadPool);
    }
    printf("\n//----------THREAD TERMINATI PROCEDO ALLA CHIUSURA DEFINITIVA----------\\\n");
    /*libero i buffer usati per il nome e i messaggi eseguo l'unlink del .sock e chiudo*/
    free(name);
    free(msg);
    unlink("objstore.sock");
    dispose(); 
    return 0;
}

void addEl(userDesc **list, char* name, int sock,userDesc **last){
    if(name==NULL || sock<0){
        /*parametri non validi esco dalla funzione*/
        return;
    }
    /*inizializzo un nuovo blocco cche contiene il nome e la socket*/
    userDesc *new = NULL;   
    CHECK_PTR((new = calloc(1,sizeof(userDesc))),"new");
    CHECK_PTR((new->name = calloc(WORDSIZE,sizeof(char))),"new->name");
    strcpy(new->name,name);
    new->next=NULL;
    new->prev=NULL;
    new->sock = sock;
    if((*list)==NULL){
        /*il puntatore alla testa è NULL quindi questo è il primo elemento. Faccio puntare 
        * list al nuovo blocco */
        (*list)=new;   
    }
    else{
        /*la lista contiene già alatri elementi quindi la scorro fino all'ultimo di essi*/
        userDesc *corr = (*list);
        while(corr->next!=NULL){
            corr = corr->next;
        } 
        /*collego il nuovo blocco all'ultimo elemento*/
        corr->next=new;
        new->prev=corr;
    }
    /*faccio puntare last al nuovo elemento*/
    (*last)=new;
 }
 
 void freeList(userDesc **list){
     if((*list)!=NULL){
         /*se la lista non è già vuota, scorro liberando tutti gli elementi*/
         while((*list)->next){
             (*list)=(*list)->next;
             free((*list)->prev);
         }
         free(*list);
     }
 }

int receive(int socket,char **usrname){
    struct stat st = {0};
    char *income=NULL,**inMsg=NULL,*path=NULL;
    /*alloco il buffer che conterrà il path alla cartella da creare eventualmente*/
    CHECK_PTR((path=calloc(WORDSIZE,sizeof(char))),"errore durante l'allocazione di path nella recieve nel mainServer");
    /*alloco il buffer che conterrà il messaggio in entrata da parte del client*/
    CHECK_PTR((income=calloc(WORDSIZE,sizeof(char))),"errore durante l'allocazione della risposta");
    /*leggo dalla socket nel buffer income il messaggio da parte del client*/
    CHECK_OP(read(socket,income,WORDSIZE),"errore facendo la read!");
    /*lo tokenizzo e ricavo l'username dell'utente da registrare che sealvo in una variabile passata per riferimento*/
    inMsg=tokenizer(income);
    strcpy(*usrname,inMsg[1]);
    /*mi costruisco il path per la cartella*/
    strcpy(path,"./data/");
    strcat(path,inMsg[1]);
    /*controllo che non esista già una cartella con il path indicato */
    if (stat(path,&st)==-1){
        printf("la cartella non esiste\n");
        /*se non esiste provo a crearla */
        if (mkdir(path,0700)==0){
            printf("ho creato la cartella\n");
            /*la creazione è andata a buon fine quindi libero le strutture dati e restituisco false*/
            free(income);
            cleanPtr(inMsg);
            free(path);
            return 1;
        }
        else{
            /*non sono riuscito a creare la cartella, libero le strutture dati e restituisco false*/
            printf("non sono riuscito a creare la cartella\n");
            free(income);
            cleanPtr(inMsg);
            free(path);
            return 0;
        }
        
    }
    else{
        /*la cartella esiste già quindi non la creo, libero e restituisco true*/
        printf("la cartella esiste già!\n");
        free(income);
        cleanPtr(inMsg);
        free(path);
        return 1;
    }
    
}

void signalHandler(int sig){
    write(1,"[Server] Ho catturato un nuovo segnale\n",39);
    if (sig!=SIGUSR1 && (getUsr()==0)){
        /*se non ci sono più utenti nel sistema imposto terminated a 1*/
        terminated=1;
    }
    /*assegno il segnale catturato a sigFlag*/
    sigFlag=sig;
}

void signalDetector(){
    sigFlag=0;
    
    struct sigaction sg;
    memset(&sg,0,sizeof(sg));
    sg.sa_handler= signalHandler;
    if (sigaction(SIGINT,&sg,NULL)==-1){
        perror("sigaction SIGINT");
    }
    if (sigaction(SIGQUIT,&sg,NULL)==-1){
        perror("sigaction SIGQUIT");
    }
    if (sigaction(SIGTERM,&sg,NULL)==-1)
    {
        perror("sigaction SIGTERM");
    }
    if (sigaction(SIGUSR1,&sg,NULL)==-1){
        perror("sigaction SIGUSR1");
    }
    

}

void *newSocket(void *val){
    /*casto val al tipo della struct*/
    userDesc proprieties=*(userDesc*)val;
    int readSize;
    FILE *fname=NULL;
    char *clientMessage=NULL,**parameters=NULL,*response=NULL,*path=NULL;
    /*allocoil buffer che conterrà i messaggi in entrata da parte del buffer*/
    CHECK_PTR((clientMessage=calloc(WORDSIZE+1,sizeof(char))),"errore nell'allocazione della stringa clientMessage");
    /*alloco il buffer che conterrà il path alla cartella su cui effettuare le operazioni*/
    CHECK_PTR((path=calloc(WORDSIZE,sizeof(char))),"errore nell'allocazione della stringa path nel task");
    /*alloco il buffer che conterrà l'esito delle operazioni eseguite sul server*/
    CHECK_PTR((response=calloc(WORDSIZE,sizeof(char))),"errore durante l'alloczione della stringa di response");
    /*eseguo un ulteriore memset per sicurezza*/
    memset(clientMessage,0,WORDSIZE);
    memset(path,0,WORDSIZE);
    memset(response,0,WORDSIZE);
    operation op;
    /*ciclo fino a che terminated è falso e sto effettivamente leggendo qualcosa dalla socket*/
    while (!terminated && (readSize=read(proprieties.sock,clientMessage,WORDSIZE))>0){
            /*tokenizzo la richiesta letta dalla socket*/
            parameters=tokenizer(clientMessage);
            /*converto la prima parola della richiesta in una enum*/
            op=str2enum(parameters[0]);
            /*eseguo uno switch sulla richiesta*/
            switch (op){   
            case 0:
                /*CASE 0 = STORE*/
                printf("Client %d says: %s\n",proprieties.sock,parameters[0]);
                /*costruisco il path per il documento da salvare nell'object store*/
                strcpy(path,"./data/");
                strcat(path,proprieties.name);
                strcat(path,"/");
                strcat(path,parameters[1]);
                strcat(path,".txt");
                /*inizizalizzo il descrittore del file in modalità scrittura*/
                CHECK_PTR((fname=fopen(path,"w")),"errore durante la creazione del file!");
                char *file=NULL;
                int len=atoi(parameters[2]);
                /*alloco un buffer di lunghezza len che conterrà il file da salvare*/
                CHECK_PTR((file=calloc(len+1,sizeof(char))),"errore durante l'allocazione del buffer di file nel case1")
                /*eseguo una readn sulla socket assicurandomi di leggere esattamente len caratteri dalla socket*/
                if (readn(proprieties.sock,file,len)<0){
                    /*la readn non è andata a buon fine. Imposto il messaggio di failure*/
                    perror("read del file fallita\n");
                    strcpy(response,"KO lettura da socket fallita!!");
                }
                else{
                    /*la readn è andata a buon fine. Imposto il messaggio di successo e aumento il numero di oggetti 
                    * presenti nello store*/
                   /*scrivo il contenuto del buffer nel file*/
                    if(fwrite(file,sizeof(char),len,fname)!=len){
                        perror("read del file fallita\n");
                        strcpy(response,"KO scrittura su file fallita!!");
                    }
                    else{
                        strcpy(response,"OK");
                        addObj(len);
                    }
                }
                /*chiudo il file descriptor*/
                fclose(fname);
                /*libero il buffer*/
                free(file);
                /*mando il risultato dell'operazione al client*/
                if(write(proprieties.sock,response,strlen(response))<0){
                    perror("scrittura della risposta fallita!");
                }
                /*resetto il buffer per l'esito delle operazioni*/
                memset(response,0,WORDSIZE);
                break;
            case 1:
                /*CASE 1 = RETRIEVE*/
                printf("Client %d says: %s\n",proprieties.sock,parameters[0]);
                int size=0;
                char *buff=NULL,*msg=NULL,*slen=NULL;
                /*cotruisco il path per il documento da recuperare dall'objectstore*/
                strcpy(path,"./data/");
                strcat(path,proprieties.name);
                strcat(path,"/");
                strcat(path,parameters[1]);
                strcat(path,".txt");
                /*alloco il buffer che conterrà il file letto dal path*/
                CHECK_PTR((buff=calloc(WORDSIZE,sizeof(char))),"errore nell'allocazione del buffer per l'op recieve nel server!");
                /*alloco il buffer che conterra il file da inviare al client dopo la lettura dal file descriptor*/
                CHECK_PTR((msg=calloc(1,sizeof(char))),"errore nell'allocazione del buffer del messaggio nella recieve");
                /*istanzio il file descriptor in modalità lettura*/
                fname=fopen(path,"r");
                if (fname!=NULL){
                    /*l'operazio di open è andata abuon fine inizio a leggere dal file*/
                    while (fread(buff,sizeof(char),WORDSIZE-1,fname)){
                        /*aggiorno la size aggiungendo al dimensione di quello che ho letto dal file*/
                        size+=strlen(buff);
                        /*rialloco il il buffer con la nuova dimensione*/
                        CHECK_PTR((msg=realloc(msg,(size+1)*sizeof(char))),"errore nella riallocazione della stringa del messaggio!");
                        /*inserisco il nuovo pezzo di file letto*/
                        strcat(msg,buff);
                        /*resetto il buffer di lettura dal file */
                        memset(buff,0,WORDSIZE);
                    }
                    /*alloco il buffer che conterrà la dimensione del file sotto forma di stringa*/
                    CHECK_PTR((slen=calloc(WORDSIZE,sizeof(char))),"errore nell'allocazione della stringa len nel case 2!");
                    /*converto la len in stringa*/
                    sprintf(slen,"%ld",strlen(msg));
                    /*costruisco il messaggio da inviare al client*/
                    strcpy(response,"DATA ");
                    strcat(response,slen);
                    /*invio il messaggio preliminare al client*/
                    if(write(proprieties.sock,response,WORDSIZE)<0){
                        perror("write della risposta al client fallita nel case 2!");
                    }
                    /*invio il file con la writen di modo da essere sicuro 
                    * scrivere un numero di caratteri esattamente uguale alla lunghezza*/
                    if(writen(proprieties.sock,msg,strlen(msg))<=0){
                        perror("write verso il client fallita!\n");
                    }
                    /*libero le strutture dati*/
                        free(slen);
                        fclose(fname);  
                    }
                else{
                    /*la fopen non è andata buon fine. Setto il messaggio di failure e lo mando al client*/
                    strcpy(response,"KO unknown file!");
                    write(proprieties.sock,response,strlen(response));
                
                }
                /*chiudo il file descriptor, libero il buffer e resetto il buffer che contiene l'esito
                * dell'operazioni*/
                free(msg);
                free(buff);
                memset(response,0,WORDSIZE);
                break;
            case 2:
                /*CASE 2 = DELETE*/
                printf("Client %d says: %s\n",proprieties.sock,parameters[0]);
                /*costruisco il path che mi condurrà al file da eliminare*/
                strcpy(path,"./data/");
                strcat(path,proprieties.name);
                strcat(path,"/");
                strcat(path,parameters[1]);
                strcat(path,".txt");
                FILE *fname=NULL;
                int sz;
                /*inizializzo il file descriptor in modalità lettura*/
                fname=fopen(path,"r");
                if(fname!=NULL){
                     fseek(fname,0L,SEEK_END);
                    /*mi ricavo la lunghezza del file*/
                    sz=ftell(fname);
                    if (remove(path)==0){
                        /*la remove è stata eseguita con successo, setto il messaggio di successo
                        * diminuisco il contatore degli oggetti presenti nell'objectstore*/
                        strcpy(response,"OK");
                        remObj(sz);
                    }
                    else{
                    /*la remove non è andata a buon fine, setto il messaggio di failure*/
                    strcpy(response,"KO delete fallita!");
                    }
                    fclose(fname);
                }
                else{
                    strcpy(response,"KO file non esistente!");
                }
                /*mando al client il responso dell'operazione eseguita*/
                if(write(proprieties.sock,response,strlen(response))<0){
                    perror("write dell'esito della delete sulla socket fallita!");
                }
                /*resetto il buffer per il responso delle operazioni e chiudo il file descriptor*/
                memset(response,0,WORDSIZE);
                
                break;
            case 3:
                /*CASE 3 = LEAVE*/
                printf("Client %d says: %s\n",proprieties.sock,parameters[0]);
                /*imposto il messggio di response*/
                strcpy(response,"OK\n");
                /*mando il messaggio di response attraverso la socket*/
                CHECK_OP(write(proprieties.sock,response,strlen(response)),"Error during the writing of the response message!\n");
                /*libero le strutture dati*/
                free(response);
                cleanPtr(parameters);
                free(path);
                free(clientMessage);
                free(proprieties.name);
                printf("[THREAD %ld]Client %d disconnected!\n",pthread_self(),proprieties.sock);
                /*riduco il numero degli utenti presenti nel sistema*/
                remOnlineUsr();
                /*chiudo la socket*/
                close(proprieties.sock);
                return NULL;
                break;
            default:
                break;
            }
            /*resetto i buffer e svuoti il buffer che contiene la richiesta tokenizzata */
            memset(clientMessage,0,WORDSIZE);
            memset(response,0,WORDSIZE);
            memset(path,0,WORDSIZE);
            cleanPtr(parameters);
    }
    if (readSize==0){
            /*ho effettuato una lettura nulla*/
            printf("[THREAD %ld]Client %d disconnected!\n",pthread_self(),proprieties.sock);
            /*rimuovo l'utente e chiudo la socket*/
            remOnlineUsr();
            close(proprieties.sock);
            fflush(stdout);
    }
    else if(readSize==-1){
        /* il client è crashato, rimuovo l'utente*/
        remOnlineUsr();
        printf("[THREAD %ld]Client %d crashed!\n",pthread_self(), proprieties.sock);
    }
    else if(terminated){
        /*non ci sono più client online, chiudo la socket*/
        close(proprieties.sock);
    }
    /*libero le strutture dati*/
    free(proprieties.name);
    free(response);
    free(path);
    free(clientMessage);
    return NULL;
}