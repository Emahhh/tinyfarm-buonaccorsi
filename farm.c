#define _GNU_SOURCE   // permette di usare estensioni GNU
#include <stdio.h>    // permette di usare scanf printf etc ...
#include <stdlib.h>   // conversioni stringa exit() etc ...
#include <stdbool.h>  // gestisce tipo bool
#include <assert.h>   // permette di usare la funzione ass
#include <string.h>   // funzioni per stringhe
#include <errno.h>    // richiesto per usare errno
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

// struct contenente i parametri di input e output di ogni thread 
typedef struct {
  int n_thread;

  int* cindex;  // indice nel buffer
  long* buffer; 

	pthread_mutex_t *cmutex;
  sem_t *sem_free_slots;
  sem_t *sem_data_items;

} dati;

void *tbody(void *arg)
{  
  dati *a = (dati *)arg; 
  

  do {
/*     xsem_wait(a->sem_data_items,QUI);
		xpthread_mutex_lock(a->cmutex,QUI);

    n = a->buffer[*(a->cindex) % Buf_size];
    *(a->cindex) +=1;
    // printf("thread %ld ha letto n = %ld\n", pthread_self(), n);
    
		xpthread_mutex_unlock(a->cmutex,QUI);
    xsem_post(a->sem_free_slots,QUI);

    if(n>0 && collatz(n)>a->tbestcollatz) {
      a->tbestcollatz = collatz(n);
      a->tbestn = n;
      // printf("curr tbestcollatz=%ld \n",a->tbestcollatz);
    } */
  } while(n!= 0); // terminazione se trovo 0!

  // printf("questo thread è terminato con tbestcollatz=%ld \n",a->tbestcollatz);
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

  

  long buffer[qlen];
  int pindex=0, cindex=0;

  // avvio nthread thread consumatori
  pthread_mutex_t cmutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_t t[nthread];
  dati a[nthread];

  sem_t sem_free_slots, sem_data_items;

  xsem_init(&sem_free_slots,0,qlen,QUI);
  xsem_init(&sem_data_items,0,0,QUI);

 for(int i=0; i<nthread; i++) { // faccio partire thread consumatori
    a[i].n_thread = i;

    a[i].buffer = buffer;
		a[i].cindex = &cindex;
		a[i].cmutex = &cmutex;
    a[i].sem_data_items = &sem_data_items;
    a[i].sem_free_slots = &sem_free_slots;
    xpthread_create(&t[i], NULL, tbody ,a+i, QUI);
  }

  // devo mandare i nomi dei file nel buffer 
  // ottengo nomi file, ricordando che getopt permuta le opzioni all'inizio di argv
  for(int i=numopt*2+1; i<argc; i++) {
    // argv[i] contiene il nome del file
    printf("%s\n",argv[i]);

  }

	return 0;
}