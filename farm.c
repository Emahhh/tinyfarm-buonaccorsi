#include "xerrori.h"
#define QUI __LINE__,__FILE__
#define HOST "127.0.0.1"
#define PORT 59885

bool debugPrint = false;
volatile sig_atomic_t interrompi = false;



// stampa il corretto utilizzo della chiamata da linea di comando - per quando l'utente chiama il programma in modo errato
void printUso(char* argv[]){
  fprintf(stderr, "Uso: %s file [file ...] [-n nthread>0] [-q qlen>0] [-t delay>=0]\n", argv[0]);
  return;
}



// struct contenente i parametri di input di ogni thread
typedef struct dati {  
  int* cindex;  // indice nel buffer per i consumatori
  char** buffer; 
  int qlen;

	pthread_mutex_t *cmutex;
  sem_t *sem_free_slots;
  sem_t *sem_data_items;

} dati;



// handler per il segnale SIGINT
void handler(int s){
  printf("Segnale %d ricevuto dal processo principale %d. Interrompo.\n", s, getpid());
  interrompi = true;
  return;
}



// funzione usata dai thread per mandare SOMMA e FILENAME al server di stampa
void mandaServer(long sum, char* filename){
  int fd_skt = 0;      // file descriptor associato al socket
  struct sockaddr_in serv_addr;
  size_t e;
  long tmp;
  
  // creazione socket -----------------------------------------
  if ((fd_skt = socket(AF_INET, SOCK_STREAM, 0)) < 0) termina("Errore creazione socket"); // crea socket

  serv_addr.sin_family = AF_INET; // assegna indirizzo
  serv_addr.sin_port = htons(PORT); // il numero della porta deve essere convertito in network order
  serv_addr.sin_addr.s_addr = inet_addr(HOST);

  if (  connect(fd_skt, (struct sockaddr*) &serv_addr, sizeof(serv_addr))   < 0 ) termina("Errore apertura connessione"); // apre connessione
  
  // connesso: invio i dati nell'ordine prestabilito -------------------------------------------------------
  // mando lunghezza del nomefile
  if(debugPrint) printf("invio filename %s di lunghezza %ld caratteri\n", filename, strlen(filename));
  tmp = htobe64(strlen(filename));
  e = writen(fd_skt, &tmp, sizeof(tmp));
  if(e!=sizeof(tmp)) termina("Errore write lunghezza nome file");

  // mando il nome del file
  e = writen(fd_skt, filename, strlen(filename));
  if(e!=strlen(filename)) termina("Errore write nome file");

  // mando il risultato (somma)
  if(debugPrint) printf("invio somma %ld in %ld bytes\n", sum, sizeof(sum));
  tmp = htobe64(sum); // convertiamo da hardware a network order (big endian)
  e = writen(fd_skt, &tmp, sizeof(tmp));
  if(e!=sizeof(tmp)) termina("Errore write somma");

  // chiudo la connessione
  if(close(fd_skt)<0) perror("Errore chiusura socket");
  if(debugPrint) puts("Connessione terminata"); 
  return;
}



// si collega con il server per mandargli uno speciale messaggio di terminazione prestabilito
void terminaServer(){
  if(debugPrint) puts("Chiedo al server di terminare.");
  mandaServer(0, "TerminaServer!!!?");
  return;
}



// codice eseguito da ogni thread worker
void *tbody(void *arg){  
  dati *myDati = (dati *)arg; 
  int numfile = 0; // numero di file elaborati dal thread - per debugging

  while(true) 
    { // prendi file da buffer, aprilo, scorri tutto ottenendo somma e manda risultato a server
    char* file;

    // prendo un nomefile dal buffer
    xsem_wait(myDati->sem_data_items,QUI);
		xpthread_mutex_lock(myDati->cmutex,QUI); // i semafori possono anche far passare due thread consumatori - serve mutua esclusione per le due op. successive

    file = myDati->buffer[*(myDati->cindex) % myDati->qlen];
    *(myDati->cindex) +=1;
    
		xpthread_mutex_unlock(myDati->cmutex,QUI);
    xsem_post(myDati->sem_free_slots,QUI); // fine accesso al buffer ---

    if(strcmp(file, "TerminaNonHoPiuFile!!!?") == 0) break; // se ho preso il segnale di terminazione, termino

    // apro file e calcolo somma -------
    FILE *f = fopen(file, "r");
    if(f==NULL) {
      fprintf(stderr, "Il thread %ld non è riuscito ad aprire il file chiamato %s.\n", pthread_self(), file); 
      perror("Errore");
      continue;
    }
    numfile++;

    long sum = 0;
    long n;
    int i = 0;
    while(true) {
      int e = fread(&n, sizeof(long), 1, f);
      if(e!=1) break; // se il valore e' letto correttamente e==1
      sum += n*i;
      i++;
    }

    if (fclose(f) != 0) fprintf(stderr, "Errore chiusura file");
    if(debugPrint) printf("thread %ld ha calcolato la somma del file %s: %ld. Provvedo a comunicarlo al server...\n", pthread_self(), file, sum);

    mandaServer(sum, file); // mando risultato connettendomi al server
  }

  if(debugPrint) printf("il thread %ld è terminato dopo aver elaborato %d file.\n", pthread_self(), numfile);
  pthread_exit(NULL); 
} 



int main(int argc, char *argv[])
{
  // ottengo i valori dei parametri opzionali -----------------------
  if(argc<2) {
      printUso(argv);
      return 1;
  }

  // valori di default
  int nthread = 4;
  int qlen = 8;
  int delay = 0;

  int numopt = 0; // numero di opzioni
  int opt = 0;

  while((opt = getopt(argc, argv, "n:q:t:d")) != -1) {
    switch(opt) {
      case 'n':
        nthread = atoi(optarg);
        numopt+=2;
        break;
      case 'q':
        qlen = atoi(optarg);
        numopt+=2;
        break;
      case 't':
        delay = atoi(optarg);
        numopt+=2;
        break;
      case 'd':
        debugPrint = true;
        numopt++;
        break;
      default:
        printUso(argv);
        return 1;
    }
  }

  if(nthread<=0 || qlen<=0 || delay<0) {
    printUso(argv);
    return 1;
  }


  // gestione segnali ------------------------------------------------
  struct sigaction act;
  act.sa_handler = &handler;
  sigemptyset(&act.sa_mask); // pone la maschera a insieme vuoto
  act.sa_flags = SA_RESTART;
  sigaction(SIGINT, &act, NULL); // setto gestione di SIGINT


  // inizializzo buffer e threads -------------------------
  char* buffer[qlen];
  int pindex=0, cindex=0; // cindex è contesa da tutti i thread

  // avvio nthread thread consumatori
  pthread_mutex_t cmutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_t t[nthread];
  
  sem_t sem_free_slots, sem_data_items;
  xsem_init(&sem_free_slots, 0, qlen, QUI);
  xsem_init(&sem_data_items, 0, 0, QUI);

  dati datiWorkers;
  datiWorkers.buffer = buffer;	
  datiWorkers.cindex = &cindex;
  datiWorkers.qlen = qlen;
  datiWorkers.cmutex = &cmutex;
  datiWorkers.sem_data_items = &sem_data_items;
  datiWorkers.sem_free_slots = &sem_free_slots;

  for(int i=0; i<nthread; i++) {
    xpthread_create(&t[i], NULL, tbody, &datiWorkers, QUI);
  }


  // mando nomi dei file nel buffer ------------------------
  for(int i=numopt+1; i<argc; i++) {   
    // salto i primi elementi di argv, osservando che getopt permuta le opzioni all'inizio di argv
    // argv[i] contiene i nomi dei file (se utente ha dato input giusto)

    if(interrompi) break; // ho ricevuto SIGINT, quindi smetto di inserire file nel buffer. I thread consmeranno i file rimasti, per poi terminare grazie alle operationi di terminazione.

    if(strlen(argv[i])>255){
      printf("Il nome del file %s è troppo lungo. Lo salto.\n", argv[i]);
      usleep(delay*1000); // 1 ms = 1000 us
      continue;
    }
    xsem_wait(&sem_free_slots,QUI);
    buffer[pindex++ % qlen] = argv[i];
    xsem_post(&sem_data_items,QUI);
    usleep(delay*1000);
  }

  // terminazione threads ----------------
  for(int i=0;i<nthread;i++) {
    // metto un messaggio di terminazione nel buffer per ogni thread
    xsem_wait(&sem_free_slots,__LINE__,__FILE__);
    buffer[pindex++ % qlen] = "TerminaNonHoPiuFile!!!?";  // stringa usata come segnale di terminazione (nessun file può avere questo nome, e non si può nemmeno inserire da linea di comando a causa dei caratteri speciali)
    xsem_post(&sem_data_items,__LINE__,__FILE__);
  }

  for(int i=0; i<nthread; i++) {
    xpthread_join(t[i], NULL, QUI);
  }

  terminaServer(); // manda messaggio di terminazione al server

  printf("Tutti i thread sono terminati dopo aver esaminato tutti i file leggibili. Termino MasterWorker. \n");
	return 0;
}