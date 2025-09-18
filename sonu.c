#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define THREAD_COUNT 100
#define BUFFER_SIZE 600

// Shared buffer (if needed)
char buffer[BUFFER_SIZE];

// Structure to pass arguments to thread
typedef struct {
    int thread_id;
    char ip[100];
    int port;
    int duration;
} thread_args_t;

void* thread_function(void* args) {
    thread_args_t* t_args = (thread_args_t*)args;
    printf("Thread %d started with IP: %s, Port: %d, Duration: %d\n",
           t_args->thread_id, t_args->ip, t_args->port, t_args->duration);
    // Simulate work
    sleep(t_args->duration);
    printf("Thread %d finished.\n", t_args->thread_id);
    free(t_args);
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: %s IP port duration\n", argv[0]);
        return 1;
    }

    char* ip = argv[1];
    int port = atoi(argv[2]);
    int duration = atoi(argv[3]);

    pthread_t threads[THREAD_COUNT];

    for (int i = 0; i < THREAD_COUNT; i++) {
        thread_args_t* args = malloc(sizeof(thread_args_t));
        args->thread_id = i + 1;
        strncpy(args->ip, ip, sizeof(args->ip));
        args->port = port;
        args->duration = duration;

        if (pthread_create(&threads[i], NULL, thread_function, args) != 0) {
            perror("Failed to create thread");
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("All threads completed.\n");
    return 0;
}