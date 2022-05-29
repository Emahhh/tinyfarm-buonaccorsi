# Tiny Farm - progetto Laboratorio II

### Emanuele Buonaccorsi - 598855 - corso B

## Il server Python
Si è scelto di creare un server **basato sui thread**. 

Ad ogni thread viene mandato un singolo file. Il thread si occupa della connessione e della stampa, per poi terminare.
## Comunicazione client-server
È stato richiesto, per ogni file, di fare in modo che il server Python stampi il nome del file e la corrispettiva somma.

Per fare ciò si è scelto di stabilire un protocollo di comunicazione tra client e server che prevede di mandare, in ordine:
- la **somma** calcolata dal thread worker
- la **lunghezza** del nome del file
- il **nome del file**
  
In questo modo il server può sapere quanti byte aspettarsi per la terza comunicazione, cioè il nome del file. 
## Comandi di test
Per avviare collector: `python3 collector.py`


Con nomi di file non esistenti:
`./farm file file2 file3 z0.dat dsdgfsd sgsg z-868867545.dat sd file4 file5 -n 20 -q 100 -t 6`

Tutti i file di tes:
`./farm z*?.dat -n 20 -q 1 -t 6`
