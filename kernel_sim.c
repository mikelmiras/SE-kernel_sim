#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#define MAX_PROCESSES 100
#define MAX_EXECUTING_THREADS 4 // Máximo de hilos ejecutando procesos

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
    Scheduler *scheduler;
} WorkerArgs;

// Add these as global variables
WorkerSlot *worker_slots;

// Add this global variable at the top with others
pthread_cond_t timer_tick = PTHREAD_COND_INITIALIZER;
pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
int timer_ticked = 0;

// Función para encolar un proceso
void enqueue_process(ProcessQueue *pq, PCB process)
{
    pthread_mutex_lock(&pq->mutex);
    if (pq->size < MAX_PROCESSES)
    {
        pq->queue[pq->rear] = process;
        pq->rear = (pq->rear + 1) % MAX_PROCESSES;
        pq->size++;
        printf("[Queue] Proceso PID %d encolado (burst_time: %d ms). Tamaño actual: %d\n",
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
        usleep(config->clock_frequency * 1000); // Simular el intervalo en ms
        printf("[Clock] Tick\n");
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
    Scheduler *s = args->scheduler;

    printf("[Worker %d] Iniciando worker thread\n", my_id);

    while (1)
    {
        // Wait for a process to be assigned
        pthread_mutex_lock(&my_slot->mutex);
        while (!my_slot->assigned)
        {
            pthread_cond_wait(&my_slot->cond, &my_slot->mutex);
        }

        // Get the process
        PCB process = my_slot->process;
        my_slot->assigned = 0;
        pthread_mutex_unlock(&my_slot->mutex);

        // Execute the process
        printf("[Worker %d] Ejecutando proceso PID %d (burst_time: %d ms)\n",
               my_id, process.pid, process.burst_time);

        usleep(process.burst_time * 1000);
        if (strcmp(s->policy, "RR") == 0)
        {
            int prev_burst = process.burst_time;
            process.burst_time -= QUANTUM;
            printf("Updated burst value for %d: %d (prev was %d)\n", process.pid, process.burst_time, prev_burst);
        }else{
            process.burst_time = 0;
        }
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
        }

        // Signal that we're done
        pthread_cond_broadcast(&process_queue.cond);
    }
    return NULL;
}

// Modify the scheduler thread
void *scheduler_thread(void *arg)
{
    Scheduler *scheduler = (Scheduler *)arg;
    int next_worker = 0;

    printf("[Scheduler] Iniciando scheduler\n");

    while (1)
    {
        pthread_mutex_lock(&process_queue.mutex);

        // Wait for processes
        while (process_queue.size == 0)
        {
            pthread_cond_wait(&process_queue.cond, &process_queue.mutex);
        }

        // Get next process
        PCB process = dequeue_process(&process_queue);
        pthread_mutex_unlock(&process_queue.mutex);

        // Select worker based on policy
        if (strcmp(scheduler->policy, "FCFS") == 0)
        {
            // Find first available worker
            for (int i = 0; i < total_threads; i++)
            {
                pthread_mutex_lock(&worker_slots[i].mutex);
                if (!worker_slots[i].assigned)
                {
                    worker_slots[i].process = process;
                    worker_slots[i].assigned = 1;
                    printf("[Scheduler] Asignando proceso PID %d al worker %d\n",
                           process.pid, i);
                    pthread_cond_signal(&worker_slots[i].cond);
                    pthread_mutex_unlock(&worker_slots[i].mutex);
                    break;
                }
                pthread_mutex_unlock(&worker_slots[i].mutex);
            }
        }
        else if (strcmp(scheduler->policy, "RR") == 0)
        {
            // Round Robin between workers
            pthread_mutex_lock(&worker_slots[next_worker].mutex);
            worker_slots[next_worker].process = process;
            worker_slots[next_worker].assigned = 1;
            printf("[Scheduler] Asignando proceso PID %d al worker %d\n",
                   process.pid, next_worker);
            pthread_cond_signal(&worker_slots[next_worker].cond);
            pthread_mutex_unlock(&worker_slots[next_worker].mutex);
            next_worker = (next_worker + 1) % total_threads;
        }
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

    // Configurar el Scheduler
    Scheduler scheduler = {
        .quantum = (argc == 3) ? atoi(argv[2]) : 100 // Quantum predeterminado: 100 ms
    };
    QUANTUM = scheduler.quantum;
    strcpy(scheduler.policy, argv[1]);

    // Configuración inicial del sistema
    SystemConfig config = {
        .clock_frequency = 100,
        .timer_interval = 500};

    // Crear hilos
    pthread_t clock_tid, timer_tid, process_gen_tid;
    pthread_create(&clock_tid, NULL, clock_thread, &config);
    pthread_create(&timer_tid, NULL, timer_thread, &config);
    pthread_create(&process_gen_tid, NULL, process_generator_thread, &config);

    int num_workers = (argc == 4) ? atoi(argv[3]) : MAX_EXECUTING_THREADS;
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
