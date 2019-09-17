#!/bin/bash

exec &> testout.log

#faccio partire 50 client contemporaneamente eseguendo STORE e attendo che terminimo la loro esecuzione
for ((i = 0; i < 50; i++)); do
    ./mainClient utente$i 1 &
done
wait
# Fa partire un'altra volta i 50 clients per i test di RETRIEVE e DELETE e attendo che terminino la loro esecuzione
for ((i = 0; i < 30; i++)); do
    ./mainClient utente$i 2 &
done
for ((i = 30; i < 50; i++)); do
    ./mainClient utente$i 3 &
done
wait