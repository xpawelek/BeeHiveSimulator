#ifndef COMMON_H
#define COMMON_H

#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 700
#define MESSAGE_DEFS_H
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
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>


#define POCZATKOWA_ILOSC_PSZCZOL 100
#define FIFO_PATH "/tmp/ul_do_pszczelarz_fifo"
#define MSG_QUEUE_PROJECT_ID 'A'       
#define MSG_TYPE_EGGS 1


typedef struct msgbuf {
    long mtype;  // musi byc typu long (SysV wymaganie)
    int eggs;    // tu przechowujemy liczbe jaj
} msgbuf;

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
    int depopulacja_flaga;
    int logi_wskaznik;
} Stan_Ula;

typedef struct {
    Pszczola* pszczola;
    int sem_id;
    Stan_Ula* stan_ula_do_przekazania;
} Argumenty_Watku;

struct sembuf lock   = {0, -1, 0};
struct sembuf unlock = {0,  1, 0};


void obsluga_sygnalu(int sig);
void obsluga_sigint(int sig);

void aktualizacja_logow(char* wiadomosc, int color, int style){
    char color_str[20]; 
    char style_str[20];   
    sprintf(color_str, "%d", color);
    sprintf(style_str, "%d", style);

    printf("\033[%s;%sm%s\033[0m\n", style_str, color_str, wiadomosc);
    FILE* plik_logi = fopen("plik_logi.txt","a");
    if (plik_logi == NULL){
        perror("Blad otwarcia pliku.");
        exit(EXIT_FAILURE);
    }
     struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t now = tv.tv_sec;
    struct tm* obecny_czas = localtime(&now);
    char czas_str[30];
    strftime(czas_str, sizeof(czas_str), "%Y-%m-%d %H:%M:%S", obecny_czas);
    snprintf(czas_str + strlen(czas_str), sizeof(czas_str) - strlen(czas_str), ".%03ld", tv.tv_usec / 1000);
    fprintf(plik_logi, "[%s] %s", czas_str, wiadomosc);
    fflush(plik_logi);
    fclose(plik_logi);
}

    struct sigaction sa;

#endif 