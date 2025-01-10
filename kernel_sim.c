#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "queue.h"
#include <math.h>
#include "config.h"
#include <string.h>

int WORKER_THREADS = 4;    // Number of receiver threads
#define MAX_PROCESS 10     // Maximum number of processes
int CLOCK_FREQUENCY = 100; // Clock frequency in milliseconds

ProcessQueue process_queue = {
    .front = 0,
    .rear = 0, // Initialize rear to 0
    .size = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER,
};

PriorityProcessQueue priority_process_queue = {
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
    System *system = (System *)arg;
    printf("[CLOCK]: Starting clock with interval %d ms\n", system->interval_ms);
    while (1)
    {
        usleep(system->interval_ms * 1000); // Sleep for interval_ms milliseconds

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
    System *system = (System *)arg;
    int interval = system->interval_ms;
    int PROCESS_GENERATION_MULTIPLIER = atoi(get_config_value("config", "PROCESS_GENERATION_MULTIPLIER"));
    while (1)
    {
        pthread_mutex_lock(&timer_mutex);
        pthread_cond_wait(&timer_signal, &timer_mutex);
        printf("[Process Generator]: Generating new process.\n");
        pthread_mutex_unlock(&timer_mutex);
        int random_chance = (rand() % WORKER_THREADS) + PROCESS_GENERATION_MULTIPLIER;
        for (int i = 0; i < random_chance; i++)
        {

            PCB new_process = {
                .pid = pid_counter++,
                .burst_time = rand() % (interval) + 100,
                .state = READY};
            if (strcmp(system->policy, "FCFS") == 0)
            {
                enqueue_process(&process_queue, new_process);
            }
            else if (strcmp(system->policy, "SJF") == 0)
            {
                enqueue_priority_process(&priority_process_queue, new_process);
            }
            
        }
    }
}

void *scheduler_thread(void *arg)
{
    int pid_counter = 1;
    SchedulerArgs *scheduler_args = (SchedulerArgs *)arg;

    Worker *workers = scheduler_args->workers;
    System *system = scheduler_args->system;

    while (1)
    {
        pthread_mutex_lock(&timer_mutex);
        pthread_cond_wait(&timer_signal, &timer_mutex);
        printf("[Scheduler]: Trying to assign ready processes to available workers\n");
        pthread_mutex_unlock(&timer_mutex);

        for (int i = 0; i < WORKER_THREADS; i++)
        {
            Worker *current = &workers[i];
            if (!current->available){
                printf("[Scheduler]: CORE %d is busy (working on PID %d)\n", current->worker_id, current->process.pid);
                continue;
            }
             PCB *next_process = (PCB *)malloc(sizeof(PCB));
             if (!next_process) break;
            if (strcmp(system->policy, "FCFS") == 0)
            {
                *next_process = dequeue_process(&process_queue);
            }
            else if (strcmp(system->policy, "SJF") == 0)
            {
                *next_process = dequeue_priority_process(&priority_process_queue);
            }

            if (next_process->pid != -1)
            {
                printf("[Scheduler]: Assigning process PID %d to available CORE %d\n", next_process->pid, current->worker_id);
                current->process = *next_process;
                current->available = 0;
            }
            else
            {
                printf("[Scheduler]: No available processes found for CORE %d\n", current->worker_id);
            }
        }
    }
}

// Receiver thread function
void *worker_thread(void *arg)
{
    // WorkerArgs *worker_args = (WorkerArgs *)arg;
    // Worker *worker = worker_args->worker;
    // System *system = worker_args->system;

    Worker *worker = (Worker *)arg;

    while (1)
    {
        pthread_mutex_lock(&clock_mutex);
        pthread_cond_wait(&clock_signal, &clock_mutex);
        pthread_mutex_unlock(&clock_mutex);
        PCB *current_process = &(worker->process);
        if (current_process->pid == -1 || current_process->state == FINISHED)  {
            worker->available = 1;
            continue;
        }

        current_process->state = RUNNING;
        int max_burst_time = fmax(current_process->burst_time, 0);
        printf("[CORE %d]: Running process PID %d. Remaining burst time: %d\n", worker->worker_id, current_process->pid, max_burst_time);
        int consumed_time = (int) fmin(CLOCK_FREQUENCY, current_process->burst_time);
        usleep(consumed_time * 1000);
        current_process->burst_time -= CLOCK_FREQUENCY;
        worker->consumed_time += consumed_time;
        if (current_process->burst_time <= 0 && current_process->state != FINISHED)
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
    System *system = (System *)arg;
    int last_tick = 0;
    int ticks_interval = system->timer_interval;
    while (1)
    {
        pthread_mutex_lock(&clock_mutex);
        while (clock_tick < last_tick + ticks_interval)
        {
            pthread_cond_wait(&clock_signal, &clock_mutex); // Wait for clock signal
        }
        pthread_mutex_unlock(&clock_mutex);
        pthread_mutex_lock(&timer_mutex);
        pthread_cond_broadcast(&timer_signal); // Signal all waiting threads
        pthread_mutex_unlock(&timer_mutex);

        last_tick = clock_tick; // Update the last tick
        printf("[TIMER]: Tick\n");
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
    char *CORE_COUNT;
    char *clock_frequency;
    char *process_multiplier;
    CORE_COUNT = get_config_value("config", "CORE_COUNT");
    clock_frequency = get_config_value("config", "CLOCK_FREQUENCY");
    process_multiplier = get_config_value("config", "PROCESS_GENERATION_MULTIPLIER");
    WORKER_THREADS = atoi(CORE_COUNT);
    CLOCK_FREQUENCY = atoi(clock_frequency);

    char *timer_interval;
    timer_interval = get_config_value("config", "TIMER_INTERVAL");
    int interval_ms = atoi(clock_frequency);                           // Clock interval in milliseconds
    int ticks_interval = atoi(timer_interval) / interval_ms; // Timer interval in clock ticks
    int thread_ids[WORKER_THREADS];
    char *selected_policy = get_config_value("config", "SCHEDULER_POLICY");

    System *system = (System *)malloc(sizeof(System));
    system->cpu_number = WORKER_THREADS;
    system->interval_ms = interval_ms;
    system->timer_interval = ticks_interval;
    system->policy = selected_policy;
    system->process_multiplier = atoi(process_multiplier);

    char **allowed_policies = (char *[]){"FCFS", "SJF"};
    for (int i = 0; i < 3; i++)
    {
        if (strcmp(selected_policy, allowed_policies[i]) == 0)
        {
            break;
        }
        if (i == 2)
        {
            printf("[System] Invalid policy selected. Exiting...\n");
            exit(EXIT_FAILURE);
        }
    }

    printf("[System] Initializing system with %d CORES, CLOCK FREQUENCY %d AND %s POLICY\n", system->cpu_number, system->interval_ms, system->policy);

    // Create the clock thread
    if (pthread_create(&clock_tid, NULL, clock_thread, system) != 0)
    {
        perror("Failed to create clock thread");
        exit(EXIT_FAILURE);
    }
    Worker WORKERS[WORKER_THREADS];
    // Create the receiver threads
    // Create the receiver threads
for (int i = 0; i < WORKER_THREADS; i++)
{
    PCB *process = (PCB *)malloc(sizeof(PCB));
    process->pid = -1;
    process->burst_time = 0;
    process->state = FINISHED;

    // Direct initialization of Worker without dereferencing `process`
    WORKERS[i] = (Worker){
        .available = 1,
        .process = *process,  // Assign process directly
        .worker_id = i + 1,
        .consumed_time = 0,
    };

    WorkerArgs worker_arg = {
        .system = system,
        .worker = &WORKERS[i],
    };

    if (pthread_create(&receiver_tids[i], NULL, worker_thread, &WORKERS[i]) != 0)
    {
        perror("Failed to create worker thread");
        exit(EXIT_FAILURE);
    }
    printf("[System] CORE %d initialized\n", WORKERS[i].worker_id);
}

    // Create the timer thread
    if (pthread_create(&timer_tid, NULL, timer_thread, system) != 0)
    {
        perror("Failed to create timer thread");
        exit(EXIT_FAILURE);
    }

    SchedulerArgs scheduler_args = {
        .system = system,
        .workers = WORKERS,
    };

    pthread_create(&process_generator_tid, NULL, process_generator_thread, system);
    pthread_create(&scheduler_tid, NULL, scheduler_thread, &scheduler_args);

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
