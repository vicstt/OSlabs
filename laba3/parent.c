#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>

#define SHM_SIZE 4096
#define SEM_NAME "/semaphore"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Необходимо указать имя файла в качестве аргумента.\n");
        exit(EXIT_FAILURE);
    }

    int shm_fd = shm_open("/shared_memory", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    void *shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        char *args[] = {"./child", argv[1], NULL};
        execv(args[0], args);
        perror("execv");
        exit(EXIT_FAILURE);
    } else {
        char buffer[100];
        while (1) {
            printf("Введите числа (или end для завершения): ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = 0; 

            sem_wait(sem);
            strncpy((char *)shm_ptr, buffer, SHM_SIZE);
            sem_post(sem);

            if (strcmp(buffer, "end") == 0) {
                break;
            }
        }

        wait(NULL);

        sem_close(sem);
        sem_unlink(SEM_NAME);
        shm_unlink("/shared_memory");
        munmap(shm_ptr, SHM_SIZE);
    }

    return 0;
}