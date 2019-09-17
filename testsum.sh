#!/bin/bash

function Rep () {
    # Numero totale di test per la batteria
    total=$(grep -c "Test $1" testout.log)
    # Numero di test finiti con successo
    let successfull=$(grep -c "Test $1 passed!" testout.log)
    # Numero di test falliti
    let failed=$total-$successfull
    # Percentuale di test finiti con successo
    let pPerc=($successfull/$total)*100
    #Percentuale di test falliti
    let fPerc=($failed/$total)*100
    # Stampa le informazioni per il test in questione
    echo "Test $1: Lanciati $total, Passati $successfull ($pPerc%), Falliti $failed ($fPerc%)"
}

# Numero totale di test effettuati
total=$(grep -c "Test $1" testout.log)
echo "Test lanciati: $total"
# Stampa il report per ogni batteria
for ((i = 1; i <= 3; i++)); do
    Rep $i
done

# Manda un segnale per effettuare la diagnostica dell'objectstore
kill -USR1 $(pidof ./mainServer)
# Termina il server
kill -TERM $(pidof ./mainServer)