#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>

int simulation_termination = 0;

//handle signal to termination
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

void setup_signal_handling(void)
{
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; 

    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("[PSZCZELARZ] sigaction error");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Reads the hive PID to be able to send a signal.
 *
 * This function opens the FIFO defined by FIFO_PATH for reading, reads the PID 
 * from it, and returns the PID as an integer.
 *
 * @return int Returns the hive PID read from FIFO, or -1 if an error occurs.
 */
int read_hive_pid(void)
{
    int fd = open(FIFO_PATH, O_RDONLY);
    if (fd == -1) 
    {
        perror("[PSZCZELARZ] open FIFO for reading");
        return -1;
    }

    char buffer[100] = {0};
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer));
    if (bytes_read == -1) 
    {
        perror("[PSZCZELARZ] Błąd odczytu z FIFO");
        close(fd);
        return -1;
    }

    close(fd);

    int hive_pid = atoi(buffer);
    return hive_pid;
}

/**
 * @brief Main function for the "pszczelarz" process.
 *
 * The main function:
 * - Checks the correctness of the provided arguments.
 * - Initializes the random number generator.
 * - Registers the SIGINT signal handler.
 * - Reads the hive PID from FIFO.
 * - Attaches to shared memory.
 * - Prints the current number of bees.
 * - In a loop, waits for input commands from stdin:
 *   - "1" sends SIGUSR1 for increasing the hive capacity (if not already increased 
 *     and if the depopulation flag is not set).
 *   - "2" sends SIGUSR2 for depopulating the hive.
 *   - Any other command prints an error message.
 * - When the simulation terminates, the shared memory is detached and a log message is recorded.
 *
 * @param argc The number of arguments.
 * @param argv Array of arguments. Expected: <sem_id> <shm_id>.
 * @return int Returns 0 on success, or 1 if an error occurs.
 */
int main(int argc, char* argv[]) 
{
    if (argc < 3) 
    {
        fprintf(stderr, "[PSZCZELARZ] Sposób użycia: %s <sem_id> <shm_id>\n", argv[0]);
        return 1;
    }

    srand(time(NULL));

    int sem_id = atoi(argv[1]);
    int shm_id = atoi(argv[2]);

    //sigint handle registration
    setup_signal_handling();

    
    int hive_pid = read_hive_pid();
    if (hive_pid == -1)
    {
        exit(EXIT_FAILURE);
    }
    printf("[PSZCZELARZ] Odebrano PID ula: %d\n", hive_pid);

    Hive* shared_hive_state = (Hive*) shmat(shm_id, NULL, 0);
    if (shared_hive_state == (void*) -1) 
    {
        perror("[PSZCZELARZ] shmat");
        return 1;
    }

    printf("[PSZCZELARZ] Obecna liczba pszczół: %d\n", shared_hive_state->current_bees);

    //flag to check if population has been already incremented
    int incremented_population = 0;

    fd_set read_fds;
    struct timeval timeout;

    //loop which waits to commands from stdin
    while (!simulation_termination) 
    {
        //descriptor to read
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        printf("[PSZCZELARZ] Czekam na dane wejsciowe (1 lub 2)...\n");
        int ret = select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &timeout);

        if (ret == -1) 
        {
            if (!simulation_termination) 
            {
                perror("[PSZCZELARZ] select error");
            }
            break;
        } 
        else if (ret == 0) 
        {
            //if timeout has passed
            printf("[PSZCZELARZ] Brak danych wejsciowych (timeout).\n");
            continue;
        }

        //check if data has been entered
        if (FD_ISSET(STDIN_FILENO, &read_fds)) 
        {
            char input[10] = {0};
            if (fgets(input, sizeof(input), stdin)) 
            {
                //if 1 - sigusr1, increment
                if (strcmp(input, "1\n") == 0 && incremented_population != 1 && shared_hive_state->depopulation_flag != 1) 
                {
                    incremented_population = 1;
                    if (kill(hive_pid, SIGUSR1) == -1)
                    {
                        perror("[PSZCZELARZ] kill(SIGUSR1) error");
                    }
                    else
                    {
                        printf("[PSZCZELARZ] Wyslano SIGUSR1 (zwiekszenie pojemnosci ula).\n");
                    }
                } 
                // 2 - sigusr2, decrement
                else if (strcmp(input, "2\n") == 0 && shared_hive_state->depopulation_flag != 1) 
                {
                    if (semop(sem_id, &lock, 1) == -1)
                    {
                        perror("[PSZCZELARZ] semop lock error");
                        break;
                    }
                    shared_hive_state->depopulation_flag = 1; //set depopulation 
                    if (semop(sem_id, &unlock, 1) == -1)
                    {
                        perror("[PSZCZELARZ] semop unlock error");
                        break;
                    }

                    if (kill(hive_pid, SIGUSR2) == -1)
                    {
                        perror("[PSZCZELARZ] kill(SIGUSR2) error");
                    }
                    else
                    {
                        printf("[PSZCZELARZ] Wyslano SIGUSR2 (zdepopulowanie ula).\n");
                    }
                } 
                else 
                {
                    printf("[PSZCZELARZ] Nieprawidlowa komenda.\n");
                }
            }
        }
    }

    if (shmdt(shared_hive_state) == -1) 
    {
        perror("[PSZCZELARZ] shmdt");
        return 1;
    }
    
    update_logs("[PSZCZELARZ] Kończę pracę.", 41, 1);

    return 0;
}
