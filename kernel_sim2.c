#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "queue.h"
#include <math.h>

#define WORKER_THREADS 4  // Number of receiver threads
#define MAX_PROCESSES 100 // Maximum number of processes
#define CLOCK_FREQUENCY 100

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
        printf("[CLOCK]: Tick %d\n", clock_tick);
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
        printf("[Process Generator]: Generating new process\n");
        pthread_mutex_unlock(&timer_mutex);
        int random_chance = (rand() % WORKER_THREADS) + 1;
        for (int i = 0; i < random_chance; i++)
        {

            PCB new_process = {
                .pid = pid_counter++,
                .burst_time = rand() % 500 + 100,
                .state = READY};
            enqueue_process(&process_queue, new_process);
        }
    }
}

void *scheduler_thread(void *arg)
{
    int pid_counter = 1;
    Worker *workers = (Worker *)arg;
    while (1)
    {
        pthread_mutex_lock(&timer_mutex);
        pthread_cond_wait(&timer_signal, &timer_mutex);
        printf("[Scheduler]: Trying to assign ready processes to available workers\n");
        pthread_mutex_unlock(&timer_mutex);

        PCB *next_process = (PCB *)malloc(sizeof(PCB));
        for (int i = 0; i < WORKER_THREADS; i++)
        {
            Worker *current = &workers[i];
            if (!current->available)
                continue;
            *next_process = dequeue_process(&process_queue);
            if (next_process->pid != -1)
            {
                printf("[Scheduler]: Assigning process PID %d to available CORE %d\n", next_process->pid, current->worker_id);
                current->process = *next_process;
                current->available = 0;
            }else{
                printf("[Scheduler]: No available processes found for CORE %d\n", current->worker_id);
            }
        }

        // FCFS: El scheduler en cada timer tick iterará por todos los workers disponibles. Si hay un worker libre, se intentará asignar un proceso a ese worker. En esta política, un worker queda libre cuando el proceso ha terminado de ejecutarse.
    }
}

// Receiver thread function
void *worker_thread(void *arg)
{
    Worker *worker = (Worker *)arg;

    while (1)
    {
        pthread_mutex_lock(&clock_mutex);
        pthread_cond_wait(&clock_signal, &clock_mutex);
        pthread_mutex_unlock(&clock_mutex);
        PCB *current_process = &(worker->process);
        if (current_process->pid == -1 || current_process->state == FINISHED)
            continue;
        int max_burst_time = fmax(current_process->burst_time, 0);
        printf("[CORE %d]: Running process PID %d. Remaining burst time: %d\n", worker->worker_id, current_process->pid, max_burst_time);
        usleep(CLOCK_FREQUENCY * 1000);
        current_process->burst_time -= CLOCK_FREQUENCY;
        worker->consumed_time += CLOCK_FREQUENCY;
        if (current_process->burst_time <= 0)
        {
            printf("[CORE %d]: Process PID %d finished. Total consumed time for this process: %d\n", worker->worker_id, current_process->pid, worker->consumed_time);
            current_process->state = FINISHED;
            worker->available = 1;
            worker->consumed_time = 0;
        }
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
        printf("[TIMER]: Tick\n");
        pthread_mutex_unlock(&clock_mutex);
    }

    return NULL;
}

int main()
{
    pthread_t clock_tid;
    pthread_t receiver_tids[WORKER_THREADS];
    pthread_t timer_tid;
    pthread_t process_generator_tid;
    pthread_t scheduler_tid;
    int interval_ms = 500;  // Clock interval in milliseconds
    int ticks_interval = 3; // Timer interval in clock ticks
    int thread_ids[WORKER_THREADS];

    // Create the clock thread
    if (pthread_create(&clock_tid, NULL, clock_thread, &interval_ms) != 0)
    {
        perror("Failed to create clock thread");
        exit(EXIT_FAILURE);
    }
    Worker WORKERS[WORKER_THREADS];
    // Create the receiver threads
    for (int i = 0; i < WORKER_THREADS; i++)
    {
        WORKERS[i].available = 1;
        WORKERS[i].worker_id = i + 1;
        WORKERS[i].process.pid = -1;
        WORKERS[i].consumed_time = 0;
        if (pthread_create(&receiver_tids[i], NULL, worker_thread, &WORKERS[i]) != 0)
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
    pthread_create(&scheduler_tid, NULL, scheduler_thread, &WORKERS);

    pthread_join(scheduler_tid, NULL);
    pthread_join(process_generator_tid, NULL);
    pthread_join(clock_tid, NULL);
    for (int i = 0; i < WORKER_THREADS; i++)
    {
        pthread_join(receiver_tids[i], NULL);
    }
    pthread_join(timer_tid, NULL);

    return 0;
}
