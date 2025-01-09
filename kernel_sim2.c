#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define RECEIVER_THREADS 5  // Number of receiver threads
#define MAX_PROCESSES 100   // Maximum number of processes

typedef enum
{
    READY,
    RUNNING,
    FINISHED
} ProcessState;

typedef struct
{
    int pid;
} PCB;

typedef struct
{
    PCB queue[MAX_PROCESSES]; // Arreglo de procesos
    int front, rear, size;    // Índices y tamaño actual
    pthread_mutex_t mutex;    // Mutex para sincronización
    pthread_cond_t cond;      // Condición para notificar disponibilidad
} ProcessQueue;


pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t clock_signal = PTHREAD_COND_INITIALIZER;
int clock_tick = 0;

// Clock thread function
void* clock_thread(void* arg) {
    int interval_ms = *(int*)arg;

    while (1) {
        usleep(interval_ms * 1000);  // Sleep for interval_ms milliseconds

        pthread_mutex_lock(&lock);
        clock_tick++;  // Increment clock tick
        printf("Clock: Tick %d\n", clock_tick);
        pthread_cond_broadcast(&clock_signal);  // Signal all waiting threads
        pthread_mutex_unlock(&lock);
    }

    return NULL;
}

// Receiver thread function
void* receiver_thread(void* arg) {
    int thread_id = *(int*)arg;

    while (1) {
        pthread_mutex_lock(&lock);
        pthread_cond_wait(&clock_signal, &lock);  // Wait for clock signal
        printf("Thread %d: Clock received\n", thread_id);
        pthread_mutex_unlock(&lock);
    }

    return NULL;
}

// Timer thread function
void* timer_thread(void* arg) {
    int ticks_interval = *(int*)arg;
    int last_tick = 0;

    while (1) {
        pthread_mutex_lock(&lock);
        while (clock_tick < last_tick + ticks_interval) {
            pthread_cond_wait(&clock_signal, &lock);  // Wait for clock signal
        }
        last_tick = clock_tick;  // Update the last tick
        printf("Timer: %d clock ticks passed\n", ticks_interval);
        pthread_mutex_unlock(&lock);
    }

    return NULL;
}

int main() {
    pthread_t clock_tid;
    pthread_t receiver_tids[RECEIVER_THREADS];
    pthread_t timer_tid;
    int interval_ms = 500;       // Clock interval in milliseconds
    int ticks_interval = 3;      // Timer interval in clock ticks
    int thread_ids[RECEIVER_THREADS];

    // Create the clock thread
    if (pthread_create(&clock_tid, NULL, clock_thread, &interval_ms) != 0) {
        perror("Failed to create clock thread");
        exit(EXIT_FAILURE);
    }

    // Create the receiver threads
    for (int i = 0; i < RECEIVER_THREADS; i++) {
        thread_ids[i] = i + 1;
        if (pthread_create(&receiver_tids[i], NULL, receiver_thread, &thread_ids[i]) != 0) {
            perror("Failed to create receiver thread");
            exit(EXIT_FAILURE);
        }
    }

    // Create the timer thread
    if (pthread_create(&timer_tid, NULL, timer_thread, &ticks_interval) != 0) {
        perror("Failed to create timer thread");
        exit(EXIT_FAILURE);
    }

    // Join threads (not necessary as the program runs indefinitely)
    pthread_join(clock_tid, NULL);
    for (int i = 0; i < RECEIVER_THREADS; i++) {
        pthread_join(receiver_tids[i], NULL);
    }
    pthread_join(timer_tid, NULL);

    // Cleanup (not reachable in this example, but good practice)
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&clock_signal);

    return 0;
}
