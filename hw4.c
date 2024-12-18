//
// Created by Jouji Takeda on 11/24/24.
//
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_THREADS 3
#define NUM_RANDOMS 500
#define NUM_CHILD_THREADS 10
#define NUM_READS_PER_THREAD (NUM_RANDOMS * NUM_THREADS / NUM_CHILD_THREADS)

int pipefd[2];
pthread_mutex_t mutex;

typedef struct {
    int thread_id;
    int *sum;
}   thread_data;

void *rng(void *arg) {
    int numbers[NUM_RANDOMS];
    for (int i = 0; i < NUM_RANDOMS; i++) {
        numbers[i] = rand() % 1001;
    }
   pthread_mutex_lock(&mutex);
    if(write(pipefd[1], &numbers, sizeof(numbers)) == -1) {
        perror("write");
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void *calculate_sum(void *arg) {
    thread_data *data = (thread_data *) arg;
    int *sum = data -> sum; // Use a pointer to store the sum
    int numbers[NUM_READS_PER_THREAD];

    if(read(pipefd[0], numbers, sizeof(numbers)) == -1) {
        perror("read");
    }
    for (int i = 0; i < NUM_READS_PER_THREAD; i++) {
        *sum += numbers[i];
    }

    return NULL;
}

void thread_count(int parent_count, int child_count) {
    FILE *file = fopen("thread_count.txt", "w");
    if (file != NULL) {
        fprintf(file, "Parent Threads Created: %d\n", parent_count);
        fprintf(file, "Child Threads Created: %d\n", child_count);
        fclose(file);
    } else {
        perror("Error file could not be opened and written");
    }
}

int main() {
    srand(time(NULL));

    pthread_t threads[NUM_THREADS];
    pthread_t child_threads[NUM_CHILD_THREADS];
    int total_sum = 0;
    thread_data *args[NUM_CHILD_THREADS];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }


    if (pthread_mutex_init(&mutex, NULL) != 0) {
        perror("mutex init");
        exit(EXIT_FAILURE);
    }


    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, rng, NULL) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }


    pid_t child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) {
        close(pipefd[1]);


        for (int i = 0; i < NUM_CHILD_THREADS; i++) {
            args[i] = malloc(sizeof(thread_data));
            args[i]->thread_id = i;
            args[i]->sum = malloc(sizeof(int));
            *(args[i]->sum) = 0;  // Initialize sum to 0

            if (pthread_create(&child_threads[i], NULL, calculate_sum, args[i]) != 0) {
                perror("pthread_create");
                exit(EXIT_FAILURE);
            }
        }


        for (int i = 0; i < NUM_CHILD_THREADS; i++) {
            pthread_join(child_threads[i], NULL);
            total_sum += *(args[i]->sum);
            free(args[i]->sum);
            free(args[i]);
        }


        double average = (double)total_sum / NUM_CHILD_THREADS;


        FILE *file = fopen("average.txt", "w");
        if (file == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
        fprintf(file, "Average of sums calculated by child process: %.2f\n", average);
        fclose(file);


        close(pipefd[0]);
        exit(EXIT_SUCCESS);
    } else {
        close(pipefd[0]);


        for (int i = 0; i < NUM_THREADS; i++) {
            pthread_join(threads[i], NULL);
        }


        close(pipefd[1]);


        int status;
        waitpid(child_pid, &status, 0);
        if (WIFEXITED(status)) {
            printf("Child process completed successfully.\n");
        }
        thread_count(NUM_THREADS, NUM_CHILD_THREADS);
    }


    pthread_mutex_destroy(&mutex);
    return 0;
}