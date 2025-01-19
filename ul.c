#include "common.h"
#include <unistd.h>

#define LIFE_CYCLE_NUM 4
#define INSIDE_WORKING_TIME 5

static sem_t hole_1, hole_2;
static pthread_mutex_t bees_in_hive_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t available_space_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t depopulation_mutex = PTHREAD_MUTEX_INITIALIZER;
int to_reduce_num = 0;
int reduced_counter = 0;
Hive *shared_hive_state = NULL;
int sem_id;
int simulation_termination = 0;


//send pid of hive to beekepper
void send_pid_to_beekepper(const char *fifo_path) {
    if (mkfifo(fifo_path, 0600) == -1) {
        if (errno != EEXIST) {
            perror("Cannot create fifo.");
            exit(EXIT_FAILURE);
        }
    }
    
    int fifo_fd = open(fifo_path, O_WRONLY);
    if (fifo_fd == -1) {
        perror("Error while writing to fifo.");
        exit(EXIT_FAILURE);
    }
    
    // get current pid
    int hive_pid = (int)getpid();
    if (hive_pid <= 0) {
        perror("Błąd przy pobieraniu PID");
        close(fifo_fd);
        exit(EXIT_FAILURE);
    }
    
    char pid_str[20];
    if (snprintf(pid_str, sizeof(pid_str), "%d", hive_pid) < 0) {
        perror("Błąd przy konwersji PID na ciąg znaków");
        close(fifo_fd);
        exit(EXIT_FAILURE);
    }
    
    if (write(fifo_fd, pid_str, strlen(pid_str) + 1) == -1) {
        perror("Błąd zapisu do FIFO");
        close(fifo_fd);
        exit(EXIT_FAILURE);
    }
    
    if (close(fifo_fd) == -1) {
        perror("Błąd zamykania FIFO");
        exit(EXIT_FAILURE);
    }
    
    printf("PID wysłany do FIFO: %s\n", pid_str);
}

//responsible to depopulation process
void depopulation_handler(Hive* shared_hive_state, int sem_id, struct sembuf lock, struct sembuf unlock,Thread_Args* thread_args)
{
    pthread_mutex_lock(&depopulation_mutex);

    if (shared_hive_state->depopulation_flag == 1) 
    {
        reduced_counter++;
        update_logs(create_mess("Zredukowano pszczole %d z kolei o numerze: %lu", reduced_counter, (unsigned long)pthread_self()), 
            37, 
            1
        );

        if (semop(sem_id, &lock, 1) == -1) {
            perror("[DEPOPULATION] semop lock");
            pthread_mutex_unlock(&depopulation_mutex);
            pthread_exit(NULL);
        }
        shared_hive_state->current_bees = shared_hive_state->current_bees - 1;
        if (semop(sem_id, &unlock, 1) == -1) {
            perror("[DEPOPULATION] semop unlock");
            pthread_mutex_unlock(&depopulation_mutex);
            pthread_exit(NULL);
        }

        if (reduced_counter >= to_reduce_num) 
        {
            printf("KONIEC DEPOPULACJI!\n");

            if (semop(sem_id, &lock, 1) == -1) {
                perror("[DEPOPULATION] semop lock (reset depopulation)");
                pthread_mutex_unlock(&depopulation_mutex);
                pthread_exit(NULL);
            }
            shared_hive_state->depopulation_flag = 0; // turning off depopulation
            if (semop(sem_id, &unlock, 1) == -1) {
                perror("[DEPOPULATION] semop unlock (reset depopulation)");
                pthread_mutex_unlock(&depopulation_mutex);
                pthread_exit(NULL);
            }
            reduced_counter = 0;
        }

        pthread_mutex_unlock(&depopulation_mutex);

        free(thread_args->bee);
        free(thread_args);
        pthread_exit(NULL);
    } 
    else 
    {
        pthread_mutex_unlock(&depopulation_mutex);
    }
}

//responsible for enterning hive
void handle_hive_entrance(Hive* shared_hive_state, Bee* bee, int sem_id,struct sembuf lock, struct sembuf unlock)
{
    int ret = pthread_mutex_lock(&bees_in_hive_mutex);
    if (ret != 0) {
        perror("[ENTRANCE] pthread_mutex_lock");
        pthread_exit(NULL);
    }

    while (shared_hive_state->current_bees_hive >= (BEG_QUANTITY / 2)) 
    {
        //wait till enough space
        ret = pthread_cond_wait(&available_space_cond, &bees_in_hive_mutex);
        if (ret != 0) {
            perror("[ENTRANCE] pthread_cond_wait");
            pthread_mutex_unlock(&bees_in_hive_mutex);
            pthread_exit(NULL);
        }
    }

    //take up space for bee
    if (semop(sem_id, &lock, 1) == -1) {
        perror("[ENTRANCE] semop lock (current_bees_hive++)");
        pthread_mutex_unlock(&bees_in_hive_mutex);
        pthread_exit(NULL);
    }
    shared_hive_state->current_bees_hive = shared_hive_state->current_bees_hive + 1;
    if (semop(sem_id, &unlock, 1) == -1) {
        perror("[ENTRANCE] semop unlock (current_bees_hive++)");
        pthread_mutex_unlock(&bees_in_hive_mutex);
        pthread_exit(NULL);
    }

    pthread_mutex_unlock(&bees_in_hive_mutex);

    char* info;
    int entrance = rand() % 2;
    if (entrance == 1) 
    {
        if (sem_wait(&hole_1) == -1) {
            perror("[ENTRANCE] sem_wait(hole_1)");
            pthread_exit(NULL);
        }
        info = "[ROBOTNICA] Pszczoła weszła wejsciem 1";
        if (sem_post(&hole_1) == -1) {
            perror("[ENTRANCE] sem_post(hole_1)");
            pthread_exit(NULL);
        }
    } 
    else 
    {
        if (sem_wait(&hole_2) == -1) {
            perror("[ENTRANCE] sem_wait(hole_2)");
            pthread_exit(NULL);
        }
        info = "[ROBOTNICA] Pszczoła weszła wejsciem 2";
        if (sem_post(&hole_2) == -1) {
            perror("[ENTRANCE] sem_post(hole_2)");
            pthread_exit(NULL);
        }
    }

    ret = pthread_mutex_lock(&bees_in_hive_mutex);
    if (ret != 0) {
        perror("[ENTRANCE] pthread_mutex_lock 2");
        pthread_exit(NULL);
    }

    //increase if bee entered
    if (semop(sem_id, &lock, 1) == -1) {
        perror("[ENTRANCE] semop lock (capacity_control++)");
        pthread_mutex_unlock(&bees_in_hive_mutex);
        pthread_exit(NULL);
    }
    shared_hive_state->capacity_control += 1;
    if (semop(sem_id, &unlock, 1) == -1) {
        perror("[ENTRANCE] semop unlock (capacity_control++)");
        pthread_mutex_unlock(&bees_in_hive_mutex);
        pthread_exit(NULL);
    }

    //log after entering
    update_logs(create_mess("Stan: %d/%d, %s, id: %lu", shared_hive_state->capacity_control, BEG_QUANTITY/2, info, (unsigned long)pthread_self()),
        32, 
        5
    );
    pthread_mutex_unlock(&bees_in_hive_mutex);

    bee->in_hive = 1;

    //working in hive
    int ealier_leave = rand() % 5;
    sleep(INSIDE_WORKING_TIME - ealier_leave);
}

//responsible for exiting hive
void handle_hive_exit(Hive* shared_hive_state, Bee* bee, Thread_Args* thread_args,int sem_id, struct sembuf lock, struct sembuf unlock)
{
    char* info;
    int exit = rand() % 2;
    if (exit == 1) 
    {
        if (sem_wait(&hole_1) == -1) {
            perror("[EXIT] sem_wait(hole_1)");
            pthread_exit(NULL);
        }
        info = "[ROBOTNICA] Pszczola wyszla wejsciem 1";
        if (sem_post(&hole_1) == -1) {
            perror("[EXIT] sem_post(hole_1)");
            pthread_exit(NULL);
        }
    } 
    else 
    {
        if (sem_wait(&hole_2) == -1) {
            perror("[EXIT] sem_wait(hole_2)");
            pthread_exit(NULL);
        }
        info = "[ROBOTNICA] Pszczolas wyszla wejsciem 2";
        if (sem_post(&hole_2) == -1) {
            perror("[EXIT] sem_post(hole_2)");
            pthread_exit(NULL);
        }
    }

    bee->in_hive = 0;

    int ret = pthread_mutex_lock(&bees_in_hive_mutex);
    if (ret != 0) {
        perror("[EXIT] pthread_mutex_lock");
        pthread_exit(NULL);
    }

    //reducing after hive exit
    if (semop(sem_id, &lock, 1) == -1) {
        perror("[EXIT] semop lock (current_bees_hive--)");
        pthread_mutex_unlock(&bees_in_hive_mutex);
        pthread_exit(NULL);
    }
    shared_hive_state->current_bees_hive = shared_hive_state->current_bees_hive - 1;
    shared_hive_state->capacity_control -= 1;
    if (semop(sem_id, &unlock, 1) == -1) {
        perror("[EXIT] semop unlock (current_bees_hive--)");
        pthread_mutex_unlock(&bees_in_hive_mutex);
        pthread_exit(NULL);
    }

    //log after exit
    update_logs(create_mess("Stan: %d/%d, %s, id: %lu", shared_hive_state->capacity_control, BEG_QUANTITY/2, info, (unsigned long)pthread_self()),
        34, 
        5
    );

    //send signal if bee left hive
    pthread_cond_signal(&available_space_cond);
    pthread_mutex_unlock(&bees_in_hive_mutex);

    bee->visits_counter++;

    // check if bee has reached its life cycle
    if (bee->visits_counter >= bee->life_cycles && shared_hive_state->depopulation_flag != 1) 
    {
        //log if dies
        update_logs(create_mess("[ROBOTNICA] Pszczoła o numerze %lu umiera (liczba odwiedzin %d).", (unsigned long)pthread_self(), bee->visits_counter),
            31, 
            3
        );

        if (semop(sem_id, &lock, 1) == -1)
        {
            perror("[ROBOTNICA] semop lock (śmierć)");
        }

        //reduce amount of bees after death + log how many bees left
        shared_hive_state->current_bees = shared_hive_state->current_bees - 1;
        printf("[Robotnica] Po smierci pszczoly obecna liczba pszczol wynosi: %d\n", shared_hive_state->current_bees);

        if (semop(sem_id, &unlock, 1) == -1) 
        {
            perror("[ROBOTNICA] semop unlock (śmierć)");
        }

        free(bee);
        free(thread_args);
        pthread_exit(NULL);
    }

    //working outside hive
    int outside_hive_work = rand() % 5 + 1;
    sleep(outside_hive_work);
}


//thread method
void* bee_worker_func(void* arg)
{
    Thread_Args* thread_args = (Thread_Args*) arg;
    if (!thread_args) 
    {
        fprintf(stderr, "[ROBOTNICA] Błąd: argumenty wątku są NULL.\n");
        pthread_exit(NULL);
    }

    Bee* bee                = thread_args->bee;
    Hive* shared_hive_state = thread_args->shared_hive_state;
    int sem_id              = thread_args->sem_id;

    // Definicje operacji semaforowych (bez zmiany)
    struct sembuf lock   = {0, -1, 0};
    struct sembuf unlock = {0,  1, 0};

    while (1) 
    {
        // 1. check if depopulation
        depopulation_handler(shared_hive_state, sem_id, lock, unlock, thread_args);

        // 2. enters hive if beforehand was outside
        if (bee->in_hive == 0)
        {
            handle_hive_entrance(shared_hive_state, bee, sem_id, lock, unlock);
        }

        // 3. exits hive if beforehand was inside or dies if reached limit
        if (bee->in_hive == 1)
        {
            handle_hive_exit(shared_hive_state, bee, thread_args, sem_id, lock, unlock);
        }
    }

    free(bee);
    free(thread_args);
    pthread_exit(NULL);
}


//signals handling - sigusr1 -> for increasing population, sigusr2 -> for decreasing amount of bees, sigint -> for termination
void signal_handle(int sig)
{
    if (!shared_hive_state) return;
    if (sig == SIGUSR1) 
    {
        update_logs("[UL] Otrzymano SIGUSR1 -> zwiekszamy populacje!", 35, 1);
        struct sembuf lock = {0, -1, 0};
        struct sembuf unlock = {0, 1, 0};

        if (semop(sem_id, &lock, 1) == -1) {
            perror("[UL] semop lock (SIGUSR1)");
            return;
        }

        shared_hive_state->max_bees = shared_hive_state->max_bees * 2;

        if (semop(sem_id, &unlock, 1) == -1) {
            perror("[UL] semop unlock (SIGUSR1)");
            return;
        }
    }
    else if (sig == SIGUSR2) 
    {
        update_logs("[UL] Otrzymano SIGUSR2 -> zmniejszamy populacje!", 35, 1);

        if (semop(sem_id, &lock, 1) == -1) {
            perror("[UL] semop lock (SIGINT)");
            return;
        }

        to_reduce_num = shared_hive_state->current_bees / 2;

        if (semop(sem_id, &unlock, 1) == -1) {
            perror("[UL] semop unlock (SIGINT)");
            return;
        }
    }
    else if(sig == SIGINT)
    {
        struct sembuf lock = {0, -1, 0};
        struct sembuf unlock = {0, 1, 0};

        if (semop(sem_id, &lock, 1) == -1) {
            perror("[UL] semop lock (SIGINT)");
            return;
        }
        shared_hive_state->depopulation_flag = 1;
        to_reduce_num = shared_hive_state->current_bees;
        if (semop(sem_id, &unlock, 1) == -1) {
            perror("[UL] semop unlock (SIGINT)");
            return;
        }
        simulation_termination = 1;
    }
}


void setup_signal_handling(void)
{
    struct sigaction sa;
    sa.sa_handler = signal_handle;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGUSR1, &sa, NULL) == -1)
    {
        perror("[UL] sigaction error SIGUSR1");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGUSR2, &sa, NULL) == -1)
    {
        perror("[UL] sigaction error SIGUSR2");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("[UL] sigaction error SIGINT");
        exit(EXIT_FAILURE);
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

    //struct for getting info from mq
    msgbuf eggs_obtained;

    send_pid_to_beekepper(FIFO_PATH);

    shared_hive_state = (Hive*) shmat(shm_id, NULL, 0);
    if (shared_hive_state == (void*)-1) {
        perror("[UL] shmat");
        exit(EXIT_FAILURE);
    }

    struct sembuf lock = {0, -1, 0};
    struct sembuf unlock = {0, 1, 0};
    
    //semaphores init (holes)
    if (sem_init(&hole_1, 0, 1) == -1) {
        perror("[UL] sem_init wejscie1");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&hole_2, 0, 1) == -1) {
        perror("[UL] sem_init wejscie2");
        exit(EXIT_FAILURE);
    }

    //handling signal from beekepper
    setup_signal_handling();
    
    //creating static bees - full population
    pthread_t* bee_workers = (pthread_t*) calloc(BEG_QUANTITY, sizeof(pthread_t));
    if (!bee_workers) {
        fprintf(stderr, "[UL] Błąd alokacji robotnice_tab.\n");
        if (shmdt(shared_hive_state) == -1) {
            perror("[UL] shmdt");
        }
        sem_destroy(&hole_1);
        sem_destroy(&hole_2);
        pthread_mutex_destroy(&bees_in_hive_mutex);
        pthread_cond_destroy(&available_space_cond);
        pthread_mutex_destroy(&depopulation_mutex);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < BEG_QUANTITY; i++) 
    {
        Bee* bee = (Bee*) malloc(sizeof(Bee));
        if (!bee) 
        {
            fprintf(stderr, "[UL] Błąd alokacji pszczoły.\n");
            continue;
        }
        
        bee->visits_counter = 0;
        bee->life_cycles    = LIFE_CYCLE_NUM;
        bee->in_hive        = 0;

        if (semop(sem_id, &lock, 1) == -1) {
            perror("[UL] semop lock (tworzenie robotnic)");
            free(bee);
            continue;
        }
        shared_hive_state->current_bees = shared_hive_state->current_bees + 1;
        if (semop(sem_id, &unlock, 1) == -1) {
            perror("[UL] semop unlock (tworzenie robotnic)");
            free(bee);
            continue;
        }

        // arguments for thread
        Thread_Args* args = (Thread_Args*) malloc(sizeof(Thread_Args));
        if (!args) 
        {
            fprintf(stderr, "[UL] Błąd alokacji Argumenty_Watku.\n");
            free(bee);
            continue;
        }

        args->bee                = bee;
        args->sem_id             = sem_id;
        args->shared_hive_state  = shared_hive_state;

        // creating thread
        if (pthread_create(&bee_workers[i], NULL, bee_worker_func, (void*) args) != 0) {
            perror("[UL] pthread_create (bee_workers)");
            free(args);
            free(bee);
            continue;
        }

        // detaching thread
        if (pthread_detach(bee_workers[i]) != 0) {
            perror("[UL] pthread_detach (bee_workers)");
            free(args);
            free(bee);
            continue;
        }

        update_logs(create_mess("Stworzono %d\n", i+1), 1, 1);

        //if static population has been created - start simulation
        if(i + 1 == BEG_QUANTITY)
        {
            if (semop(sem_id, &lock, 1) == -1) {
                perror("[UL] semop lock (start_simulation)");
            }
            shared_hive_state->start_simulation = 1;
            if (semop(sem_id, &unlock, 1) == -1) {
                perror("[UL] semop unlock (start_simulation)");
            }
        }
    }
    free(bee_workers);

    while (!simulation_termination) {

        if (msgrcv(msqid, &eggs_obtained, sizeof(msgbuf) - sizeof(long), MSG_TYPE_EGGS, 0) == -1) {
            if (errno == EINTR) {
                if (simulation_termination) {
                    break;
                }
                printf("[UL] msgrcv interrupted by signal, retrying...\n");
                continue;
            }
            perror("[UL] msgrcv error");
        }
        
        //creating new bees - receiving info from queen
        update_logs(create_mess("Krolowa zniosla %d jaj.", eggs_obtained.eggs_num), 33, 1);

        pthread_t* new_bees = (pthread_t*) calloc(eggs_obtained.eggs_num, sizeof(pthread_t));
        if (!new_bees) 
        {
            fprintf(stderr, "[UL] Blad alokacji dla nowych robotnic.\n");
            continue;
        }
            
        for (int i = 0; i < eggs_obtained.eggs_num; i++) 
        {
            Bee* bee = (Bee*) malloc(sizeof(Bee));
            if (!bee) 
            {
                fprintf(stderr, "[UL] Błąd alokacji pszczoły (nowej).\n");
                continue;
            }

            bee->visits_counter = 0;
            bee->life_cycles    = LIFE_CYCLE_NUM;
            bee->in_hive        = 1; 

            Thread_Args* args = (Thread_Args*) malloc(sizeof(Thread_Args));
            if (!args) 
            {
                fprintf(stderr, "[UL] Błąd alokacji Argumenty_Watku (nowej pszczoły).\n");
                free(bee);
                continue;
            }

            args->bee               = bee;
            args->sem_id            = sem_id;
            args->shared_hive_state = shared_hive_state;

            //creating
            if (pthread_create(&new_bees[i], NULL, bee_worker_func, (void*)args) != 0) {
                perror("[UL] pthread_create (nowa pszczola)");
                free(args);
                free(bee);
                continue;
            }

            //detachnig
            if (pthread_detach(new_bees[i]) != 0) {
                perror("[UL] pthread_detach (nowa pszczola)");
                free(args);
                free(bee);
                continue;
            }
        }
        free(new_bees);

        update_logs(create_mess("[UL] Stan -> obecna liczba poszczol = %d, maksymalnie = %d", shared_hive_state->current_bees, shared_hive_state->max_bees), 37, 1);
    }

    //closing all threads
    //usleep(100);

    while(1)
    {
        if(shared_hive_state->current_bees == 0)
        {
            printf("Zakonczono, obecna l.pszczol: %d\n", shared_hive_state->current_bees);
            break;            
        }
        usleep(10000);
    }

    //cleaning
    if (shmdt(shared_hive_state) == -1) 
    {
        perror("[UL] shmdt");
    }

    if (sem_destroy(&hole_1) == -1) {
        perror("[UL] sem_destroy(hole_1)");
    }

    if (sem_destroy(&hole_2) == -1) {
        perror("[UL] sem_destroy(hole_2)");
    }

    if (pthread_mutex_destroy(&bees_in_hive_mutex) != 0) {
        perror("[UL] pthread_mutex_destroy(bees_in_hive_mutex)");
    }

    if (pthread_cond_destroy(&available_space_cond) != 0) {
        perror("[UL] pthread_cond_destroy(available_space_cond)");
    }

    if (pthread_mutex_destroy(&depopulation_mutex) != 0) {
        perror("[UL] pthread_mutex_destroy(depopulation_mutex)");
    }

    update_logs("[Ul] Kończę pracę.", 41, 1);
    return 0;
}
