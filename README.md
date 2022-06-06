# Tiny Farm - progetto Laboratorio II

> Emanuele Buonaccorsi - 598855 - corso B

## Il server Python

Si è scelto di creare un server **basato sui thread**.

Ad ogni thread viene mandato un singolo file. Il thread si occupa della connessione e della stampa, per poi terminare.

Se il thread riceve uno speciale messaggio di terminazione inviatogli dal client, fa terminare l'intero programma.

## Comunicazione client-server

È stato richiesto, per ogni file, di fare in modo che il server Python stampi il nome del file e la corrispettiva somma.

Per fare ciò si è scelto di stabilire un protocollo di comunicazione tra client e server che prevede di mandare, in ordine:

- la **lunghezza** del nome del file
- il **nome del file**
- la **somma** calcolata dal thread worker

In questo modo il server può sapere quanti byte aspettarsi per la seconda comunicazione, cioè il nome del file.

## Parametro opzionale `-d` (DebugPrint)

È stato aggiunto un parametro opzionale alla linea di comando. Aggiungendo `-d` alla chiamata viene modificata la variabile globale `debugPrint` a `true`, abilitando così la stampa di alcune stringhe poste in posizioni specifiche del programma per facilitarne il debug.

Di default il parametro è posto a `false`, stampando solo le stringhe richieste.

## Gestione di `SIGINT`

> Alla ricezione di tale segnale il processo deve completare i task eventualmente presenti nel buffer produttori/consumatori e terminare [...]

Vista la richiesta di completare i task presenti nel buffer, è stato scelto di gestire il segnale in questo modo:

l'handler del segnale imposta la variabile globale `interrompi` a `true`. Prima di inserire il nome di un file nel buffer viene controllato questo valore booleano. Se ho ricevuto un segnale uscirò dal ciclo e smetterò così di inserire ulteriori file nel buffer. Passerò al chiedere ai thread di terminare permettendogli prima di completare i task già presenti. Stessa cosa per il server.

## Alcuni esempi di comandi di test

Con alcuni nomi di file non esistenti:
`./farm file file2 file3 z0.dat dsdgfsd sgsg z-868867545.dat sd file4 file5 -t 500`

Tutti i file di test:
`./farm z*?.dat -t 500`
