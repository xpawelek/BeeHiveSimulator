#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <time.h>

#define MAKSYMALNA_ILOSC_RAMEK 2

static int licznik = 0;

int main(int argc, char* argv[])
{
    if (argc < 3) 
    {
        fprintf(stderr, "[PSZCZELARZ] Sposób użycia: %s <sem_id> <shm_id> \n", argv[0]);
        return 1;
    }
    srand(time(NULL));

    int sem_id = atoi(argv[1]);
    int shm_id = atoi(argv[2]);

    int fd = open(FIFO_PATH, O_RDONLY);
    if (fd == -1) 
    {
        perror("open FIFO for reading");
        exit(EXIT_FAILURE);
    }

    //odbieranie pidu ula
    char buffer[100];
    memset(buffer, 0, sizeof(buffer));

    if (read(fd, buffer, sizeof(buffer)) == -1) {
        perror("Blad odczytu z FIFO");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (close(fd) == -1) {
        perror("Blad zamykania FIFO");
        exit(EXIT_FAILURE);
    }

    printf("Received PID: %s\n", buffer);
    int pid_ul = atoi(buffer);
    printf("pid ul : %d\n", pid_ul);


    // dolaczenie do pam. dzielonej
    Stan_Ula* stan_ula = (Stan_Ula*) shmat(shm_id, NULL, 0);
    if (stan_ula == (void*)-1) {
        perror("[PSZCZELARZ] shmat");
        return 1;
    }

    if (semop(sem_id, &lock, 1) == -1) {
        perror("[PSZCZELARZ] semop lock (początek)");
    }

    int stan_poczatkowy = stan_ula->stan_poczatkowy;

    if (semop(sem_id, &unlock, 1) == -1) {
        perror("[PSZCZELARZ] semop unlock (początek)");
    }

    int obecna_liczba_pszczol;
    int obecna_ilosc_ramek = 1;

    while (1)
    {
        if (semop(sem_id, &lock, 1) == -1) {
            perror("[PSZCZELARZ] semop lock (odczyt)");
            break;
        }

        obecna_liczba_pszczol = stan_ula->obecna_liczba_pszczol;
        //maksymalna_ilosc_osobnikow = stan_ula->maksymalna_ilosc_osobnikow;
        //liczba_w_ulu               = stan_ula->obecna_liczba_pszczol_ul;

        if (semop(sem_id, &unlock, 1) == -1) {
            perror("[PSZCZELARZ] semop unlock (odczyt)");
            break;
        }

        if(obecna_ilosc_ramek < MAKSYMALNA_ILOSC_RAMEK && obecna_liczba_pszczol == stan_poczatkowy * obecna_ilosc_ramek)
        {
            //printf("Stan poczatkowy: %d i obecna liczba pszczol: %d\n", stan_poczatkowy, obecna_liczba_pszczol);
            obecna_ilosc_ramek++;
            kill(pid_ul, SIGUSR1);
        }

        if(obecna_ilosc_ramek == MAKSYMALNA_ILOSC_RAMEK&& obecna_liczba_pszczol == stan_poczatkowy * obecna_ilosc_ramek)
        {
            obecna_ilosc_ramek--;
            kill(pid_ul, SIGUSR2);
        }

        licznik++;

        if(licznik % 2000000 == 0)
        {
            kill(pid_ul, SIGUSR2); 
        }
    }

    //sprzatanie
    /*
    if (shmdt(stan_ula) == -1) {
        perror("[PSZCZELARZ] shmdt");
        return 1;
    }

    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("[PSZCZELARZ] shmctl");
        return 1;
    }
    */

    return 0;
}
