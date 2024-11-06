#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
    int pipe1[2];
    pid_t pid;
    char buffer[100];

    if (argc < 2) {
        char error_msg[100];
        snprintf(error_msg, sizeof(error_msg), "Необходимо указать имя файла в качестве аргумента.\n");
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        exit(EXIT_FAILURE);
    }

    if (pipe(pipe1) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);

    } else if (pid == 0) { 
        close(pipe1[1]); 
        dup2(pipe1[0], STDIN_FILENO); 
        close(pipe1[0]); 
        char *args[] = {"./child", argv[1], NULL}; 
        execv(args[0], args);
        perror("execv"); 
        exit(EXIT_FAILURE);
        
    } else { 
        close(pipe1[0]); 
        while (1) {
            write(STDOUT_FILENO, "Введите числа (или end для завершения): ", strlen("Введите числа (или end для завершения): "));
            read(STDIN_FILENO, buffer, sizeof(buffer));
            buffer[strcspn(buffer, "\n")] = 0; 

            if (strcmp(buffer, "end") == 0) {
                break;
            }

            write(pipe1[1], buffer, strlen(buffer));
            write(pipe1[1], "\n", 1); 
        }

        close(pipe1[1]); 
        wait(NULL);
    }

    return 0;
}