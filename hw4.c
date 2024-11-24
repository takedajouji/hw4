//
// Created by Jouji Takeda on 11/24/24.
//
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


#define NUM_THREADS 3
#define NUM_RANDOMS 500
#define NUM_CHILD_THREADS 10
#define NUM_READS_PER_THREAD (NUM_RANDOMS * NUM_THREADS/NUM_CHILD_THREADS)

int pipefd[2];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *rng(void *arg) {
    int i;
    int numbers[NUM_RANDOMS];

    for (i = 0; i < NUM_RANDOMS; i++) {
        numbers[i] = rand() % 1001;
    }

    pthread_mutex_lock(&mutex);
    write(pipefd[1], numbers, sizeof(numbers));
    pthread_mutex_unlock(&mutex);

    return NULL;
}

void *sum(void *arg) {
    int sum = 0;
    int numbers[NUM_READS_PER_THREAD];

    pthread_mutex_lock(&mutex);
    read(pipefd[0], numbers, sizeof(numbers));
    pthread_mutex_unlock(&mutex);

    for (int i = 0; i < NUM_READS_PER_THREAD; i++) {
        sum += numbers[i];
    }

    return (void *)(long)sum;
}



int main() {
    pthread_t threads[NUM_THREADS];
    pid_t child_pid;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, rng, NULL);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    child_pid = fork();
    if (child_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) {
        pthread_t child_threads[NUM_CHILD_THREADS];
        long total_sum = 0;

        for (int i = 0; i < NUM_CHILD_THREADS; i++) {
            pthread_create(&child_threads[i], NULL, sum, NULL);
        }

        for (int i = 0; i < NUM_CHILD_THREADS; i++) {
            long thread_sum;
            pthread_join(child_threads[i], (void **)&thread_sum);
            total_sum += thread_sum;
        }

        double average = (double)total_sum / (NUM_CHILD_THREADS * NUM_READS_PER_THREAD);

        FILE *file = fopen("avg.txt", "w");
        if (file != NULL) {
            fprintf(file, "Avg: %.2f\n", average);
            fclose(file);
        } else {
            perror("Could not open file for writing");
        }

        exit(EXIT_SUCCESS);
    } else {
        close(pipefd[1]);
        wait(NULL);
        close(pipefd[0]);

    }

    return 0;
}