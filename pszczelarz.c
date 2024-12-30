#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
typedef struct{
    int obecna_liczba_pszczol;
    int maksymalna_ilosc_osobnikow;
    int obecna_liczba_pszczol_ul;
    int stan_poczatkowy;
    //int pojemnosc_ula;
} Stan_Ula;

int main()
{
    //printf("PID: %d i PPID: %d dla pszczelarza\n", getpid(), getppid());
    //sleep(2);
    //printf("Jestem pszelarz xaxa!\n");
    //kill(getppid(), SIGUSR1);
    //sleep(2);
    //kill(getppid(), SIGUSR2);
    sleep(0.5);
    int ilosc_ramek = 5;
    key_t klucz = ftok("./unikalny_klucz.txt", 65);
    int shm_id = shmget(klucz, 1024, 0666);
    if (shm_id == -1) {
        perror("shmget");
        exit(1);
    }

    Stan_Ula* stan_ula = (Stan_Ula*) shmat(shm_id, NULL, 0);
    if (stan_ula == (void*)-1) {
        perror("shmat");
        return 1;
    }

    struct sembuf lock = {0, -1, 0};
    struct sembuf unlock = {0, 1, 0};
    
    int stan_poczatkowy;
    semop(shm_id, &lock, 1);
    stan_poczatkowy = stan_ula->stan_poczatkowy;
    semop(shm_id, &unlock, 1);

    while(1)
    {
        semop(shm_id, &lock, 1);
        //stan poczatkowy
        //obecnie ile jest
       // printf("Pszczelarz: Jest tyle pszczol: %d, a moze byc maks tyle %d\n", stan_ula->obecna_liczba_pszczol, stan_ula->maksymalna_ilosc_osobnikow);
        if(stan_ula->obecna_liczba_pszczol == stan_ula->maksymalna_ilosc_osobnikow && ilosc_ramek != 0)
        {
            kill(getppid(), SIGUSR1);
            printf("signal sigusr1 sent - ilosc ramek: %d!\n", ilosc_ramek);
            ilosc_ramek--;
        }
        //printf("Pszczelarz odczytał obecna liczbe pszczol: %d i obecna liczbe pszczol w ulu: %d = \n", stan_ula->obecna_liczba_pszczol, stan_ula->obecna_liczba_pszczol_ul);
        semop(shm_id, &unlock, 1);
    }

    if (shmdt(stan_ula) == -1) {
        perror("shmdt");
        return 1;
    }

    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        return 1;
    }
    return 0;
}