#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>
#include <semaphore.h>

int maksymalna_pojemnosc_ula = 5;
int obecna_liczba_pszczol_ul = 0;
sem_t wejscie1, wejscie2;
pthread_mutex_t liczba_pszczol_ul_mutex;
pthread_cond_t cond_dostepne_miejsce = PTHREAD_COND_INITIALIZER;

typedef struct{
    int licznik_odwiedzen;
    int liczba_cykli;
} Pszczola;

void* robotnica(void* args)
{
    Pszczola* pszczola = (Pszczola*) args;
    while(1)
    {
        pthread_mutex_lock(&liczba_pszczol_ul_mutex);
    //moglibysmy zrobic while i contition variable?
    while(obecna_liczba_pszczol_ul >= maksymalna_pojemnosc_ula)
    {
        printf("Obecna liczba pszczol w ulu czekajac na sygnal: %d\n", obecna_liczba_pszczol_ul);
        pthread_cond_wait(&cond_dostepne_miejsce, &liczba_pszczol_ul_mutex);
    }
        printf("Obecna liczba pszczol w ulu po dostaniu sygnalu: %d\n", obecna_liczba_pszczol_ul);
        int wejscie = rand() % 2;

        if(wejscie == 1)
        {
            sem_wait(&wejscie1);
            //pszczola wchodzik idk przez sekunde i zwaliamy dziurke
            sleep(1);
            sem_post(&wejscie1);
            printf("Pszczola weszla wejsciem 1\n");
        }
        else
        {
            sem_wait(&wejscie2);
            //pszczola wchodzik idk przez sekunde i zwaliamy dziurke
            sleep(1);
            sem_post(&wejscie2);
             printf("Pszczola weszla wejsciem 2\n");
        }
        obecna_liczba_pszczol_ul++;
        pthread_mutex_unlock(&liczba_pszczol_ul_mutex);

        //wykonujemy jakas prace przez jakiś randomowy czas i pszczola opuszcza ul

        //sleep(rand() % 10 + 5);
        sleep(2);

        pthread_mutex_lock(&liczba_pszczol_ul_mutex);
        wejscie = rand() % 2;

        if(wejscie == 1)
        {
            sem_wait(&wejscie1);
            //pszczola wchodzik idk przez sekunde i zwaliamy dziurke
            sleep(1);
            sem_post(&wejscie1);
            printf("Pszczola wyszla wejsciem 1\n");
        }
        else
        {
            sem_wait(&wejscie2);
            //pszczola wchodzik idk przez sekunde i zwaliamy dziurke
            sleep(1);
            sem_post(&wejscie2);
             printf("Pszczola wyszla wejsciem 2\n");
        }
        obecna_liczba_pszczol_ul--;
        pszczola->licznik_odwiedzen++;
        pthread_cond_signal(&cond_dostepne_miejsce);
        pthread_mutex_unlock(&liczba_pszczol_ul_mutex);
        printf("Liczba odwiedzen danej pszczoly: %d\n", pszczola->licznik_odwiedzen);
                if(pszczola-> licznik_odwiedzen == pszczola->liczba_cykli)
        {
            printf("Pszczola is dead!\n");
            pthread_exit(NULL);
        }
    }

    
    //sprawdzenie czy wystarczajaca ilosc miejsca w ulu -> jesli nie, to czkekamy az jakas pszczola wyjdzie lub sygnal do pszczelarza?

    //wejscie -> dwa wejscia, pszczola losuje ktore? - sprawdzamy czy nie jest zajete (dwa mutexy na wejscia?) / semafory?

    //obsluga bycia w ulu przez jakis czas // sleep

    //opuszczenie ula, inkrementacja liczby odwiedzin, zwolnienie miejsca w ulu -> 

    //sprawdzenie ile razy pszczola byla w ulu, usuniecie wątku jesli przekroczyl ilosc cykli pthread_exit(null)

    

}

void proces_krolowej(int pipe_fd){
    while(1)
    {
        int liczba_zlozonych_jaj = rand() % 5 + 1;
        write(pipe_fd, &liczba_zlozonych_jaj, sizeof(int));
        sleep(3);
    }
}

void proces_ula(int pipe_fd)
{
    int otrzymana_ilosc_jaj;
    pthread_t* wyklute_robotnice;
    while(1)
    {
        read(pipe_fd, &otrzymana_ilosc_jaj, sizeof(int));
        printf("Krolowa zniosla %d jaj\n", otrzymana_ilosc_jaj);
        wyklute_robotnice = malloc(sizeof(pthread_t) * otrzymana_ilosc_jaj);
        
        for(int i=0; i<otrzymana_ilosc_jaj; i++)
        {
            Pszczola* pszczola_robotnica_dane = malloc(sizeof(Pszczola));
            pszczola_robotnica_dane->licznik_odwiedzen = 0;
            //pszczoly[i].liczba_cykli = rand() % 200 + 100;
            pszczola_robotnica_dane->liczba_cykli = 3;
            pthread_create(&wyklute_robotnice[i], NULL, &robotnica, (void*) pszczola_robotnica_dane);

        }
    }

    for(int i=0; i<otrzymana_ilosc_jaj; i++)
    {
        pthread_join(wyklute_robotnice[i], NULL);
    }
}

int main()
{
    srand(time(NULL));
    int pipe_skladanie_jaj[2];
        if(pipe(pipe_skladanie_jaj) == -1)
    {
        return 2;
    }
    int krolowa = fork();
    int pszczelarz;
    if(krolowa == 0)
    {
        printf("jestem krolowa!\n");
        close(pipe_skladanie_jaj[0]);  // Zamknięcie odczytu w procesie dziecka
        proces_krolowej(pipe_skladanie_jaj[1]);
        close(pipe_skladanie_jaj[1]);
        return 0;
    }
    else
    {
        pszczelarz = fork();
        if(pszczelarz == 0)
        {
            printf("Jestem pszelarz!\n");
            close(pipe_skladanie_jaj[0]);
            close(pipe_skladanie_jaj[1]);
            exit(0);
        }
        else
        {
            printf("jestem wątek glowny!\n");
        }
    }

 
    close(pipe_skladanie_jaj[1]); 
    int liczba_osobnikow = 3;
    pthread_t robotnice[liczba_osobnikow];
    Pszczola pszczoly[liczba_osobnikow];
    sem_init(&wejscie1, 0, 1);
    sem_init(&wejscie2, 0, 1);
    pthread_mutex_init(&liczba_pszczol_ul_mutex, NULL);
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

    for(int i=0; i<liczba_osobnikow; i++)
    {
        Pszczola* pszczola_robotnica_dane = malloc(sizeof(Pszczola));
        pszczola_robotnica_dane->licznik_odwiedzen = 0;
        //pszczoly[i].liczba_cykli = rand() % 200 + 100;
        pszczola_robotnica_dane->liczba_cykli = 3;
        if(pthread_create(&robotnice[i], NULL, *robotnica, (void*)pszczola_robotnica_dane) != 0){

            return 1;
        }
    }

    proces_ula(pipe_skladanie_jaj[0]);
    close(pipe_skladanie_jaj[0]);

    for(int i=0; i<10; i++)
    {
        if(pthread_join(robotnice[i], NULL) != 0){
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