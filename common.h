#ifndef COMMON_H
#define COMMON_H

#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 700
#define MESSAGE_DEFS_H

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
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <stdarg.h>

// paths and constants
#define BEG_QUANTITY 100
#define FIFO_PATH "/tmp/ul_do_pszczelarz_fifo"
#define MSG_QUEUE_PROJECT_ID 'A'
#define MSG_TYPE_EGGS 1  

// structure of message in message queue
typedef struct msgbuf {
    long mtype;  
    int eggs_num;    
} msgbuf;

// struct for bee - visits counter, life length, information if bee is in hive
typedef struct {
    int visits_counter;
    int life_cycles;
    int in_hive;
} Bee;

// struct for shared memory, how many bees is currently in hive and in general, max quantity of bees, quantity of bees at beggining, depopulation flag
typedef struct {
    int current_bees;      
    int max_bees; 
    int current_bees_hive;         
    int capacity_control;
    int start_simulation;    
    int depopulation_flag;        
} Hive;

// structure for arguments which will be sent to workers bees
typedef struct {
    Bee* bee;
    int sem_id;
    Hive* shared_hive_state;
} Thread_Args;

// sem operations - lock, unlock
struct sembuf lock;
struct sembuf unlock;


//prototypes for signals
void signal_handle(int sig);
void handle_sigint(int sig);

//functions for creating messages
char* create_mess(const char* mess, ...)
{
    va_list args;
    va_start(args, mess);

    int size = vsnprintf(NULL, 0, mess, args) + 1;

    va_end(args);

    char* buf = (char*)malloc(size);
    if (buf == NULL) {
        return NULL; // Allocation failed
    }

    va_start(args, mess);
    vsnprintf(buf, size, mess, args);
    va_end(args);

    return buf;
}

//function for logs
void update_logs(char* mess, int color, int style)
{
    char color_str[20]; 
    char style_str[20];   
    sprintf(color_str, "%d", color);
    sprintf(style_str, "%d", style);

    printf("\033[%s;%sm%s\033[0m\n", style_str, color_str, mess);
    FILE* plik_logi = fopen("plik_logi.txt","a");
    if (plik_logi == NULL){
        perror("File opening error.");
        exit(EXIT_FAILURE);
    }
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t now = tv.tv_sec;
    struct tm* cur_time = localtime(&now);
    char time_str[30];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", cur_time);
    snprintf(time_str + strlen(time_str), sizeof(time_str) - strlen(time_str), ".%03ld", tv.tv_usec / 1000);
    fprintf(plik_logi, "[%s] %s\n", time_str, mess);
    fflush(plik_logi);
    fclose(plik_logi);
}

//struct for defining signals
struct sigaction sa;

#endif
