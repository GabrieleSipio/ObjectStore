#include "check.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

char **tokenizer(char *str){
    int index=0;
    char **tokenizedString=NULL,*token,*strTmp;
    CHECK_PTR((tokenizedString=calloc(5,sizeof(char*))),"errore durante l'allocazione della risposta");
    token=__strtok_r(str," \n",&strTmp);
    while (token){
        CHECK_PTR((tokenizedString[index]=calloc((strlen(token)+1),sizeof(char))),"errore nellallocazione delle stringhe inn tokenizer");
        strcpy(tokenizedString[index],token);
        index++;
        token=__strtok_r(NULL," \n",&strTmp);
    }
    return tokenizedString;
}

operation str2enum (const char *str){
    int j;
     for (j = 0;  j < sizeof (conversion) / sizeof (conversion[0]);  ++j){
         if (!strcmp (str, conversion[j].str)){
             return conversion[j].val;
         }
     }
    return -1; 
}

void cleanPtr(char **ptr){
    int i=0;
    while (ptr[i]!=NULL){
        free(ptr[i++]);
    }
    free(ptr);
}

int readn(long fd, void *buf, size_t size) {
 size_t left = size;
 int r;
 char *bufptr = (char*)buf;
 while(left>0) {
  if ((r=read((int)fd ,bufptr,left)) == -1) {
   if (errno == EINTR) continue;
   return -1;
  }
  if (r == 0) return 0;   // fine messaggio
  left    -= r;
  bufptr  += r;
 }
 return size;
}

int writen(long fd, void *buf, size_t size) {
 size_t left = size;
 int r;
 char *bufptr = (char*)buf;
 while(left>0) {
  if ((r=write((int)fd ,bufptr,left)) == -1) {
   if (errno == EINTR) continue;
   return -1;
  }
  if (r == 0) return 0;  //non dovrebbe mai verificarsi, a meno che left non sia 0, condizione esclusa dal while.
  left    -= r;
  bufptr  += r;
 }
 return 1;
}