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

  // ottengo nomi file, ricordando che getopt permuta le opzioni all'inizio di argv
  for(int i=numopt*2+1; i<argc; i++) {
    // argv[i] contiene il nome del file
    printf("%s\n",argv[i]);


  }

	return 0;
}