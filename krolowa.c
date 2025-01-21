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

/**
 * @brief Sets the simulation termination flag.
 *
 * This function handles the SIGINT signal by setting the termination flag so that
 * the loops in the program can terminate.
 *
 * @param sig The received signal number.
 */
void handle_sigint(int sig)
{
    (void)sig;
    simulation_termination = 1;
}

/**
 * @brief Configures SIGINT signal handling.
 *
 * This function sets up a signal handler for SIGINT using the sigaction structure.
 * In case of an error, the program terminates.
 */
void setup_signal_handling(void)
{
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("[KRÓLOWA] sigaction error");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Safely locks a semaphore.
 *
 * This function attempts to perform a lock operation on a semaphore.
 * If the operation is interrupted by a signal (errno set to EINTR),
 * it retries the operation.
 *
 * @param sem_id The semaphore identifier.
 * @return Returns 0 on success, or -1 if an error occurs.
 */
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

/**
 * @brief Safely unlocks a semaphore.
 *
 * This function attempts to perform an unlock operation on a semaphore.
 * If the operation is interrupted by a signal (errno set to EINTR),
 * it retries the operation.
 *
 * @param sem_id The semaphore identifier.
 * @return Returns 0 on success, or -1 if an error occurs.
 */
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

/**
 * @brief Main function of the "krolowa" process.
 *
 * The main function:
 * - Checks the correctness of the provided arguments.
 * - Initializes the random number generator.
 * - Attaches to the shared memory.
 * - Configures signal handling.
 * - In a loop, randomly determines the number of eggs to lay.
 *   If there is space in the hive, the hive resources are updated and a message
 *   (via message queue) about the laid eggs is sent.
 * - Upon simulation termination, the shared memory is detached and a log message is recorded.
 *
 * @param argc Number of arguments.
 * @param argv Array of arguments. The expected format is: <sem_id> <fd_pipe> <shm_id> <msqid>.
 * @return Returns 0 on successful termination.
 */
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
        int laid_eggs_rand = rand() % 25 + 5;

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

            update_logs(create_mess("[KRÓLOWA] Obecna - %d | Obecna ul - %d | Maks - %d | Stan początkowy - %d\n",
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
        perror("[KRÓLOWA] shmdt");
    }

    update_logs("[KRÓLOWA] Koncze prace.", 41, 1);

    return 0;
}
