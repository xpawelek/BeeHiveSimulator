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

#define POCZATKOWA_ILOSC_PSZCZOL 16

int maksymalna_pojemnosc_ula = POCZATKOWA_ILOSC_PSZCZOL / 2;
sem_t wejscie1, wejscie2;
sem_t* sem_dostep_ul; 
pthread_mutex_t liczba_pszczol_ul_mutex;
pthread_mutex_t blokada_ula = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_dostepne_miejsce = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_wejscie_otwarte = PTHREAD_COND_INITIALIZER;
int wejscie_otwarte = 1;
pthread_mutex_t wejscie_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct{
    int licznik_odwiedzen;
    int liczba_cykli;
    int pszczola_jest_w_ulu;
} Pszczola;

typedef struct{
    int obecna_liczba_pszczol;
    int maksymalna_ilosc_osobnikow;
    int obecna_liczba_pszczol_ul;
    int stan_poczatkowy;
    //int pojemnosc_ula;
} Stan_Ula;

Stan_Ula* stan_ula;

void* robotnica(void* args)
{
    Pszczola* pszczola = (Pszczola*) args;
    while(1)
    {
        pthread_mutex_lock(&liczba_pszczol_ul_mutex);
        //moglibysmy zrobic while i contition variable?
       // printf("Od teraz tyle moze wejsc do ula: %d\n", stan_ula->maksymalna_ilosc_osobnikow/2);
        //while(stan_ula->obecna_liczba_pszczol_ul >= stan_ula->maksymalna_ilosc_osobnikow/2)
        while (stan_ula->obecna_liczba_pszczol_ul >= POCZATKOWA_ILOSC_PSZCZOL / 2 || wejscie_otwarte == 0) {
            pthread_cond_wait(&cond_dostepne_miejsce, &liczba_pszczol_ul_mutex);
        }
        //printf("Obecna liczba pszczol w ulu po dostaniu sygnalu: %d\n", stan_ula->obecna_liczba_pszczol_ul);
        
        if(pszczola->pszczola_jest_w_ulu == 0)
        {
            int wejscie = rand() % 2;
            if(wejscie == 1)
            {
                sem_wait(&wejscie1);
                sleep(1);
                printf("Pszczola weszla wejsciem 1\n");
                sem_post(&wejscie1);
            }
            else
            {
                sem_wait(&wejscie2);
                sleep(1);
                printf("Pszczola weszla wejsciem 1\n");
                sem_post(&wejscie2);               
            }

            pszczola->pszczola_jest_w_ulu = 1;
            stan_ula->obecna_liczba_pszczol_ul++;
        }
        pthread_mutex_unlock(&liczba_pszczol_ul_mutex);
        
        sleep(2); //praca - instynktowne opuszczenie ula/ moze opuscic wczesniej? / niektore wczesniejn niektore pozniej
        //wykonujemy jakas prace przez jakiś randomowy czas i pszczola opuszcza ul

        //sleep(rand() % 10 + 5);

        pthread_mutex_lock(&liczba_pszczol_ul_mutex);
        if(pszczola->pszczola_jest_w_ulu == 1)
        {
            int wejscie = rand() % 2;
            if(wejscie == 1)
            {
                sem_wait(&wejscie1);
                sleep(1);
                printf("Pszczola wyszla wejsciem 1\n");
                sem_post(&wejscie1);
            }
            else
            {
                sem_wait(&wejscie2);
                sleep(1);
                printf("Pszczola wyszla wejsciem 2\n");
                sem_post(&wejscie2);                
            }
            pszczola->pszczola_jest_w_ulu = 0;
            stan_ula->obecna_liczba_pszczol_ul--;
            pszczola->licznik_odwiedzen++;
           // printf("Pszczola odwiedzila ul %d razy: \n", pszczola->licznik_odwiedzen);
            pthread_cond_signal(&cond_dostepne_miejsce);
        }

        pthread_mutex_unlock(&liczba_pszczol_ul_mutex);

        if(pszczola->licznik_odwiedzen == pszczola->liczba_cykli)
            {
               // printf("Pszczola is dead!\n");
                stan_ula->obecna_liczba_pszczol--;
                pthread_exit(NULL);
            }

    }

    
    //sprawdzenie czy wystarczajaca ilosc miejsca w ulu -> jesli nie, to czkekamy az jakas pszczola wyjdzie lub sygnal do pszczelarza?

    //wejscie -> dwa wejscia, pszczola losuje ktore? - sprawdzamy czy nie jest zajete (dwa mutexy na wejscia?) / semafory?

    //obsluga bycia w ulu przez jakis czas // sleep

    //opuszczenie ula, inkrementacja liczby odwiedzin, zwolnienie miejsca w ulu -> 

    //sprawdzenie ile razy pszczola byla w ulu, usuniecie wątku jesli przekroczyl ilosc cykli pthread_exit(null)

    

}

void proces_krolowej(int pipe_fd, int klucz, int shm_id){
    //sleep(0.5);
    if (shm_id == -1) 
    {
        perror("shmget");
        exit(1);
    }

    Stan_Ula* stan_ula = (Stan_Ula*) shmat(shm_id, NULL, 0);
    struct sembuf lock = {0, -1, 0};
    struct sembuf unlock = {0, 1, 0};

    while(1)
    {   
        semop(shm_id, &lock, 1);
        int liczba_pszczol_ul = stan_ula->obecna_liczba_pszczol_ul;
        int obecna_liczba_pszczol = stan_ula->obecna_liczba_pszczol;
        int maksymalna_ilosc_osobnikow = stan_ula->maksymalna_ilosc_osobnikow;
        semop(shm_id, &unlock, 1);
        printf("Krolowa po odczytaniu po odczytaniu danych - obecna liczba pszczol: %d, maksymalna l.osobnikow: %d, ilosc osobnikow w ulu: %d\n",
        obecna_liczba_pszczol, maksymalna_ilosc_osobnikow, liczba_pszczol_ul);
        int liczba_zlozonych_jaj = rand() % 3 + 1;
        printf("DEBUG: liczba_pszczol_ul=%d, obecna_liczba_pszczol=%d, liczba_zlozonych_jaj=%d, maksymalna_ilosc_osobnikow=%d\n",
           liczba_pszczol_ul, obecna_liczba_pszczol, liczba_zlozonych_jaj, maksymalna_ilosc_osobnikow);
        if ((liczba_pszczol_ul + liczba_zlozonych_jaj <= POCZATKOWA_ILOSC_PSZCZOL / 2) && 
    (obecna_liczba_pszczol + liczba_zlozonych_jaj <= maksymalna_ilosc_osobnikow)) {
    if (liczba_pszczol_ul + liczba_zlozonych_jaj == POCZATKOWA_ILOSC_PSZCZOL / 2) {
        pthread_mutex_lock(&wejscie_mutex);
        wejscie_otwarte = 0; // Zamykamy wejście
        pthread_mutex_unlock(&wejscie_mutex);
    }
    write(pipe_fd, &liczba_zlozonych_jaj, sizeof(int));
}
        else
        {
            liczba_zlozonych_jaj = 0;
            write(pipe_fd, &liczba_zlozonych_jaj, sizeof(int));
        }

        sleep(3);
    }

    if (shmdt(stan_ula) == -1) 
    {
        perror("shmdt");
    }

    if (shmctl(shm_id, IPC_RMID, NULL) == -1) 
    {
        perror("shmctl");
    }
}

void proces_ula(int pipe_fd, int shm_id, Stan_Ula* stan_ula_do_przekazania)
{
    int otrzymana_ilosc_jaj;
    pthread_t* wyklute_robotnice;
    struct sembuf lock = {0, -1, 0};
    struct sembuf unlock = {0, 1, 0};

    while(1)
    {
        read(pipe_fd, &otrzymana_ilosc_jaj, sizeof(int));
        printf("Krolowa zniosla %d jaj\n", otrzymana_ilosc_jaj);
        printf("Jest tyle pszczol: %d, a moze byc maks tyle %d\n", stan_ula->obecna_liczba_pszczol, stan_ula->maksymalna_ilosc_osobnikow);

        if (otrzymana_ilosc_jaj > 0)
        {
            wyklute_robotnice = malloc(sizeof(pthread_t) * otrzymana_ilosc_jaj);
            sleep(1);
            for(int i=0; i<otrzymana_ilosc_jaj; i++)
            {
                Pszczola* pszczola_robotnica_dane = malloc(sizeof(Pszczola));
                pszczola_robotnica_dane->licznik_odwiedzen = 0;
                //pszczoly[i].liczba_cykli = rand() % 200 + 100;
                pszczola_robotnica_dane->liczba_cykli = 3;
                pszczola_robotnica_dane->pszczola_jest_w_ulu = 1;
                pthread_create(&wyklute_robotnice[i], NULL, &robotnica, (void*) pszczola_robotnica_dane);
                semop(shm_id, &lock, 1); 
                stan_ula->obecna_liczba_pszczol++;
                stan_ula->obecna_liczba_pszczol_ul++;
                semop(shm_id, &unlock, 1);

                semop(shm_id, &lock, 1);
                stan_ula_do_przekazania->obecna_liczba_pszczol = stan_ula->obecna_liczba_pszczol;
                semop(shm_id, &unlock, 1);
            }
            pthread_mutex_lock(&wejscie_mutex);
            wejscie_otwarte = 1; // Otwieramy wejście
            pthread_cond_broadcast(&cond_dostepne_miejsce); // Powiadamiamy wszystkie pszczoły
            pthread_mutex_unlock(&wejscie_mutex);
        }
            semop(shm_id, &lock, 1);
            stan_ula_do_przekazania->obecna_liczba_pszczol = stan_ula->obecna_liczba_pszczol;
            stan_ula_do_przekazania->obecna_liczba_pszczol_ul = stan_ula->obecna_liczba_pszczol_ul;
            stan_ula_do_przekazania->maksymalna_ilosc_osobnikow = stan_ula->maksymalna_ilosc_osobnikow;
            //printf("Przeslano %d jako maksymalna l.osobnikow\n", stan_ula_do_przekazania->maksymalna_ilosc_osobnikow);
            //printf("Na początku wysyłam stan poczatkowy: %d, max l. osobnikow: %d, obecna l.os: %d i obecna l.os. w ulu: %d\n",
            //stan_ula->stan_poczatkowy, stan_ula->maksymalna_ilosc_osobnikow, stan_ula->obecna_liczba_pszczol, stan_ula->obecna_liczba_pszczol_ul);
            semop(shm_id, &unlock, 1);

        printf("Ul - proces glowny wysyla po zapisaniu danych - obecna liczba pszczol: %d, maksymalna l.osobnikow: %d, ilosc osobnikow w ulu: %d\n",
         stan_ula_do_przekazania->obecna_liczba_pszczol, stan_ula_do_przekazania->maksymalna_ilosc_osobnikow, stan_ula_do_przekazania->obecna_liczba_pszczol_ul);
    }

    for(int i=0; i<otrzymana_ilosc_jaj; i++)
    {
        pthread_join(wyklute_robotnice[i], NULL);
    }
}

void obsluga_sygnalu(int sig)
{
    if(sig == SIGUSR1)
    {
        printf("\n\nsigusr1 - zwieksz\n\n");
        //stan_ula->maksymalna_ilosc_osobnikow += (stan_ula->stan_poczatkowy / 5);
        stan_ula->maksymalna_ilosc_osobnikow *= 2;
        printf("Teraz maksymalna liczba osobnikow: %d\n", stan_ula->maksymalna_ilosc_osobnikow);
    }

    if(sig == SIGUSR2)
    {
        printf("\n\nsigusr1 - zmniejsz\n\n");
        stan_ula->maksymalna_ilosc_osobnikow /= 2;
        printf("Teraz maksymalna liczba osobnikow: %d\n", stan_ula->maksymalna_ilosc_osobnikow);
    }
}

int main()
{
   // printf("PID: %d i PPID: %d dla ula\n", getpid(), getppid());
    srand(time(NULL));
    sem_init(&wejscie1, 0, 1);
    sem_init(&wejscie2, 0, 1);
    pthread_mutex_init(&liczba_pszczol_ul_mutex, NULL);
    
    struct sigaction sa;
    sa.sa_handler = &obsluga_sygnalu;
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction error");
        return 1;
    }

    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction error");
        return 1;
    }
    struct sembuf lock = {0, -1, 0};
    struct sembuf unlock = {0, 1, 0};

    stan_ula = malloc(sizeof(Stan_Ula));
    stan_ula->obecna_liczba_pszczol = 0;
    stan_ula->obecna_liczba_pszczol_ul = 0;
    stan_ula->maksymalna_ilosc_osobnikow = POCZATKOWA_ILOSC_PSZCZOL; // poddzielone na dwa = pojemnosc ula
    stan_ula->stan_poczatkowy = POCZATKOWA_ILOSC_PSZCZOL;

    int pipe_skladanie_jaj[2];
    if(pipe(pipe_skladanie_jaj) == -1)
    {
        return 2;
    }

    key_t klucz = ftok("./unikalny_klucz.txt", 65);

    int shm_id = shmget(klucz, 1024, 0666 | IPC_CREAT);
    if (shm_id == -1) 
    {
        perror("shmget");
        exit(1);
    }

    Stan_Ula* stan_ula_do_przekazania = (Stan_Ula*) shmat(shm_id, NULL, 0);
    if (stan_ula_do_przekazania == (void*) -1) 
    {
        perror("shmat");
        exit(1);
    }

    /*
    else
    {
        pszczelarz = fork();
        if(pszczelarz == 0)
        {
            while(1)
            {
                printf("Jestem pszelarz!\n");
                printf("pszczelarz zna obecna liczbe pszczol: %d\n", obecna_liczba_pszczol);
                sleep(5);
            }
            close(pipe_skladanie_jaj[0]);
            close(pipe_skladanie_jaj[1]);
            exit(0);
        }
        else
        {
            printf("jestem wątek glowny!\n");
        }
    }
*/

    //tworzenie robotnic, krolowej i pszczelarza oraz czekanie na ich zakonczenie
    /*
    for(int i=0; i<liczba_osobnikow; i++)
    {
        pszczoly[i].pszczola_znajduje_sie_w_ulu = 0;
        pszczoly[i].licznik_odwiedzen = 0;
        //pszczoly[i].liczba_cykli = rand() % 200 + 100;
        pszczoly[i].liczba_cykli = 3;
    }
    */

   pthread_t robotnice[POCZATKOWA_ILOSC_PSZCZOL];
   Pszczola pszczoly[POCZATKOWA_ILOSC_PSZCZOL];

    for(int i=0; i<POCZATKOWA_ILOSC_PSZCZOL; i++)
    {
        Pszczola* pszczola_robotnica_dane = malloc(sizeof(Pszczola));
        pszczola_robotnica_dane->licznik_odwiedzen = 0;
        //pszczoly[i].liczba_cykli = rand() % 200 + 100;
        pszczola_robotnica_dane->liczba_cykli = 3; //mozemy dodac ze albo jest albo nie
        pszczola_robotnica_dane->pszczola_jest_w_ulu = 0;
        stan_ula->obecna_liczba_pszczol++;

        if(pthread_create(&robotnice[i], NULL, *robotnica, (void*)pszczola_robotnica_dane) != 0)
        {
            return 1;
        }
    }

    semop(shm_id, &lock, 1);
    stan_ula_do_przekazania->obecna_liczba_pszczol = stan_ula->obecna_liczba_pszczol;
    stan_ula_do_przekazania->obecna_liczba_pszczol_ul = stan_ula->obecna_liczba_pszczol_ul;
    stan_ula_do_przekazania->maksymalna_ilosc_osobnikow = stan_ula->maksymalna_ilosc_osobnikow;
    stan_ula_do_przekazania->stan_poczatkowy = stan_ula->stan_poczatkowy;
    semop(shm_id, &unlock, 1);
    printf("Na początku wysyłam stan poczatkowy: %d, max l. osobnikow: %d, obecna l.os: %d i obecna l.os. w ulu: %d\n",
            stan_ula->stan_poczatkowy, stan_ula->maksymalna_ilosc_osobnikow, stan_ula->obecna_liczba_pszczol, stan_ula->obecna_liczba_pszczol_ul);

    if(fork() == 0)
    {
        execl("./pszczelarz", "./pszczelarz", NULL);
        exit(1);
    }

    sleep(1);

    int krolowa = fork();
    if(krolowa == 0)
    {
        printf("jestem krolowa!\n");
        close(pipe_skladanie_jaj[0]);  // Zamknięcie odczytu w procesie dziecka
        proces_krolowej(pipe_skladanie_jaj[1], klucz, shm_id);
        close(pipe_skladanie_jaj[1]);
        return 0;
    }

    close(pipe_skladanie_jaj[1]);
    proces_ula(pipe_skladanie_jaj[0], shm_id, stan_ula_do_przekazania);
    close(pipe_skladanie_jaj[0]);

    if (shmdt(stan_ula_do_przekazania) == -1) 
    {
        perror("shmdt");
        exit(1);
    }
    
    shmctl(shm_id, IPC_RMID, NULL);

    for(int i=0; i<10; i++)
    {
        if(pthread_join(robotnice[i], NULL) != 0)
        {
            return 2;
        }
    }
 

    while(wait(NULL) > 0);

    sem_destroy(&wejscie1);
    sem_destroy(&wejscie2);
    pthread_mutex_destroy(&liczba_pszczol_ul_mutex);
    pthread_cond_destroy(&cond_dostepne_miejsce);   
    
    return 0;
}