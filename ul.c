#include "common.h"
#include <unistd.h>

#define LICZBA_CYKLI_ZYCIA 4
#define CZAS_PRACY_W_ULU 5

static sem_t wejscie1, wejscie2;
static pthread_mutex_t liczba_pszczol_ul_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_dostepne_miejsce = PTHREAD_COND_INITIALIZER;
pthread_mutex_t numer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t depopulacja_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_numer = PTHREAD_COND_INITIALIZER;
int kontrola_pojemnosci_ula = 0;
int flaga_depopulacja = 0;
int liczebnosc_do_zredukowania = 0;
int licznik_zredukowanych = 0;
static Stan_Ula* stan_ula_local = NULL; // wskaznik na lokalną kopie stanu ula (ten proc)
Stan_Ula *stan_ula_dzielony = NULL;
int sem_id;

void* robotnica(void* arg)
{
    Argumenty_Watku* argumenty_watku = (Argumenty_Watku*) arg;
    if (!argumenty_watku) 
    {
        fprintf(stderr, "[ROBOTNICA] Błąd: argumenty wątku są NULL.\n");
        pthread_exit(NULL);
    }

    Pszczola* pszczola = argumenty_watku->pszczola;
    Stan_Ula* stan_ula_dzielony = argumenty_watku->stan_ula_do_przekazania;
    int sem_id = argumenty_watku -> sem_id;

    struct sembuf lock   = {0, -1, 0};
    struct sembuf unlock = {0,  1, 0};

    while (1) {

        pthread_mutex_lock(&depopulacja_mutex);
        
        if (stan_ula_dzielony->depopulacja_flaga == 1) {
            licznik_zredukowanych++;
            printf("Zredukowano %d\n", licznik_zredukowanych);
            semop(sem_id, &lock, 1);
            stan_ula_dzielony->obecna_liczba_pszczol = stan_ula_dzielony->obecna_liczba_pszczol - 1;
            semop(sem_id, &unlock, 1);

            if (licznik_zredukowanych >= liczebnosc_do_zredukowania) 
            {
                printf("licznik_zredukowanych %d pszczol\n", licznik_zredukowanych);
                printf("Koniec depoplacji!\n");
                semop(sem_id, &lock, 1);
                stan_ula_dzielony->depopulacja_flaga = 0; // Wylacz depopulacje, gdy osiągnięto limit
                semop(sem_id, &unlock, 1);
                licznik_zredukowanych = 0;
            }

            pthread_mutex_unlock(&depopulacja_mutex);

            //printf("Z powodu depopulacji zredukowano: %lu\n", (unsigned long)pthread_self());
            pthread_exit(NULL);
        } 
        else 
        {
            pthread_mutex_unlock(&depopulacja_mutex);
        }

        if(pszczola->pszczola_jest_w_ulu == 0)
        {
            pthread_mutex_lock(&liczba_pszczol_ul_mutex);
            while (stan_ula_dzielony->obecna_liczba_pszczol_ul >= (POCZATKOWA_ILOSC_PSZCZOL / 2)) 
            {
                pthread_cond_wait(&cond_dostepne_miejsce, &liczba_pszczol_ul_mutex);
            }
            semop(sem_id, &lock, 1);
            stan_ula_dzielony->obecna_liczba_pszczol_ul = stan_ula_dzielony->obecna_liczba_pszczol_ul + 1;
            semop(sem_id, &unlock, 1);
            pthread_mutex_unlock(&liczba_pszczol_ul_mutex);

            char* info;
            int wejscie = rand() % 2;
            if (wejscie == 1) 
            {
                sem_wait(&wejscie1);
                info = "[ROBOTNICA] Pszczoła weszła wejsciem 1";
                sem_post(&wejscie1);
            } 
            else 
            {
                sem_wait(&wejscie2);
                info = "[ROBOTNICA] Pszczoła weszła wejsciem 2";
                sem_post(&wejscie2);
            }

            pthread_mutex_lock(&liczba_pszczol_ul_mutex);
            kontrola_pojemnosci_ula++; //nowy mutex do tego?
            printf("Stan: %d/%d, \033[5;32m%s, id: %lu\033[0m\n", kontrola_pojemnosci_ula, POCZATKOWA_ILOSC_PSZCZOL/2, info, (unsigned long)pthread_self());
            pthread_mutex_unlock(&liczba_pszczol_ul_mutex);

            pszczola->pszczola_jest_w_ulu = 1;

            //praca w ulu
            int wczesniejsze_opuszczenie = rand() % 5;
            sleep(CZAS_PRACY_W_ULU - wczesniejsze_opuszczenie);
        }

        if(pszczola->pszczola_jest_w_ulu == 1)
        {
            char* info;
            int wyjscie = rand() % 2;
            if (wyjscie == 1) 
            {
                sem_wait(&wejscie1);
                info = "[ROBOTNICA] Pszczola wyszla wejsciem 1";
                sem_post(&wejscie1);
            } 
            else 
            {
                sem_wait(&wejscie2);
                info = "[ROBOTNICA] Pszczolas wyszla wejsciem 2";
                sem_post(&wejscie2);
            }

            pthread_mutex_lock(&liczba_pszczol_ul_mutex);
            kontrola_pojemnosci_ula--;
            printf("%d/%d, \033[4;34m%s, id: %lu\033[0m\n", kontrola_pojemnosci_ula, POCZATKOWA_ILOSC_PSZCZOL/2, info, (unsigned long)pthread_self());
            pthread_mutex_unlock(&liczba_pszczol_ul_mutex);

            pszczola->pszczola_jest_w_ulu = 0;
            pthread_mutex_lock(&liczba_pszczol_ul_mutex);
            semop(sem_id, &lock, 1);
            stan_ula_dzielony->obecna_liczba_pszczol_ul = stan_ula_dzielony->obecna_liczba_pszczol_ul - 1;
            semop(sem_id, &unlock, 1);
            pthread_cond_signal(&cond_dostepne_miejsce);
            pthread_mutex_unlock(&liczba_pszczol_ul_mutex);

            pszczola->licznik_odwiedzen++;
            //printf("[ROBOTNICA] Pszczoła %lu odwiedziła ul %d razy.\n", (unsigned long)pthread_self(), pszczola->licznik_odwiedzen);
            
            if (pszczola->licznik_odwiedzen >= pszczola->liczba_cykli) 
            {
                printf("\033[13;31m[ROBOTNICA] Pszczoła %lu umiera (liczba odwiedzin %d).\033[0m\n", (unsigned long)pthread_self(), pszczola->licznik_odwiedzen);

                if (semop(sem_id, &lock, 1) == -1)
                {
                    perror("[ROBOTNICA] semop lock (śmierć)");
                }

                stan_ula_dzielony->obecna_liczba_pszczol = stan_ula_dzielony->obecna_liczba_pszczol - 1;
                printf("Po smierci obecna liczba pszczol: %d\n", stan_ula_dzielony->obecna_liczba_pszczol);
                if (semop(sem_id, &unlock, 1) == -1) 
                {
                    perror("[ROBOTNICA] semop unlock (śmierć)");
                }

                free(pszczola);
                free(argumenty_watku);
                pthread_exit(NULL);
            }

            int praca_poza_ulem = rand() % 5 + 1;
            sleep(praca_poza_ulem); //praca poza ulem
        }
        }
    pthread_exit(NULL);
}


void obsluga_sygnalu(int sig)
{
    if (!stan_ula_dzielony) return;
    if (sig == SIGUSR1) 
    {
        printf("\n\033[1;35m[UL] Otrzymano SIGUSR1 -> zwiekszamy populacje!\033[0m\n");
        semop(sem_id, &lock, 1);
        stan_ula_dzielony->maksymalna_ilosc_osobnikow = stan_ula_dzielony->maksymalna_ilosc_osobnikow * 2;
        semop(sem_id, &unlock, 1);
    }
    else if (sig == SIGUSR2) 
    {
        printf("\n\033[1;35m[UL] Otrzymano SIGUSR2 -> zmniejszamy populacje!\033[0m\n");
        //printf("przed flaga_depopulacja: %d\n", stan_ula_local->obecna_liczba_pszczol);
        liczebnosc_do_zredukowania = stan_ula_dzielony->obecna_liczba_pszczol / 2;
        //printf("po depopulacji bedzie: %d\n", stan_ula_local->obecna_liczba_pszczol);
        }
        
}

int main(int argc, char* argv[])
{
    if (argc < 5) 
    {
        fprintf(stderr, "[UL] Sposób użycia: %s <fd_pipe> <shm_id> <sem_id> <msqid> \n", argv[0]);
        return 1;
    }

    srand(time(NULL));

    //int pipe_fd = atoi(argv[1]);
    int shm_id  = atoi(argv[2]);
    sem_id = atoi(argv[3]); 
    int msqid = atoi(argv[4]); 

    msgbuf otrzymana_ilosc_jaj;

    //fifo do wyslania pidu ula pszczelarzowi
    if (mkfifo(FIFO_PATH, 0600) == -1) 
    {
        if (errno != EEXIST) 
        {
            perror("Nie mozna utworzyc fifo.");
            exit(EXIT_FAILURE);
        }
    }

    int fifo_fd = open(FIFO_PATH, O_WRONLY);
    if (fifo_fd == -1) 
    {
        perror("Blad podczas otwarcia do zapisu");
        exit(EXIT_FAILURE);
    }

    int pid = (int)getpid();
    if (pid <= 0) {
        perror("Błąd przy pobieraniu PID");
        exit(EXIT_FAILURE);
    }

    char pid_str[20];
    if (snprintf(pid_str, sizeof(pid_str), "%d", pid) < 0) {
        perror("Blad przy konwersji PID na ciag znakow");
        exit(EXIT_FAILURE);
    }

    if (write(fifo_fd, pid_str, strlen(pid_str) + 1) == -1) {
        perror("Blad zapisu do FIFO");
        close(fifo_fd);
        exit(EXIT_FAILURE);
    }
    
    if (close(fifo_fd) == -1) {
        perror("Blad zamykania FIFO");
        exit(EXIT_FAILURE);
    }

    printf("Pid ul: %s\n", pid_str);

    // dolaczenie do pma.dzielonej
    stan_ula_dzielony = (Stan_Ula*) shmat(shm_id, NULL, 0);
    if (stan_ula_dzielony == (void*)-1) {
        perror("[UL] shmat");
        exit(EXIT_FAILURE);
    }

    stan_ula_local = (Stan_Ula*) malloc(sizeof(Stan_Ula));
    if (!stan_ula_local) {
        fprintf(stderr, "[UL] Błąd alokacji stan_ula_local.\n");
        exit(EXIT_FAILURE);
    }

    struct sembuf lock = {0, -1, 0};
    struct sembuf unlock = {0, 1, 0};
    
    //init semaforow dla wejsc
    if (sem_init(&wejscie1, 0, 1) == -1) {
        perror("[UL] sem_init wejscie1");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&wejscie2, 0, 1) == -1) {
        perror("[UL] sem_init wejscie2");
        exit(EXIT_FAILURE);
    }

    // sygnaly od pszczelarza
    struct sigaction sa;
    sa.sa_handler = obsluga_sygnalu;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    


    printf("[Ul] Obecna - %d\n Obecna ul - %d\n, Maks - %d\n, Stan poczatkowy - %d\n", stan_ula_dzielony->obecna_liczba_pszczol, stan_ula_dzielony->obecna_liczba_pszczol_ul, stan_ula_dzielony->maksymalna_ilosc_osobnikow, stan_ula_dzielony->stan_poczatkowy);

    //tworzymy kilka statycznych robotnic
    int liczba_poczatek = POCZATKOWA_ILOSC_PSZCZOL;
    pthread_t* robotnice_tab = (pthread_t*) calloc(liczba_poczatek, sizeof(pthread_t));
    if (!robotnice_tab) {
        fprintf(stderr, "[UL] Błąd alokacji robotnice_tab.\n");
        free(stan_ula_local);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < liczba_poczatek; i++) 
    {
        Pszczola* p = (Pszczola*) malloc(sizeof(Pszczola));
        if (!p) 
        {
            fprintf(stderr, "[UL] Błąd alokacji pszczoły.\n");
            continue;
        }
        
        p->licznik_odwiedzen    = 0;
        p->liczba_cykli         = LICZBA_CYKLI_ZYCIA;
        p->pszczola_jest_w_ulu  = 0;

        //stan_ula_local->obecna_liczba_pszczol++;
        if (semop(sem_id, &lock, 1) == -1) {
            perror("[UL] semop lock (tworzenie robotnic)");
        }

        stan_ula_dzielony->obecna_liczba_pszczol = stan_ula_dzielony->obecna_liczba_pszczol + 1;
        //printf("[Ul]Obecna l.pszczol - %d\n Obecna l.pszczol ul - %d\n",stan_ula_dzielony->obecna_liczba_pszczol, stan_ula_dzielony->obecna_liczba_pszczol_ul);
        
        if (semop(sem_id, &unlock, 1) == -1) {
            perror("[UL] semop unlock (tworzenie robotnic)");
        }

        // arguemnty dla watkow
        Argumenty_Watku* args = (Argumenty_Watku*) malloc(sizeof(Argumenty_Watku));
        if (!args) 
        {
            fprintf(stderr, "[UL] Błąd alokacji Argumenty_Watku.\n");
            free(p);
            continue;
        }

        args->pszczola                  = p;
        args->sem_id                    = sem_id;
        args->stan_ula_do_przekazania   = stan_ula_dzielony;

        pthread_create(&robotnice_tab[i], NULL, robotnica, (void*) args);
        printf("Stworzono %d\n", i+1);
        usleep(10000);
    }

    while (1) {

        printf("[UL] Stan: obecna=%d, w_ulu=%d, max=%d\n",stan_ula_dzielony->obecna_liczba_pszczol,stan_ula_dzielony->obecna_liczba_pszczol_ul,stan_ula_dzielony->maksymalna_ilosc_osobnikow);

        /*
        ssize_t count = read(pipe_fd, &otrzymana_ilosc_jaj, sizeof(int));
        if (count <= 0) 
        {
            perror("[UL] read (potok krolowej)");
            break;
        }
        */

        if (msgrcv(msqid, &otrzymana_ilosc_jaj, sizeof(msgbuf) - sizeof(long), MSG_TYPE_EGGS, 0) == -1) {
        if (errno == EINTR) {
            printf("[UL] msgrcv przerwano sygnalem, retrying...\n");
            continue;
        }
        perror("[UL] msgrcv error");
        }
        
        //tworzenie nowych robotnic - odbieranie informacji o probie zlozenia jaj
        
        printf("\033[1;33mKrolowa zniosla %d jaj\033[0m\n", otrzymana_ilosc_jaj.eggs);
        kontrola_pojemnosci_ula+=otrzymana_ilosc_jaj.eggs;

        pthread_t* nowe_robotnice = (pthread_t*) calloc(otrzymana_ilosc_jaj.eggs, sizeof(pthread_t));
        if (!nowe_robotnice) 
        {
            fprintf(stderr, "[UL] Blad alokacji dla nowych robotnic.\n");
            continue;
        }
            

        for (int i = 0; i < otrzymana_ilosc_jaj.eggs; i++) 
        {
            Pszczola* p = (Pszczola*) malloc(sizeof(Pszczola));
            if (!p) 
            {
                fprintf(stderr, "[UL] Błąd alokacji pszczoły.\n");
                continue;
            }

            p->licznik_odwiedzen   = 0;
            p->liczba_cykli        = LICZBA_CYKLI_ZYCIA;
            p->pszczola_jest_w_ulu = 1; // osobnik w ulu

            Argumenty_Watku* args = (Argumenty_Watku*) malloc(sizeof(Argumenty_Watku));
            if (!args) 
            {
                fprintf(stderr, "[UL] Błąd alokacji Argumenty_Watku.\n");
                free(p);
                continue;
            }

            args->pszczola                  = p;
            args->sem_id                    = sem_id;
            args->stan_ula_do_przekazania   = stan_ula_dzielony;

            //wyleganie tutaj
            pthread_create(&nowe_robotnice[i], NULL, robotnica, (void*)args);
        }
        free(nowe_robotnice);

        printf("[UL] Stan: obecna=%d, w_ulu=%d, max=%d\n",stan_ula_dzielony->obecna_liczba_pszczol,stan_ula_dzielony->obecna_liczba_pszczol_ul,stan_ula_dzielony->maksymalna_ilosc_osobnikow);
        sleep(1);
    }

    for (int i = 0; i < liczba_poczatek; i++) 
    {
        pthread_join(robotnice_tab[i], NULL);
    }
    free(robotnice_tab);

    // sprzatenie
    if (shmdt(stan_ula_dzielony) == -1) 
    {
        perror("[UL] shmdt");
    }

    free(stan_ula_local);

    sem_destroy(&wejscie1);
    sem_destroy(&wejscie2);
    pthread_mutex_destroy(&liczba_pszczol_ul_mutex);
    pthread_cond_destroy(&cond_dostepne_miejsce);
    pthread_mutex_destroy(&depopulacja_mutex);
    printf("[UL] Koncze proces ula.\n");
    return 0;
}