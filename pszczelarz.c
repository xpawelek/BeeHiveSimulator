#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <time.h>

int zakonczenie_programu = 0;

void obsluga_sigint(int sig)
{
    zakonczenie_programu = 1;
}


int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "[PSZCZELARZ] Sposób użycia: %s <sem_id> <shm_id> \n", argv[0]);
        return 1;
    }
    srand(time(NULL));

    int sem_id = atoi(argv[1]);
    int shm_id = atoi(argv[2]);

    sa.sa_handler = obsluga_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);


    // Odbieranie PID ula
    int fd = open(FIFO_PATH, O_RDONLY);
    if (fd == -1) {
        perror("open FIFO for reading");
        exit(EXIT_FAILURE);
    }

    char buffer[100] = {0};
    if (read(fd, buffer, sizeof(buffer)) == -1) {
        perror("Blad odczytu z FIFO");
        close(fd);
        exit(EXIT_FAILURE);
    }
    close(fd);

    int pid_ul = atoi(buffer);
    printf("[PSZCZELARZ] Odebrano PID ula: %d\n", pid_ul);

    // Dolaczenie do pamieci wspoldzielonej
    Stan_Ula* stan_ula = (Stan_Ula*) shmat(shm_id, NULL, 0);
    if (stan_ula == (void*)-1) {
        perror("[PSZCZELARZ] shmat");
        return 1;
    }

    //aktualizacja_logow("oooolaaa amigo!!");


    printf("[PSZCZELARZ] Obecna liczba pszczół: %d\n", stan_ula->obecna_liczba_pszczol);


    int zwiekszono_pojemnosc = 0;
    // Monitorowanie wejscia
    fd_set read_fds;
    struct timeval timeout;

    while (!zakonczenie_programu) {
        // deskryptory plikow do monitorowania
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        // ustawiamy timeouty
        timeout.tv_sec = 5;  //czekamy max 5s
        timeout.tv_usec = 0;

        printf("[PSZCZELARZ] Czekam na dane wejsciowe...\n");
        int ret = select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &timeout);

        if (ret == -1) {
            if (zakonczenie_programu) {
                break;  // Exit if SIGINT was received
            }
            perror("[PSZCZELARZ] select error");
            break;
        } else if (ret == 0) {
            printf("[PSZCZELARZ] Brak danych wejsciowych (timeout).\n");
            continue;
        }

        // jesli mamy cos na wejsciu
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            char input[10] = {0};
            if (fgets(input, sizeof(input), stdin)) {
                if (strcmp(input, "1\n") == 0 && zwiekszono_pojemnosc != 1) {
                    zwiekszono_pojemnosc = 1;
                    kill(pid_ul, SIGUSR1);
                    printf("[PSZCZELARZ] Wyslano SIGUSR1 (zwiekszenie pojemnosci ula).\n");
                } else if (strcmp(input, "2\n") == 0 && stan_ula->depopulacja_flaga != 1) {
                    semop(sem_id, &lock, 1);
                    stan_ula->depopulacja_flaga = 1;
                    semop(sem_id, &unlock, 1);
                    kill(pid_ul, SIGUSR2);
                    printf("[PSZCZELARZ] Wyslano SIGUSR2 (zdepopulowanie ula).\n");
                } else {
                    printf("[PSZCZELARZ] Nieprawidlowa komenda.\n");
                }
            }
        }
    }

    //sprzatanie
    if (shmdt(stan_ula) == -1) {
        perror("[PSZCZELARZ] shmdt");
        return 1;
    }

     aktualizacja_logow("[Pszczelarz] Kończę pracę.", 41, 1);

    return 0;
}