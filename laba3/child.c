#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <unistd.h>
#include <unistd.h>
#include <fcntl.h>

#define SEM_NAME "/my_semaphore"  

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Использование: %s <shm_id> <имя_файла>\n", argv[0]);
        exit(1);
    }

    int shm_id = atoi(argv[1]);  
    char *filename = argv[2];   

    sem_t *semaphore = sem_open(SEM_NAME, 0);
    if (semaphore == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    char *shm_ptr = shmat(shm_id, NULL, 0);
    if (shm_ptr == (char *) -1) {
        perror("shmat");
        sem_close(semaphore);
        exit(1);
    }

    if (sem_wait(semaphore) == -1) {
        perror("sem_wait");
    }

    char *input = shm_ptr;
    int sum = 0;
    char *token = strtok(input, " ");
    
    while (token != NULL) {
        sum += atoi(token);
        token = strtok(NULL, " ");
    }

    int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644); 
    if (fd == -1) {
        perror("open");
        shmdt(shm_ptr);
        sem_close(semaphore);
        exit(1);
    }
    char sum_str[20];
    snprintf(sum_str, sizeof(sum_str), "%d\n", sum);
    write(fd, sum_str, strlen(sum_str));
    close(fd);

    shmdt(shm_ptr);
    sem_close(semaphore);
    return 0;
}