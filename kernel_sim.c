#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include "config.h"

#define MAX_PROCESSES 100
#define MAX_EXECUTING_THREADS 4 // Máximo de hilos ejecutando procesos
#define CONFIG_FILE "config"

// Estructura para representar un estado de proceso
typedef enum
{
    READY,
    RUNNING,
    FINISHED
} ProcessState;

typedef struct
{
    int pgb;
    int code;
    int data;
} mm;

// Estructura para representar un proceso (PCB)
typedef struct
{
    int pid;            // Identificador único
    int burst_time;     // Tiempo de ejecución (en ms)
    ProcessState state; // Estado del proceso
} PCB;

// Cola de procesos
typedef struct
{
    PCB queue[MAX_PROCESSES]; // Arreglo de procesos
    int front, rear, size;    // Índices y tamaño actual
    pthread_mutex_t mutex;    // Mutex para sincronización
    pthread_cond_t cond;      // Condición para notificar disponibilidad
} ProcessQueue;

// Configuración del sistema
typedef struct
{
    int clock_frequency;   // Frecuencia del reloj (ms)
    int timer_interval;    // Intervalo del temporizador (ms)
    int consumed_time;     // Tiempo consumido por los procesos (ms)

} SystemConfig;

typedef struct
{
    char policy[10];       // Política de planificación: "FCFS" o "RR"
    int quantum;           // Quantum para Round Robin
    pthread_mutex_t mutex; // Mutex para proteger el Scheduler
} Scheduler;

// Cola global de procesos
ProcessQueue process_queue = {
    .front = 0,
    .rear = 0,
    .size = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER};

// Add this at the top with other global variables
pthread_mutex_t thread_counter_mutex = PTHREAD_MUTEX_INITIALIZER;
int current_thread = 0;
int total_threads = 0;
int *thread_busy; // Array to track if each thread is busy
pthread_mutex_t executor_mutex = PTHREAD_MUTEX_INITIALIZER;
int current_executor = -1; // Which thread is currently allowed to execute

int QUANTUM = 100;
int CLOCK_FREQUENCY = 100;

// Add these structures at the top
typedef struct
{
    PCB process;
    int assigned; // Flag to mark if this slot is assigned
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} WorkerSlot;

typedef struct
{
    int worker_id;
    WorkerSlot *slot;
    SystemConfig *system;
    Scheduler *scheduler;
} WorkerArgs;

// Add these as global variables
WorkerSlot *worker_slots;

// Add this global variable at the top with others
pthread_cond_t timer_tick = PTHREAD_COND_INITIALIZER;
pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t clock_tick = PTHREAD_COND_INITIALIZER;
pthread_mutex_t clock_mutex = PTHREAD_MUTEX_INITIALIZER;

int timer_ticked, clock_ticked = 0;

// Función para encolar un proceso
void enqueue_process(ProcessQueue *pq, PCB process)
{
    pthread_mutex_lock(&pq->mutex);
    if (pq->size < MAX_PROCESSES)
    {
        pq->queue[pq->rear] = process;
        pq->rear = (pq->rear + 1) % MAX_PROCESSES;
        pq->size++;
        printf("[Queue] Proceso PID %d encolado (burst_time: %d ms).%d\n",
               process.pid, process.burst_time, pq->size);
    }
    else
    {
        printf("[Queue] Cola llena, no se puede encolar el proceso PID %d\n", process.pid);
    }
    pthread_mutex_unlock(&pq->mutex);
    pthread_cond_signal(&pq->cond); // Notificar que hay un nuevo proceso
}

// Función para desencolar un proceso
PCB dequeue_process(ProcessQueue *pq)
{
    PCB process = {.pid = -1, .burst_time = 0, .state = FINISHED};

    if (pq->size > 0)
    {
        process = pq->queue[pq->front];
        pq->front = (pq->front + 1) % MAX_PROCESSES;
        pq->size--;
    }

    return process;
}

// Función del hilo del reloj
void *clock_thread(void *arg)
{
    SystemConfig *config = (SystemConfig *)arg;
    while (1)
    {
        usleep(config->clock_frequency * 1000); // Simulate the interval in ms
        pthread_mutex_lock(&clock_mutex);
        printf("[Clock] Tick\n");
        clock_ticked = 1;
        pthread_cond_broadcast(&clock_tick); // Wake all worker threads
        pthread_mutex_unlock(&clock_mutex);
        config->consumed_time += config->clock_frequency;
        usleep((CLOCK_FREQUENCY - 10) * 1000);
        clock_ticked = 0;
    }
    return NULL;
}


// Función del hilo del temporizador
void *timer_thread(void *arg)
{
    SystemConfig *config = (SystemConfig *)arg;
    while (1) {
        usleep(config->timer_interval * 1000);
        
        pthread_mutex_lock(&timer_mutex);
        timer_ticked = 1;
        printf("[Timer] Timer Tick\n");
        pthread_cond_signal(&timer_tick);  // Signal process generator
        pthread_mutex_unlock(&timer_mutex);
    }
    return NULL;
}

// Función del hilo generador de procesos
void *process_generator_thread(void *arg)
{
    SystemConfig *config = (SystemConfig *)arg;
    int pid_counter = 1;

    while (1) {
        // Wait for timer tick
        pthread_mutex_lock(&timer_mutex);
        while (!timer_ticked) {
            pthread_cond_wait(&timer_tick, &timer_mutex);
        }
        timer_ticked = 0;  // Reset the flag
        pthread_mutex_unlock(&timer_mutex);

        // Generate processes
        for (int id = 0; id < MAX_EXECUTING_THREADS; id++) {
            PCB new_process = {
                .pid = pid_counter++,
                .burst_time = rand() % 500 + 100,
                .state = READY
            };
            enqueue_process(&process_queue, new_process);
        }
    }
    return NULL;
}

// Add the worker thread function
void *worker_thread(void *arg)
{
    WorkerArgs *args = (WorkerArgs *)arg;
    int my_id = args->worker_id;
    WorkerSlot *my_slot = args->slot;
    SystemConfig *s = args->system;
    Scheduler *scheduler = args->scheduler;

    printf("[Worker %d] Iniciando worker thread\n", my_id);

    while (1)
    {
        pthread_mutex_lock(&clock_mutex);
        while (!clock_ticked) {
            pthread_cond_wait(&clock_tick, &clock_mutex);
        }
        pthread_mutex_unlock(&clock_mutex);
        // Wait for a process to be assigned
        pthread_mutex_lock(&my_slot->mutex);
        while (!my_slot->assigned)
        {
            pthread_cond_wait(&my_slot->cond, &my_slot->mutex);
        }

        // Get the process
        PCB process = my_slot->process;
        pthread_mutex_unlock(&my_slot->mutex);

        // Execute the process
        printf("[Worker %d] Ejecutando proceso PID %d (burst_time: %d ms)\n",
               my_id, process.pid, process.burst_time);

        usleep(CLOCK_FREQUENCY * 1000);
        process.burst_time -= CLOCK_FREQUENCY;
        if (process.burst_time <= 0)
        {
            process.state = FINISHED;
            printf("[Worker %d] Proceso PID %d finalizado\n", my_id, process.pid);
        }
        else
        {
            process.state = READY;
            printf("[Worker %d] Process PID %d has not finished yet. Remaining burst_time: %d\n", my_id, process.pid, process.burst_time);
            enqueue_process(&process_queue, process);
            if (strcmp(scheduler->policy, "RR") == 0 && s->consumed_time >= scheduler->quantum)
            {
                pthread_mutex_lock(&my_slot->mutex);
                my_slot->assigned = 0;
                pthread_mutex_unlock(&my_slot->mutex);
                printf("[Worker %d] Quantum expirado para proceso PID %d\n", my_id, process.pid);
            }else{
                pthread_mutex_lock(&my_slot->mutex);
                my_slot->assigned = 0;
                pthread_mutex_unlock(&my_slot->mutex);
            }
        }
        pthread_cond_signal(&process_queue.cond);
    }
    return NULL;
}

void *scheduler_thread(void *arg)
{
    Scheduler *scheduler = (Scheduler *)arg;
    int next_worker = 0; // Used for Round Robin policy

    printf("[Scheduler] Iniciando scheduler\n");

    while (1)
    {
        // Lock the queue and wait for a process to be available
        pthread_mutex_lock(&process_queue.mutex);
        while (process_queue.size == 0)
        {
            pthread_cond_wait(&process_queue.cond, &process_queue.mutex);
        }

        // Find an available worker based on the policy
        int selected_worker = -1;

        if (strcmp(scheduler->policy, "FCFS") == 0)
        {
            // First Come First Serve: Find the first available worker
            for (int i = 0; i < total_threads; i++)
            {
                pthread_mutex_lock(&worker_slots[i].mutex);
                if (!worker_slots[i].assigned)
                {
                    selected_worker = i;
                    pthread_mutex_unlock(&worker_slots[i].mutex);
                    break;
                }
                pthread_mutex_unlock(&worker_slots[i].mutex);
            }
        }
        else if (strcmp(scheduler->policy, "RR") == 0)
        {
            // Round Robin: Find the next available worker in order
            for (int i = 0; i < total_threads; i++)
            {
                int worker_index = (next_worker + i) % total_threads;
                pthread_mutex_lock(&worker_slots[worker_index].mutex);
                if (!worker_slots[worker_index].assigned)
                {
                    selected_worker = worker_index;
                    pthread_mutex_unlock(&worker_slots[worker_index].mutex);
                    next_worker = (worker_index + 1) % total_threads; // Update for next round
                    break;
                }
                pthread_mutex_unlock(&worker_slots[worker_index].mutex);
            }
        }

        // If no worker is available, retry later
        if (selected_worker == -1)
        {
            pthread_mutex_unlock(&process_queue.mutex);
            sched_yield(); // Yield CPU and retry later
            continue;
        }

        // Dequeue the process
        PCB process = dequeue_process(&process_queue);
        pthread_mutex_unlock(&process_queue.mutex);

        // Assign the process to the selected worker
        pthread_mutex_lock(&worker_slots[selected_worker].mutex);
        worker_slots[selected_worker].process = process;
        worker_slots[selected_worker].assigned = 1;
        printf("[Scheduler] Asignando proceso PID %d al worker %d\n",
               process.pid, selected_worker);
        pthread_cond_signal(&worker_slots[selected_worker].cond);
        pthread_mutex_unlock(&worker_slots[selected_worker].mutex);
    }

    return NULL;
}



// Función principal
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Uso: %s <política> [quantum] [número de hilos ejecutando]\n", argv[0]);
        printf("Políticas disponibles: FCFS, RR\n");
        exit(1);
    }

    
    char *thread_count = get_config_value(CONFIG_FILE, "THREAD_COUNT");
    char *quantum = get_config_value(CONFIG_FILE, "QUANTUM");
    char *CLOCK_FREQUENCY_CONFIG = get_config_value(CONFIG_FILE, "CLOCK_FREQUENCY");
    char *TIMER_INTERVAL = get_config_value(CONFIG_FILE, "TIMER_INTERVAL");

    int threads = 4;
    int timer_interval = 500;
    if (thread_count) threads = atoi(thread_count);
    if (quantum) QUANTUM = atoi(quantum);
    if (CLOCK_FREQUENCY_CONFIG) CLOCK_FREQUENCY = atoi(CLOCK_FREQUENCY_CONFIG);
    if (TIMER_INTERVAL) timer_interval = atoi(TIMER_INTERVAL);
    printf("Thread count is: %d\n", threads);
    Scheduler scheduler = {
        .quantum = QUANTUM
    };
    QUANTUM = scheduler.quantum;
    strcpy(scheduler.policy, argv[1]);

    // Configuración inicial del sistema
    SystemConfig config = {
        .clock_frequency = CLOCK_FREQUENCY,
        .timer_interval = timer_interval
        };

    // Crear hilos
    pthread_t clock_tid, timer_tid, process_gen_tid;
    pthread_create(&clock_tid, NULL, clock_thread, &config);
    pthread_create(&timer_tid, NULL, timer_thread, &config);
    pthread_create(&process_gen_tid, NULL, process_generator_thread, &config);

    int num_workers = threads;
    total_threads = num_workers;

    // Initialize worker slots
    worker_slots = calloc(num_workers, sizeof(WorkerSlot));
    for (int i = 0; i < num_workers; i++)
    {
        pthread_mutex_init(&worker_slots[i].mutex, NULL);
        pthread_cond_init(&worker_slots[i].cond, NULL);
        worker_slots[i].assigned = 0;
    }

    // Create worker threads
    pthread_t *worker_tids = calloc(num_workers, sizeof(pthread_t));
    WorkerArgs *worker_args = calloc(num_workers, sizeof(WorkerArgs));

    for (int i = 0; i < num_workers; i++)
    {
        worker_args[i].worker_id = i;
        worker_args[i].slot = &worker_slots[i];
        worker_args[i].system = &config;
        worker_args[i].scheduler = &scheduler;
        pthread_create(&worker_tids[i], NULL, worker_thread, &worker_args[i]);
    }

    // Create scheduler thread
    pthread_t scheduler_tid;
    pthread_create(&scheduler_tid, NULL, scheduler_thread, &scheduler);

    // Esperar a los hilos
    pthread_join(clock_tid, NULL);
    pthread_join(timer_tid, NULL);
    pthread_join(process_gen_tid, NULL);
    pthread_join(scheduler_tid, NULL);
    for (int i = 0; i < num_workers; i++)
    {
        pthread_join(worker_tids[i], NULL);
        pthread_mutex_destroy(&worker_slots[i].mutex);
        pthread_cond_destroy(&worker_slots[i].cond);
    }

    free(worker_slots);
    free(worker_tids);
    free(worker_args);

    return 0;
}
