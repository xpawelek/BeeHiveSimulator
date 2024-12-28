#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

int main()
{
    printf("Jestem pszelarz xaxa!\n");
    sleep(3);
    key_t klucz = ftok("./unikalny_klucz.txt", 65);
    int shm_id = shmget(klucz, 1024, 0666);
    if (shm_id == -1) {
        perror("shmget");
        exit(1);
    }

    char *shm_ptr = (char *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (char *)-1) {
        perror("shmat");
        return 1;
    }

    struct sembuf lock = {0, -1, 0};
    struct sembuf unlock = {0, 1, 0};

    while(1)
    {
        semop(shm_id, &lock, 1);
        printf("Pszczelarz odczytał: %d\n", *shm_ptr);
        semop(shm_id, &unlock, 1);
        sleep(1);
    }

    if (shmdt(shm_ptr) == -1) {
        perror("shmdt");
        return 1;
    }

    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        return 1;
    }
    return 0;
}