#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>

int maksymalna_pojemnosc_ula = 5;

typedef struct{
    int pszczola_znajduje_sie_w_ulu;
    int licznik_odwiedzen;
    int liczba_cykli;
} Pszczola;

void* robotnica(void* args)
{
    Pszczola* pszczola = (Pszczola*) args;
    
    //sprawdzenie czy wystarczajaca ilosc miejsca w ulu -> jesli nie, to czkekamy az jakas pszczola wyjdzie lub sygnal do pszczelarza?

    //wejscie -> dwa wejscia, pszczola losuje ktore? - sprawdzamy czy nie jest zajete (dwa mutexy na wejscia?) / semafory?

    //obsluga bycia w ulu przez jakis czas // sleep

    //opuszczenie ula, inkrementacja liczby odwiedzin, zwolnienie miejsca w ulu -> 

    //sprawdzenie ile razy pszczola byla w ulu, usuniecie wątku jesli przekroczyl ilosc cykli pthread_exit(null)

}

int main()
{
    srand(time(NULL));
    int liczba_osobnikow = 10;
    pthread_t robotnice[liczba_osobnikow];
    Pszczola pszczoly[liczba_osobnikow];
    //tworzenie robotnic, krolowej i pszczelarza oraz czekanie na ich zakonczenie
    for(int i=0; i<liczba_osobnikow; i++)
    {
        pszczoly[i].pszczola_znajduje_sie_w_ulu = 0;
        pszczoly[i].licznik_odwiedzen = 0;
        pszczoly[i].liczba_cykli = rand() % 200 + 100;
    }

    for(int i=0; i<liczba_osobnikow; i++)
    {
        if(pthread_create(&robotnice[i], NULL, *robotnica, (void*)&pszczoly[i]) != 0){

            return 1;
        }
    }

    for(int i=0; i<10; i++)
    {
        if(pthread_join(robotnice[i], NULL) != 0){
            return 2;
        }
    }

    int krolowa = fork();
    int pszczelarz;
    if(krolowa == 0)
    {
        printf("Jestem krolowa!\n");
        exit(0); 
    }
    else
    {
        pszczelarz = fork();
        if(pszczelarz == 0)
        {
            printf("Jestem pszelarz!\n");
            exit(0);
        }
    }
    

    while(wait(NULL) > 0);
    return 0;
}