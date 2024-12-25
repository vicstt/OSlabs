#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h> 

#define SHM_SIZE 1024  
#define SEM_NAME "/my_semaphore"  

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <имя_файла>\n", argv[0]);
        exit(1);
    }

    int shm_id = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("shmget");
        exit(1);
    }

    sem_t *semaphore = sem_open(SEM_NAME, O_CREAT, 0644, 0);
    if (semaphore == SEM_FAILED) {
        perror("sem_open");
        shmctl(shm_id, IPC_RMID, NULL);  
        exit(1);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        shmctl(shm_id, IPC_RMID, NULL);  
        sem_close(semaphore);
        sem_unlink(SEM_NAME);
        exit(1);
    }

    if (pid == 0) {
        char shm_id_str[10];
        sprintf(shm_id_str, "%d", shm_id);  
        execl("./child", "child", shm_id_str, argv[1], NULL); 
        perror("execl");  
        shmctl(shm_id, IPC_RMID, NULL);  
        sem_close(semaphore);
        sem_unlink(SEM_NAME);
        exit(1);
    } else {
        char *shm_ptr = shmat(shm_id, NULL, 0);
        if (shm_ptr == (char *) -1) {
            perror("shmat");
            shmctl(shm_id, IPC_RMID, NULL);  
            sem_close(semaphore);
            sem_unlink(SEM_NAME);
            exit(1);
        }

        printf("Введите числа через пробел: ");
        fgets(shm_ptr, SHM_SIZE, stdin);

        if (sem_post(semaphore) == -1) {
            perror("sem_post");
        }

        wait(NULL);

        shmdt(shm_ptr);
        shmctl(shm_id, IPC_RMID, NULL);

        sem_close(semaphore);
        sem_unlink(SEM_NAME);
    }

    return 0;
}