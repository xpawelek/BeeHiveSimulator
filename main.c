#include "common.h"

int shm_id = -1;
int sem_id = -1;
int msqid = -1;
Hive* shared_hive_state = NULL;

/**
 * @brief SIGINT handler for finishing children execution.
 *
 * This function handles the SIGINT signal by waiting for all child processes to finish.
 *
 * @param sig Received signal number (unused).
 */
void handle_sigint(int sig)
{
    (void)sig;
    while (wait(NULL) > 0);
}

/**
 * @brief Initializes SIGINT signal handling.
 *
 * This function sets up a signal handler for SIGINT using sigaction.
 */
void setup_signal_handling(void)
{
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("[MAIN] sigaction");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Creates a FIFO.
 *
 * This function creates a FIFO specified by FIFO_PATH with permissions 0600.
 * If the FIFO already exists, it does nothing; otherwise, in case of error it terminates the program.
 */
void create_fifo(void)
{
    if (mkfifo(FIFO_PATH, 0600) == -1 && errno != EEXIST) 
    {
        perror("mkfifo fifo_path");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Removes the FIFO.
 *
 * This function deletes the FIFO specified by FIFO_PATH.
 * If the FIFO does not exist (ENOENT), it ignores the error.
 */
void remove_fifo(void)
{
    if (unlink(FIFO_PATH) == -1 && errno != ENOENT)
    {
        perror("[MAIN] unlink FIFO_PATH");
    }
}

/**
 * @brief Creates a shared memory segment.
 *
 * This function creates a shared memory segment using the provided key and size.
 * On failure, it terminates the program.
 *
 * @param key The key to use for shared memory.
 * @param size The size of the shared memory segment.
 * @return int The shared memory identifier.
 */
int create_shared_memory(key_t key, size_t size)
{
    int shm_id = shmget(key, size, 0600 | IPC_CREAT);
    if (shm_id == -1) 
    {
        perror("[MAIN] shmget");
        exit(EXIT_FAILURE);
    }
    return shm_id;
}

/**
 * @brief Unlinks and deletes the shared memory segment.
 *
 * This function detaches the shared memory from the process and removes it.
 *
 * @param ptr Pointer to the shared memory.
 * @param shm_id The shared memory identifier.
 */
void remove_shared_memory(Hive* ptr, int shm_id)
{
    if (ptr != (void*) -1 && shmdt(ptr) == -1)
    {
        perror("[MAIN] shmdt");
    }
    if (shmctl(shm_id, IPC_RMID, NULL) == -1)
    {
        perror("[MAIN] shmctl IPC_RMID");
    }
}

/**
 * @brief Creates a semaphore.
 *
 * This function creates a semaphore using the provided key.
 * On failure, it terminates the program.
 *
 * @param key The key to use for the semaphore.
 * @return int The semaphore identifier.
 */
int create_semaphore(key_t key)
{
    int sem_id = semget(key, 1, IPC_CREAT | 0600);
    if (sem_id == -1) 
    {
        perror("[MAIN] semget");
        exit(EXIT_FAILURE);
    }
    return sem_id;
}

/**
 * @brief Sets the semaphore value to 1.
 *
 * This function sets the value of the semaphore to 1 using semctl.
 *
 * @param sem_id The semaphore identifier.
 */
void set_semaphore_value(int sem_id)
{
    if (semctl(sem_id, 0, SETVAL, 1) == -1) 
    {
        perror("[MAIN] semctl SETVAL");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Deletes the semaphore.
 *
 * This function removes the semaphore identified by sem_id.
 *
 * @param sem_id The semaphore identifier.
 */
void remove_semaphore(int sem_id)
{
    if (semctl(sem_id, 0, IPC_RMID) == -1) 
    {
        perror("[MAIN] semctl IPC_RMID");
    }
}

/**
 * @brief Creates a message queue.
 *
 * This function creates a message queue using the provided key.
 * On failure, it terminates the program.
 *
 * @param key The key to use for the message queue.
 * @return int The message queue identifier.
 */
int create_message_queue(key_t key)
{
    int msqid = msgget(key, IPC_CREAT | 0600);
    if (msqid == -1) 
    {
        perror("[main] msgget error");
        exit(EXIT_FAILURE);
    }
    return msqid;
}

/**
 * @brief Removes the message queue.
 *
 * This function deletes the message queue identified by msqid.
 *
 * @param msqid The message queue identifier.
 */
void remove_message_queue(int msqid)
{
    if (msgctl(msqid, IPC_RMID, NULL) == -1)
    {
        perror("[MAIN] msgctl IPC_RMID");
    }
}

/**
 * @brief Cleans up resources.
 *
 * This function cleans up the shared memory, semaphore, message queue, and FIFO.
 * It also logs the end of the program.
 *
 * @param shared_hive_state Pointer to the shared hive state.
 * @param shm_id The shared memory identifier.
 * @param sem_id The semaphore identifier.
 * @param msqid The message queue identifier.
 */
void cleanup(Hive* shared_hive_state, int shm_id, int sem_id, int msqid)
{
    remove_shared_memory(shared_hive_state, shm_id);
    remove_semaphore(sem_id);
    remove_message_queue(msqid);
    remove_fifo();
    update_logs("[MAIN] Koniec programu.", 41, 1);
}

int main(int argc, char* argv[])
{  
    (void)argc;
    (void)argv;
    printf("Enter 1 for doubling population of bees - you can do it just once!\n");
    printf("Enter 2 for population reduction!\n");
    printf("Enter CTRL+C for closing simulation properly!\n");
    usleep(500000);  
    srand(time(NULL));

    setup_signal_handling();
    create_fifo();

    int pipe_lay_eggs[2];
    if (pipe(pipe_lay_eggs) == -1) 
    {
        perror("[MAIN] pipe");
        exit(EXIT_FAILURE);
    }

    key_t klucz = ftok("unikalny_klucz.txt", 65);
    if (klucz == -1) 
    {
        perror("[MAIN] ftok");
        exit(EXIT_FAILURE);
    }

    shm_id = create_shared_memory(klucz, 1024);

    shared_hive_state = (Hive*) shmat(shm_id, NULL, 0);
    if (shared_hive_state == (void*) -1) 
    {
        perror("[MAIN] shmat");
        if (shmctl(shm_id, IPC_RMID, NULL) == -1)
        {
            perror("[MAIN] shmctl IPC_RMID");
        }
        exit(EXIT_FAILURE);
    }

    memset(shared_hive_state, 0, 1024);

    sem_id = create_semaphore(klucz);

    key_t queue_key = ftok("kolejka_komunikatow_jaja.txt", MSG_QUEUE_PROJECT_ID);
    if (queue_key == -1) 
    {
        perror("[MAIN] ftok error");
        cleanup(shared_hive_state, shm_id, sem_id, -1);
        exit(EXIT_FAILURE);
    }

    msqid = create_message_queue(queue_key);

    //init hive state
    semop(sem_id, &lock, 1);
    shared_hive_state->current_bees = 0;
    shared_hive_state->current_bees_hive = 0;
    shared_hive_state->max_bees = BEG_QUANTITY;
    shared_hive_state->depopulation_flag = 0;
    shared_hive_state->capacity_control = 0;
    shared_hive_state->start_simulation = 0;
    semop(sem_id, &unlock, 1);

    if (semctl(sem_id, 0, SETVAL, 1) == -1) 
    {
        perror("[MAIN] semctl SETVAL");
        cleanup(shared_hive_state, shm_id, sem_id, msqid);
        exit(EXIT_FAILURE);
    }

    // start beekepper
    update_logs("[MAIN] Uruchamiam proces pszczelarz...", 45, 1);
    pid_t beekepper = fork();
    if (beekepper == -1) 
    {
        perror("[MAIN] fork pszczelarz");
        cleanup(shared_hive_state, shm_id, sem_id, msqid);
        exit(EXIT_FAILURE);
    }

    if (beekepper == 0) 
    {
        char sem_buf[16];
        char shm_buf[16];
        snprintf(sem_buf, sizeof(sem_buf), "%d", sem_id);
        snprintf(shm_buf, sizeof(shm_buf), "%d", shm_id);
        execl("./pszczelarz", "./pszczelarz", sem_buf, shm_buf, (char*)NULL);
        perror("[MAIN] execl pszczelarz");
        exit(EXIT_FAILURE);
    }

    // start hive
    //usleep(50000);
    update_logs("[MAIN] Uruchamiam proces ul...", 46, 1);
    pid_t hive = fork();
    if (hive == -1) 
    {
        perror("[MAIN] fork ul");
        cleanup(shared_hive_state, shm_id, sem_id, msqid);
        exit(EXIT_FAILURE);
    }

    if (hive == 0) 
    {
        close(pipe_lay_eggs[1]);
        char fd_buf[16], shm_buf[16], sem_buf[16], msqid_buf[16];
        snprintf(fd_buf,  sizeof(fd_buf),  "%d", pipe_lay_eggs[0]); // fd for reading
        snprintf(shm_buf, sizeof(shm_buf), "%d", shm_id);
        snprintf(sem_buf, sizeof(sem_buf), "%d", sem_id);
        snprintf(msqid_buf, sizeof(msqid_buf), "%d", msqid);

        execl("./ul", "./ul", fd_buf, shm_buf, sem_buf, msqid_buf, (char*)NULL);
        perror("[MAIN] execl ul");
        exit(EXIT_FAILURE);
    }

    //usleep(50000);
    // start queen
    update_logs("[MAIN] Uruchamiam proces krolowa...", 43, 1);
    pid_t queen = fork();
    if (queen == -1)
    {
        perror("[MAIN] fork krolowa");
        cleanup(shared_hive_state, shm_id, sem_id, msqid);
        exit(EXIT_FAILURE);
    }

    if (queen == 0) 
    {
        close(pipe_lay_eggs[0]);
        char sem_buf[16], fd_buf2[16], shm_buf[16], msqid_buf[16];
        snprintf(sem_buf,   sizeof(sem_buf),   "%d", sem_id);
        snprintf(fd_buf2,   sizeof(fd_buf2),   "%d", pipe_lay_eggs[1]); 
        snprintf(shm_buf,   sizeof(shm_buf),   "%d", shm_id);
        snprintf(msqid_buf, sizeof(msqid_buf), "%d", msqid);

        execl("./krolowa", "./krolowa", sem_buf, fd_buf2, shm_buf, msqid_buf, (char*)NULL);
        perror("[MAIN] execl krolowa");
        exit(EXIT_FAILURE);
    }

    // close unused fd
    close(pipe_lay_eggs[0]);
    close(pipe_lay_eggs[1]);

    //wait till all finishes
    while (wait(NULL) > 0);

    //cleanup
    cleanup(shared_hive_state, shm_id, sem_id, msqid);

    return 0;
}
