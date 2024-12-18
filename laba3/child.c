#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>

#define SHM_SIZE 4096
#define SEM_NAME "/semaphore"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Необходимо указать имя файла в качестве аргумента.\n");
        exit(EXIT_FAILURE);
    }

    int shm_fd = shm_open("/shared_memory", O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    void *shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    sem_t *sem = sem_open(SEM_NAME, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    char buffer[SHM_SIZE];
    char *token;
    int sum;
    char *filename = argv[1];

    while (1) {
        sem_wait(sem);
        strncpy(buffer, (char *)shm_ptr, SHM_SIZE);
        if (strcmp(buffer, "end") == 0) {
            sem_post(sem);
            break;
        }
        sem_post(sem);

        if (strspn(buffer, "0123456789 ") == strlen(buffer)) {
            sum = 0;
            token = strtok(buffer, " ");
            while (token != NULL) {
                sum += atoi(token);
                token = strtok(NULL, " ");
            }

            int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd == -1) {
                perror("open");
                exit(1);
            }
            char sum_str[20];
            snprintf(sum_str, sizeof(sum_str), "%d\n", sum);
            write(fd, sum_str, strlen(sum_str));
            close(fd);
        }
    }

    sem_close(sem);
    munmap(shm_ptr, SHM_SIZE);

    return 0;
}