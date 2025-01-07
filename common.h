#ifndef COMMON_H
#define COMMON_H

#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 700
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <signal.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>


#define POCZATKOWA_ILOSC_PSZCZOL 100
#define FIFO_PATH "/tmp/ul_do_pszczelarz_fifo"

typedef struct {
    int licznik_odwiedzen;
    int liczba_cykli;
    int pszczola_jest_w_ulu;
} Pszczola;

typedef struct {
    int obecna_liczba_pszczol;
    int maksymalna_ilosc_osobnikow;
    int obecna_liczba_pszczol_ul;
    int stan_poczatkowy;
} Stan_Ula;

typedef struct {
    Pszczola* pszczola;
    int sem_id;
    Stan_Ula* stan_ula_do_przekazania;
} Argumenty_Watku;


void obsluga_sygnalu(int sig);

#endif 
