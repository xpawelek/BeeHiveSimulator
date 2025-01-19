#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
#include <signal.h>

int simulation_termination = 0;

//sets termination flag
void handle_sigint(int sig)
{
    (void)sig;
    simulation_termination = 1;
}

void setup_signal_handling(void)
{
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("[KROLOWA] sigaction error");
        exit(EXIT_FAILURE);
    }
}

int sem_lock_safe(int sem_id)
{
    while (semop(sem_id, &lock, 1) == -1)
    {
        if (errno == EINTR) 
        {
            fprintf(stderr, "[KRÓLOWA] semop przerwane, ponawiam próbę...\n");
            continue;
        }
        perror("[KRÓLOWA] semop lock error");
        return -1;
    }
    return 0;
}

int sem_unlock_safe(int sem_id)
{
    while (semop(sem_id, &unlock, 1) == -1)
    {
        if (errno == EINTR) 
        {
            fprintf(stderr, "[KRÓLOWA] semop przerwane, ponawiam próbę...\n");
            continue;
        }
        perror("[KRÓLOWA] semop lock error");
        return -1;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    if (argc < 5) 
    {
        fprintf(stderr, "[KRÓLOWA] Sposób użycia: %s <sem_id> <fd_pipe> <shm_id> <msqid>\n", argv[0]);
        return 1;
    }

    srand(time(NULL));

    int sem_id = atoi(argv[1]);
    //int pipe_fd = atoi(argv[2]);  
    int shm_id = atoi(argv[3]);
    int msqid = atoi(argv[4]);
    int hive_capacity;

    Hive* shared_hive_state = (Hive*) shmat(shm_id, NULL, 0);
    if (shared_hive_state == (void*) -1) 
    {
        perror("[KRÓLOWA] shmat");
        return 1;
    }

    setup_signal_handling();

    //max capacity of hive
    hive_capacity = BEG_QUANTITY / 2;

    //message queue struct to keep info about eggs
    msgbuf laid_eggs;
    laid_eggs.mtype = MSG_TYPE_EGGS;
    while (!simulation_termination) 
    {
        int laid_eggs_rand = rand() % 10 + 1;

        /* 
          int ograniczenie_ula = pojemnosc_poczatkowa - obecna_liczba_osobnikow_ul;
          int ograniczenie_populacji = maksymalna_ilosc_osobnikow - obecna_liczba_pszczol;
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
              liczba_zlozonych_jaj = rand() % 3 + 1; // Bardzo ograniczone miejsce
          }
         */

        //if there is space in hive, population and depopulation flag not set - lay eggs
        if(sem_lock_safe(sem_id) == -1)
        {
            break;
        }

        if (shared_hive_state->current_bees + laid_eggs_rand <= shared_hive_state->max_bees
            && shared_hive_state->current_bees_hive + laid_eggs_rand + 1 <= hive_capacity
            && shared_hive_state->depopulation_flag != 1
            && shared_hive_state->start_simulation == 1)
        {
            //take up space for eggs
            shared_hive_state->current_bees += laid_eggs_rand;
            shared_hive_state->current_bees_hive += laid_eggs_rand;
            shared_hive_state->capacity_control += laid_eggs_rand;

            update_logs(create_mess("[KROLOWA] Obecna - %d | Obecna ul - %d | Maks - %d | Stan początkowy - %d\n",
            shared_hive_state->current_bees,
            shared_hive_state->capacity_control,
            shared_hive_state->max_bees,
            hive_capacity), 1, 1);

            if (sem_unlock_safe(sem_id) == -1) 
            {
                break;
            }

            laid_eggs.eggs_num = laid_eggs_rand;

            //send message
            if (msgsnd(msqid, &laid_eggs, sizeof(int), 0) == -1) 
            {
                perror("[KRÓLOWA] msgsnd error");
            }

            /* // old sending by pipe
             if (write(pipe_fd, &liczba_zlozonych_jaj, sizeof(int)) == -1) {
                 perror("[KRÓLOWA] zapis pipe_fd error");
                 break;
              }
            */
        }
        else
        {
            if(sem_lock_safe(sem_id) == -1)
            {
                break;
            }
        }

        usleep(30000);
    }

    if (shmdt(shared_hive_state) == -1) 
    {
        perror("[KROLOWA] shmdt");
    }


    update_logs("[KROLOWA] Koncze prace.", 41, 1);

    return 0;
}