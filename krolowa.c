#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>

int main(int argc, char* argv[])
{
    if (argc < 5) 
    {
        fprintf(stderr, "[KRÓLOWA] Sposób użycia: %s <sem_id> <fd_pipe> <shm_id>\n", argv[0]);
        return 1;
    }

    srand(time(NULL));

    int sem_id = atoi(argv[1]);
    //int pipe_fd = atoi(argv[2]);  
    int shm_id = atoi(argv[3]);
    int msqid = atoi(argv[4]);
    int pojemnosc_poczatkowa;

    Stan_Ula* stan_ula = (Stan_Ula*) shmat(shm_id, NULL, 0);
    if (stan_ula == (void*) -1) {
        perror("[KRÓLOWA] shmat");
        return 1;
    }

    printf("[KROLOWA] Obecna - %d\n Obecna ul - %d\n, Maks - %d\n, Stan poczatkowy - %d\n", stan_ula->obecna_liczba_pszczol, stan_ula->obecna_liczba_pszczol_ul, stan_ula->maksymalna_ilosc_osobnikow, stan_ula->stan_poczatkowy);


    if (semop(sem_id, &lock, 1) == -1){
        perror("[Krolowa] semop lock (odczyt)");
    }

    pojemnosc_poczatkowa = stan_ula->stan_poczatkowy / 2;

    if (semop(sem_id, &unlock, 1) == -1) {
        perror("[Krolowa] semop unlock (odczyt)");
    }

    int obecna_liczba_pszczol;
    int maksymalna_ilosc_osobnikow;
    int obecna_liczba_osobnikow_ul;
    int depopulacja_flaga;
    msgbuf zlozona_ilosc_jaj;
    zlozona_ilosc_jaj.mtype = MSG_TYPE_EGGS;

    while (1) {        
        if (semop(sem_id, &lock, 1) == -1) {
            perror("[Krolowa] semop lock (odczyt)");
            break;
        }

        obecna_liczba_pszczol = stan_ula->obecna_liczba_pszczol;
        maksymalna_ilosc_osobnikow = stan_ula->maksymalna_ilosc_osobnikow;
        obecna_liczba_osobnikow_ul = stan_ula->obecna_liczba_pszczol_ul;
        depopulacja_flaga = stan_ula->depopulacja_flaga;

        if (semop(sem_id, &unlock, 1) == -1) {
            perror("[Krolowa] semop unlock (odczyt)");
            break;
        }

        int liczba_zlozonych_jaj = rand() % 10 + 3;

        /*
        int ograniczenie_ula = pojemnosc_poczatkowa - obecna_liczba_osobnikow_ul;
        int ograniczenie_populacji = maksymalna_ilosc_osobnikow - obecna_liczba_pszczol;
        //printf("\033[1;37m[KROLOWA]  obecna l.pszczol - %d\n, liczba w ulu - %d\n maks pojemnosc ula : %d\033[0m\n Potencjalny limit 1 - %d\noraz 2 - %d\n",obecna_liczba_pszczol, obecna_liczba_osobnikow_ul, maksymalna_ilosc_osobnikow, potencjalny_limit_jaj_1, potencjalny_limit_jaj_2);


        if (ograniczenie_ula < ograniczenie_populacji) 
        {
            liczba_zlozonych_jaj = rand() % ograniczenie_ula + 1;
        } 
        else if (ograniczenie_populacji < ograniczenie_ula)
        {
            liczba_zlozonych_jaj = rand() % ograniczenie_populacji + 1;
        } 
        else 
        {
            liczba_zlozonych_jaj = rand() % 3 + 1; // Brak miejsca na jaja
        }
        */
        //printf("\033[5;34mPszczola wyliczyla potencjalnie %d\033[0m\n", liczba_zlozonych_jaj);
       // printf("[KROLOWA]  obecna l.pszczol - %d\n, liczba w ulu - %d\n maks pojemnosc ula : %d\n",obecna_liczba_pszczol, obecna_liczba_osobnikow_ul, maksymalna_ilosc_osobnikow);
       // printf("Krolowa znioslby %d jaj, ale flaga depopulacja = %d\n", liczba_zlozonych_jaj, depopulacja_flaga);

    /*
       if(depopulacja_flaga == 1)
       {
            printf("Depopulacja jest przeprowadzana! Nie moge znosic jaj!\n");
       }
       */

        if (obecna_liczba_pszczol + liczba_zlozonych_jaj <= maksymalna_ilosc_osobnikow 
             && obecna_liczba_osobnikow_ul + liczba_zlozonych_jaj <= pojemnosc_poczatkowa
             && depopulacja_flaga  != 1) 
        {
            if (semop(sem_id, &lock, 1) == -1) {
            perror("[Krolowa] semop lock (odczyt)");
            break;
            }

            stan_ula->obecna_liczba_pszczol_ul += liczba_zlozonych_jaj;
            stan_ula->obecna_liczba_pszczol += liczba_zlozonych_jaj;

            if (semop(sem_id, &unlock, 1) == -1) {
            perror("[Krolowa] semop lock (odczyt)");
            break;
            }

            zlozona_ilosc_jaj.eggs = liczba_zlozonych_jaj;

            if (msgsnd(msqid, &zlozona_ilosc_jaj, sizeof(int), 0) == -1) 
            {
            perror("[KROLOWA] msgsnd error");
            }

            /*
            if (write(pipe_fd, &liczba_zlozonych_jaj, sizeof(int)) == -1) {
                    perror("[KROLOWA] zapisuje - pipe error");
                    break;
            }
            */
        }
        sleep(1);
    }   

    
    if (shmdt(stan_ula) == -1) {
        perror("[KRÓLOWA] shmdt");
    }
    
    printf("[KRÓLOWA] Kończę pracę.\n");

    return 0;
}