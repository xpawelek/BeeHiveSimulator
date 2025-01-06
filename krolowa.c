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
    if (argc < 4) {
        fprintf(stderr, "[KRÓLOWA] Sposób użycia: %s <sem_id> <fd_pipe> <shm_id>\n", argv[0]);
        return 1;
    }

    int sem_id = atoi(argv[1]);
    int pipe_fd = atoi(argv[2]);  
    int shm_id = atoi(argv[3]);
    int pojemnosc_poczatkowa;
    printf("[KROLOWA] Otrzymane sem_id: %d, fd_pipe: %d, shm_id: %d\n", sem_id, pipe_fd, shm_id);
    srand(time(NULL));
    sleep(3);

    
    Stan_Ula* stan_ula = (Stan_Ula*) shmat(shm_id, NULL, 0);
    if (stan_ula == (void*) -1) {
        perror("[KRÓLOWA] shmat");
        return 1;
    }

    struct sembuf lock   = {0, -1, 0};
    struct sembuf unlock = {0,  1, 0};

    if (semop(sem_id, &lock, 1) == -1)
    {
        perror("[Krolowa] semop lock (odczyt)");
    }
    pojemnosc_poczatkowa = stan_ula->stan_poczatkowy / 2;
    if (semop(sem_id, &unlock, 1) == -1) 
    {
        perror("[Krolowa] semop unlock (odczyt)");
    }

   // printf("[KRÓLOWA] Start (PID=%d). Będę składać jaja!\n", getpid());
    int obecna_liczba_pszczol;
    int maksymalna_ilosc_osobnikow;
    int liczba_w_ulu;
    

    while (1) {
        // Odczytujemy stan ula
        
        if (semop(sem_id, &lock, 1) == -1) {
            perror("[Krolowa] semop lock (odczyt)");
            break;
        }
        obecna_liczba_pszczol      = stan_ula->obecna_liczba_pszczol;
        maksymalna_ilosc_osobnikow = stan_ula->maksymalna_ilosc_osobnikow;
        liczba_w_ulu               = stan_ula->obecna_liczba_pszczol_ul;
        if (semop(sem_id, &unlock, 1) == -1) {
            perror("[Krolowa] semop unlock (odczyt)");
            break;
        }

       // printf("[KRÓLOWA] Odczyt: obecna=%d, w_ulu=%d, max=%d\n",
               //obecna_liczba_pszczol, liczba_w_ulu, maksymalna_ilosc_osobnikow);
        


        // proces wysylania jaj do ula - potencjalych do zlozenia
        printf("[KROLOWA] liczba w ulu: %d, maksymalna l.osobnikow: %d, obecna l.pszczol: %d\n",liczba_w_ulu, maksymalna_ilosc_osobnikow, obecna_liczba_pszczol);
        int liczba_zlozonych_jaj;
        int potencjalny_limit_jaj_1 = pojemnosc_poczatkowa - liczba_w_ulu;
        int potencjalny_limit_jaj_2 = maksymalna_ilosc_osobnikow - obecna_liczba_pszczol;

        if (potencjalny_limit_jaj_1 > 0 && potencjalny_limit_jaj_1 < potencjalny_limit_jaj_2) {
            liczba_zlozonych_jaj = rand() % potencjalny_limit_jaj_1 + 1;
        } else if (potencjalny_limit_jaj_2 < potencjalny_limit_jaj_1) {
            liczba_zlozonych_jaj = rand() % potencjalny_limit_jaj_2 + 1;
        } else {
            liczba_zlozonych_jaj = rand() % 10; // Brak miejsca na jaja
        }

        if (write(pipe_fd, &liczba_zlozonych_jaj, sizeof(int)) == -1) {
                perror("[KROLOWA] zapisuje - pipe error");
                break;
        }
        
        printf("[KROLOWA] Probuje zniesc %d jaj\n", liczba_zlozonych_jaj);

        // Sprawdzamy, czy możemy jeszcze złożyć jaja
        /*
        if ((liczba_pszczol_ul + liczba_zlozonych_jaj <= (POCZATKOWA_ILOSC_PSZCZOL / 2)) &&
            (obecna_liczba_pszczol + liczba_zlozonych_jaj <= maks_ilosc))
        {
            // Wysyłamy do ula
            if (write(pipe_fd, &liczba_zlozonych_jaj, sizeof(int)) == -1) {
                perror("[KRÓLOWA] write potok");
                break;
            }
            printf("[KRÓLOWA] Zniosłam %d jaj\n", liczba_zlozonych_jaj);
        }
        else {
            // Wysyłamy zero jaj
            liczba_zlozonych_jaj = 0;
            if (write(pipe_fd, &liczba_zlozonych_jaj, sizeof(int)) == -1) {
                perror("[KRÓLOWA] write potok");
                break;
            }
            printf("[KRÓLOWA] Nie mogę już znieść jaj (ograniczenia ula)\n");
        }
    */
        sleep(3);
    }

    /*
    if (shmdt(stan_ula) == -1) {
        perror("[KRÓLOWA] shmdt");
    }
    */

    printf("[KRÓLOWA] Kończę pracę.\n");
    return 0;
}
