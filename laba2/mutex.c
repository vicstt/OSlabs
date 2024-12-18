#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>

#define MAX_THREADS 16

typedef struct {
    int n;           
    double **A;        
    double *b;          
    int start, end;     
} ThreadData;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int active_threads_mutex = 0;  
int max_threads_mutex = 4;     

void *gauss_elimination_mutex(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    int n = data->n;
    double **A = data->A;
    double *b = data->b;

    for (int k = 0; k < n; k++) {
        pthread_mutex_lock(&mutex);
        if (active_threads_mutex < max_threads_mutex) {
            active_threads_mutex++; 
            pthread_mutex_unlock(&mutex);

            for (int i = data->start; i < data->end; i++) {
                if (i > k) {  
                    double factor = A[i][k] / A[k][k];
                    for (int j = k; j < n; j++) {
                        A[i][j] -= factor * A[k][j];
                    }
                    b[i] -= factor * b[k];
                }
            }

            pthread_mutex_lock(&mutex);
            active_threads_mutex--;
            pthread_mutex_unlock(&mutex);
        } else {
            pthread_mutex_unlock(&mutex);
            k--;  
        }
    }

    pthread_exit(NULL);
}

void back_substitution(int n, double **A, double *b, double *x) {
    for (int i = n - 1; i >= 0; i--) {
        x[i] = b[i] / A[i][i];
        for (int j = i - 1; j >= 0; j--) {
            b[j] -= A[j][i] * x[i];
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Использование: %s <размер матрицы> <максимальное количество потоков>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);           
    max_threads_mutex = atoi(argv[2]);  

    double **A = malloc(n * sizeof(double *));
    double *b = malloc(n * sizeof(double));
    double *x = malloc(n * sizeof(double));
    for (int i = 0; i < n; i++) {
        A[i] = malloc(n * sizeof(double));
        for (int j = 0; j < n; j++) {
            A[i][j] = (double)(rand() % 100);  
        }
        b[i] = (double)(rand() % 100);
    }

    pthread_t threads[MAX_THREADS];
    ThreadData thread_data[MAX_THREADS];

    int chunk_size = n / max_threads_mutex;  
    for (int i = 0; i < max_threads_mutex; i++) {
        thread_data[i].n = n;
        thread_data[i].A = A;
        thread_data[i].b = b;
        thread_data[i].start = i * chunk_size;
        thread_data[i].end = (i + 1) * chunk_size;
        pthread_create(&threads[i], NULL, gauss_elimination_mutex, &thread_data[i]);
    }

    for (int i = 0; i < max_threads_mutex; i++) {
        pthread_join(threads[i], NULL);
    }

    back_substitution(n, A, b, x);

    printf("Решение системы:\n");
    for (int i = 0; i < n; i++) {
        printf("x[%d] = %f\n", i, x[i]);
    }

    for (int i = 0; i < n; i++) {
        free(A[i]);
    }
    free(A);
    free(b);
    free(x);

    return 0;
}