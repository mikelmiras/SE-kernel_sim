#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "queue.h"

#define WORKER_THREADSS 5 // Number of receiver threads
#define MAX_PROCESSES 100 // Maximum number of processes


ProcessQueue process_queue = {
    .front = 0,
    .rear = 0, // Initialize rear to 0
    .size = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER,
};

pthread_mutex_t clock_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t clock_signal = PTHREAD_COND_INITIALIZER;

pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t timer_signal = PTHREAD_COND_INITIALIZER;

int clock_tick = 0;



// Clock thread function
void *clock_thread(void *arg)
{
    int interval_ms = *(int *)arg;

    while (1)
    {
        usleep(interval_ms * 1000); // Sleep for interval_ms milliseconds

        pthread_mutex_lock(&clock_mutex);
        clock_tick++; // Increment clock tick
        printf("Clock: Tick %d\n", clock_tick);
        pthread_cond_broadcast(&clock_signal); // Signal all waiting threads
        pthread_mutex_unlock(&clock_mutex);
    }

    return NULL;
}

void *process_generator_thread(void *arg)
{
    int pid_counter = 1;
    while (1)
    {
        pthread_mutex_lock(&timer_mutex);
        pthread_cond_wait(&timer_signal, &timer_mutex);
        printf("Process Generator: Generating new process\n");
        pthread_mutex_unlock(&timer_mutex);

        PCB new_process = {
            .pid = pid_counter++,
            .burst_time = rand() % 500 + 100,
            .state = READY
            };
        enqueue_process(&process_queue, new_process);
    }
}

// Receiver thread function
void *worker_thread(void *arg)
{
    int thread_id = *(int *)arg;

    while (1)
    {
        pthread_mutex_lock(&clock_mutex);
        pthread_cond_wait(&clock_signal, &clock_mutex);
        printf("Thread %d: Clock received\n", thread_id);
        pthread_mutex_unlock(&clock_mutex);
    }

    return NULL;
}

// Timer thread function
void *timer_thread(void *arg)
{
    int ticks_interval = *(int *)arg;
    int last_tick = 0;

    while (1)
    {
        pthread_mutex_lock(&clock_mutex);
        while (clock_tick < last_tick + ticks_interval)
        {
            pthread_cond_wait(&clock_signal, &clock_mutex); // Wait for clock signal
        }
        pthread_mutex_lock(&timer_mutex);
        pthread_cond_broadcast(&timer_signal); // Signal all waiting threads
        pthread_mutex_unlock(&timer_mutex);

        last_tick = clock_tick; // Update the last tick
        printf("Timer: %d clock ticks passed\n", ticks_interval);
        pthread_mutex_unlock(&clock_mutex);
    }

    return NULL;
}

int main()
{
    pthread_t clock_tid;
    pthread_t receiver_tids[WORKER_THREADSS];
    pthread_t timer_tid;
    pthread_t process_generator_tid;
    int interval_ms = 500;  // Clock interval in milliseconds
    int ticks_interval = 3; // Timer interval in clock ticks
    int thread_ids[WORKER_THREADSS];

    // Create the clock thread
    if (pthread_create(&clock_tid, NULL, clock_thread, &interval_ms) != 0)
    {
        perror("Failed to create clock thread");
        exit(EXIT_FAILURE);
    }

    // Create the receiver threads
    for (int i = 0; i < WORKER_THREADSS; i++)
    {
        thread_ids[i] = i + 1;
        if (pthread_create(&receiver_tids[i], NULL, worker_thread, &thread_ids[i]) != 0)
        {
            perror("Failed to create worker thread");
            exit(EXIT_FAILURE);
        }
    }

    // Create the timer thread
    if (pthread_create(&timer_tid, NULL, timer_thread, &ticks_interval) != 0)
    {
        perror("Failed to create timer thread");
        exit(EXIT_FAILURE);
    }

    pthread_create(&process_generator_tid, NULL, process_generator_thread, NULL);

    pthread_join(process_generator_tid, NULL);
    pthread_join(clock_tid, NULL);
    for (int i = 0; i < WORKER_THREADSS; i++)
    {
        pthread_join(receiver_tids[i], NULL);
    }
    pthread_join(timer_tid, NULL);

    return 0;
}
