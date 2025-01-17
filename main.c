#include "common.h"

void obsluga_sigint(int sig)
{
    while(wait(NULL) > 0);
}

int main(int argc, char* argv[])
{   
    printf("Enter 1 for doubling population of bees - you can do it just once!\nEnter 2 for population reduction!\nEnter CTRL+C for closing simulation properly!\n");
    sleep(1);
    srand(time(NULL));

    if (mkfifo(FIFO_PATH, 0600) == -1  && errno != EEXIST) {
        perror("mkfifo fifo_path");
        exit(EXIT_FAILURE);
    }
    // potok (krolowa -> ul)
    int pipe_skladanie_jaj[2];
    if (pipe(pipe_skladanie_jaj) == -1) {
        perror("[MAIN] pipe");
        exit(EXIT_FAILURE);
    }

    // tworzymy pam. dzielona
    key_t klucz = ftok("unikalny_klucz.txt", 65);
    if (klucz == -1) {
        perror("[MAIN] ftok");
        exit(EXIT_FAILURE);
    }

    int shm_id = shmget(klucz, 1024, 0666 | IPC_CREAT);
    if (shm_id == -1) {
        perror("[MAIN] shmget");
        exit(EXIT_FAILURE);
    }

    Stan_Ula* stan_ula_do_przekazania = (Stan_Ula*) shmat(shm_id, NULL, 0);
    if (stan_ula_do_przekazania == (void*) -1) {
        perror("[MAIN] shmat");
        exit(EXIT_FAILURE);
    }

    memset(stan_ula_do_przekazania, 0, 1024);

    struct sigaction sa;
    sa.sa_handler = obsluga_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);

    // zestaw semforow V, ustawiamy a 1
    int sem_id = semget(klucz, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("[MAIN] semget");
        exit(EXIT_FAILURE);
    }

    key_t klucz_kolejka = ftok("kolejka_komunikatow_jaja.txt", MSG_QUEUE_PROJECT_ID);
    if (klucz_kolejka == -1) {
        perror("[MAIN] ftok error");
        exit(EXIT_FAILURE);
    }

    int msqid = msgget(klucz_kolejka, IPC_CREAT | 0666);
    if (msqid == -1) {
        perror("[main] msgget error");
        exit(EXIT_FAILURE);
    }


    semop(sem_id, &lock, 1);
    stan_ula_do_przekazania->obecna_liczba_pszczol = 0;
    stan_ula_do_przekazania->obecna_liczba_pszczol_ul = 0;
    stan_ula_do_przekazania->maksymalna_ilosc_osobnikow = POCZATKOWA_ILOSC_PSZCZOL;
    stan_ula_do_przekazania->stan_poczatkowy = POCZATKOWA_ILOSC_PSZCZOL;
    stan_ula_do_przekazania->depopulacja_flaga = 0;
    semop(sem_id, &unlock, 1);

    if (semctl(sem_id, 0, SETVAL, 1) == -1) {
        perror("[MAIN] semctl SETVAL");
        exit(EXIT_FAILURE);
    }

    // uruchamiamy pszelarza
    aktualizacja_logow("[MAIN] Uruchamiam proces pszczelarz...", 45, 1);
    pid_t pid_pszczelarz = fork();
    if (pid_pszczelarz == -1) {
        perror("[MAIN] fork pszczelarz");
        exit(EXIT_FAILURE);
    }

    if (pid_pszczelarz == 0) 
    {
        char sem_buf[16];
        char shm_buf[16];
        snprintf(sem_buf, sizeof(sem_buf), "%d", sem_id);
        snprintf(shm_buf, sizeof(shm_buf), "%d", shm_id);
        execl("./pszczelarz", "./pszczelarz", sem_buf, shm_buf, (char*)NULL);
        perror("[MAIN] execl pszczelarz");
        exit(EXIT_FAILURE);
    }

    // uruchamiamy proces ul
    //usleep(500000);
    aktualizacja_logow("[MAIN] Uruchamiam proces ul...", 46, 1);
    pid_t pid_ul = fork();
    if (pid_ul == -1) 
    {
        perror("[MAIN] fork ul");
        exit(EXIT_FAILURE);
    }

    if (pid_ul == 0) 
    {
        close(pipe_skladanie_jaj[1]);
        char fd_buf[16], shm_buf[16], sem_buf[16], msqid_buf[16];
        snprintf(fd_buf,  sizeof(fd_buf),  "%d", pipe_skladanie_jaj[0]); // fd do odczytu
        snprintf(shm_buf, sizeof(shm_buf), "%d", shm_id);
        snprintf(sem_buf, sizeof(sem_buf), "%d", sem_id);
        snprintf(msqid_buf, sizeof(msqid), "%d", msqid);

        execl("./ul", "./ul", fd_buf, shm_buf, sem_buf, msqid_buf, (char*)NULL);
        perror("[MAIN] execl ul");
        exit(EXIT_FAILURE);
    }

    //usleep(500000);
    // uruchamiamy proces krolowa
    aktualizacja_logow("[MAIN] Uruchamiam proces krolowa...", 43, 1);
    pid_t pid_krolowa = fork();
    if (pid_krolowa == -1)
    {
        perror("[MAIN] fork krolowa");
        exit(EXIT_FAILURE);
    }

    if (pid_krolowa == 0) 
    {
        close(pipe_skladanie_jaj[0]);
        char sem_buf[16], fd_buf2[16], shm_buf[16], msqid_buf[16];
        snprintf(sem_buf, sizeof(sem_buf), "%d", sem_id);
        snprintf(fd_buf2,  sizeof(fd_buf2),  "%d", pipe_skladanie_jaj[1]); 
        snprintf(shm_buf, sizeof(shm_buf), "%d", shm_id);
        snprintf(msqid_buf, sizeof(msqid), "%d", msqid);

        execl("./krolowa", "./krolowa", sem_buf, fd_buf2, shm_buf, msqid_buf, (char*)NULL);
        perror("[MAIN] execl krolowa");
        exit(EXIT_FAILURE);
    }

    close(pipe_skladanie_jaj[0]);
    close(pipe_skladanie_jaj[1]);

    while(wait(NULL) > 0);

    // sprzatamy pamiec dzielona
    if (shmdt(stan_ula_do_przekazania) == -1) {
        perror("[MAIN] shmdt");
    }
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("[MAIN] shmctl IPC_RMID");
    }

    // Usuwamy semafor
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("[MAIN] semctl IPC_RMID");
    }

    //sprzatamy kolejke
    msgctl(msqid, IPC_RMID, NULL);
    unlink(FIFO_PATH);

    aktualizacja_logow("[MAIN] Koniec programu.", 41, 1);
    return 0;
}