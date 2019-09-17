# ObjectStore
Object Store - Laboratorio di Sistemi Operativi, Corso B Progetto di fine corso A.A. 2018/19
Overview
Lo scopo del progetto è quello di realizzare un object store implementato come sistema client-server, destinato a supportare le richieste di memorizzare e recuperare blocchi di dati da parte di un gran numero di applicazioni. Nel caso specifico, la connessione fra clienti e object store avviene attraverso socket su dominio locale. In particolare, il progetto consta dei seguenti file:
- mainServer.c: qui sono gestite le azioni lato server che spaziano dalla memorizzazione, all’invio di file verso un determinato client fino all’eliminazione di un determinato file
- mainClient.c: qui sono gestite le varie operazioni da richiedere all’object store che sono:
o REGISTER: Registra un utente all’object store, instaurando una connessione con esso
o STORE: Memorizza un file all’interno dell’object store
o RETRIEVE: Recupera un oggetto all’interno dell’object store
o DELETE: Elimina un file all’interno dell’object store
o LEAVE: Chiude al connessione all’object store
Oltre ciò sono presenti anche due repositories:
- data: folder che contiene al suo interno cartelle con i nomi degli utenti che sono registrati all’object store che a loro volta contengono i file da essi memorizzati. Funge da database per i documenti
- Utilities: folder che contiene al suo interno numerosi file che svolgono funzione di libreria per server e client:
o check.h
o check.c
o objstore.c
o objstore.h
o rep.h
o stat.h
