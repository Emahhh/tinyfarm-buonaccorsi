#include "xerrori.h"
#define QUI __LINE__,__FILE__

typedef struct { // struct contenente i parametri di input di ogni thread 
  int* cindex;  // indice nel buffer
  char** buffer; 
  int qlen;

	pthread_mutex_t *cmutex;
  sem_t *sem_free_slots;
  sem_t *sem_data_items;

} dati;


// thread worker
void *tbody(void *arg){  
  dati *myDati = (dati *)arg; 
  int numfile = 0; // numero di file elaborati dal thread - per debugging
 // printf("avvio il thread %ld!\n", pthread_self());

  while(true) { // prendi file da buffer, aprilo, scorri tutto ottenendo somma e manda risultato a server
    char* file;
    xsem_wait(myDati->sem_data_items,QUI);
		xpthread_mutex_lock(myDati->cmutex,QUI); // i semafori possono anche far passare due consumatori - mi serve mutua esclusione per le due op. successive

    file = strdup(myDati->buffer[*(myDati->cindex) % myDati->qlen]);
    *(myDati->cindex) +=1;
    // printf("thread %ld ha preso dal buffer il nome file %s\n", pthread_self(), file);
    
		xpthread_mutex_unlock(myDati->cmutex,QUI);
    xsem_post(myDati->sem_free_slots,QUI); // fine accesso al buffer ---

    if(strcmp(file, "TerminaNonHoPiuFile!!!?") == 0){  // segnale che mi indica di terminare
      free(file);
      break; // esco dal while e termino thread
    }

    // risolvi il problema
    FILE *f = fopen(file, "r");
    
    if(f==NULL) {
      printf("Il thread %ld non è riuscito ad aprire il file chiamato %s. ", pthread_self(), file); 
      free(file); 
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
      //printf("letto il numero %ld\n", n*i);
      sum += n*i;
      i++;
    }

    fclose(f);
    printf("thread %ld ha calcolato la somma del file %s: %ld\n", pthread_self(), file, sum);
    free(file);
  }

  // free(file); // necessaria se il thread non ha eseguito file, può essere superflua altrimenti
  printf("il thread %ld è terminato dopo aver elaborato %d file.\n", pthread_self(), numfile);
  pthread_exit(NULL); 
} 

int main(int argc, char *argv[])
{
  // controlla numero argomenti
  if(argc<2) {
      printf("Uso: %s file [file ...] [-n nthread] [-q qlen] [-t delay]\n",argv[0]);
      return 1;
  }

  // valori di default
  int nthread = 4;
  int qlen = 8;
  int delay = 0;

  int numopt = 0; // numero di opzioni
  int opt = 0;
  while((opt = getopt(argc, argv, "n:q:t:")) != -1) {
    switch(opt) {
      case 'n':
        nthread = atoi(optarg);
        numopt++;
        break;
      case 'q':
        qlen = atoi(optarg);
        numopt++;
        break;
      case 't':
        delay = atoi(optarg);
        numopt++;
        break;
      default:
        fprintf(stderr, "Uso: %s file [file ...] [-n nthread] [-q qlen] [-t delay]\n",argv[0]);
        return 1;
    }
  }

  if(nthread<=0 || qlen<=0 || delay<0) {
    fprintf(stderr, "Uso: %s file [file ...] [-n nthread>0] [-q qlen>0] [-t delay>=0]\n",argv[0]);
    return 1;
  }





  // inizializzo buffer e threads
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

  // devo mandare i nomi dei file nel buffer 
  // ottengo nomi file, ricordando che getopt permuta le opzioni all'inizio di argv
/*   // print all argv
  printf("argv è:\n");
  for(int i=0; i<argc; i++) {
    printf("%s\n", argv[i]);
  } */
  for(int i=numopt*2+1; i<argc; i++) {
    // argv[i] contiene il nome del file (se utente ha dato input giusto)
    xsem_wait(&sem_free_slots,QUI);
    buffer[pindex++ % qlen] = argv[i];
    xsem_post(&sem_data_items,QUI);
    // printf("messo nel buffer %s.\n",argv[i]);
  }

  // terminazione threads
  for(int i=0;i<nthread;i++) {
    xsem_wait(&sem_free_slots,__LINE__,__FILE__);
    buffer[pindex++ % qlen]= "TerminaNonHoPiuFile!!!?";  // "TerminaNonHoPiuFile!!!?" segnale di terminazione - nessun file può avere questo nome, e non si può nemmeno inserire da linea di comando, quindi è escluso che possa venire usato da un utente ignaro
    xsem_post(&sem_data_items,__LINE__,__FILE__);
  }

  // posso evitare di fare join?
  for(int i=0; i<nthread; i++) {
    xpthread_join(t[i],NULL, QUI);
  }

	return 0;
}