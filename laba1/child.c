#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
    char buffer[4096];
    char *token;
    int sum;

    if (argc < 2) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "Необходимо указать имя файла в качестве аргумента.\n");
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        exit(EXIT_FAILURE);
    }

    char *filename = argv[1];

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0; 
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
    return 0;
}
