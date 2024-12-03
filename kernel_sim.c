#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define MAX_PROCESSES 100

// Estructura para representar un estado de proceso
typedef enum {
    READY,
    RUNNING,
    FINISHED
} ProcessState;

// Estructura para representar un proceso (PCB)
typedef struct {
    int pid;                 // Identificador único
    int burst_time;          // Tiempo de ejecución (en ms)
    ProcessState state;      // Estado del proceso
} PCB;

// Cola de procesos
typedef struct {
    PCB queue[MAX_PROCESSES];   // Arreglo de procesos
    int front, rear, size;      // Índices y tamaño actual
    pthread_mutex_t mutex;      // Mutex para sincronización
    pthread_cond_t cond;        // Condición para notificar disponibilidad
} ProcessQueue;

// Configuración del sistema
typedef struct {
    int clock_frequency;    // Frecuencia del reloj (ms)
    int timer_interval;     // Intervalo del temporizador (ms)
    int process_frequency;  // Frecuencia de generación de procesos (ms)
} SystemConfig;

// Scheduler/Dispatcher
typedef struct {
    char policy[10];         // Política de planificación: "FCFS" o "RR"
    int quantum;             // Quantum para Round Robin
    pthread_mutex_t mutex;   // Mutex para proteger el Scheduler
} Scheduler;

// Cola global de procesos
ProcessQueue process_queue = {
    .front = 0,
    .rear = 0,
    .size = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER
};

// Función para encolar un proceso
void enqueue_process(ProcessQueue *pq, PCB process) {
    pthread_mutex_lock(&pq->mutex);
    if (pq->size < MAX_PROCESSES) {
        pq->queue[pq->rear] = process;
        pq->rear = (pq->rear + 1) % MAX_PROCESSES;
        pq->size++;
        printf("[Queue] Proceso PID %d encolado (burst_time: %d ms). Tamaño actual: %d\n",
               process.pid, process.burst_time, pq->size);
    } else {
        printf("[Queue] Cola llena, no se puede encolar el proceso PID %d\n", process.pid);
    }
    pthread_mutex_unlock(&pq->mutex);
    pthread_cond_signal(&pq->cond); // Notificar que hay un nuevo proceso
}

// Función para desencolar un proceso
PCB dequeue_process(ProcessQueue *pq) {
    pthread_mutex_lock(&pq->mutex);
    PCB process = { .pid = -1, .burst_time = 0, .state = FINISHED };

    if (pq->size > 0) {
        process = pq->queue[pq->front];
        pq->front = (pq->front + 1) % MAX_PROCESSES;
        pq->size--;
    }

    pthread_mutex_unlock(&pq->mutex);
    return process;
}

// Función del hilo del reloj
void *clock_thread(void *arg) {
    SystemConfig *config = (SystemConfig *)arg;
    while (1) {
        usleep(config->clock_frequency * 1000); // Simular el intervalo en ms
        printf("[Clock] Tick\n");
    }
    return NULL;
}

// Función del hilo del temporizador
void *timer_thread(void *arg) {
    SystemConfig *config = (SystemConfig *)arg;
    while (1) {
        usleep(config->timer_interval * 1000); // Simular el intervalo en ms
        printf("[Timer] Timer Tick\n");
    }
    return NULL;
}

// Función del hilo generador de procesos
void *process_generator_thread(void *arg) {
    SystemConfig *config = (SystemConfig *)arg;
    int pid_counter = 1; // Contador de procesos

    while (1) {
        usleep(config->process_frequency * 1000); // Frecuencia de generación

        PCB new_process = {
            .pid = pid_counter++,
            .burst_time = rand() % 500 + 100, // Burst time aleatorio (100-600 ms)
            .state = READY
        };

        enqueue_process(&process_queue, new_process);
    }
    return NULL;
}

// Función para ejecutar un proceso (Dispatcher)
void dispatch_process(PCB *process) {
    printf("[Dispatcher] Ejecutando proceso PID %d (burst_time: %d ms)\n", process->pid, process->burst_time);
    process->state = RUNNING;

    // Simular la ejecución del proceso
    usleep(process->burst_time * 1000);

    process->state = FINISHED;
    printf("[Dispatcher] Proceso PID %d finalizado\n", process->pid);
}

// Función del hilo del Scheduler
void *scheduler_thread(void *arg) {
    Scheduler *scheduler = (Scheduler *)arg;

    while (1) {
        pthread_mutex_lock(&process_queue.mutex);

        // Esperar si la cola está vacía
        while (process_queue.size == 0) {
            pthread_cond_wait(&process_queue.cond, &process_queue.mutex);
        }

        // Extraer el proceso de la cola
        PCB process = dequeue_process(&process_queue);

        pthread_mutex_unlock(&process_queue.mutex);

        // Ejecutar según la política
        if (strcmp(scheduler->policy, "FCFS") == 0) {
            dispatch_process(&process);
        } else if (strcmp(scheduler->policy, "RR") == 0) {
            // Ejecutar el proceso por un quantum o hasta finalizar
            int time_to_run = (process.burst_time > scheduler->quantum)
                              ? scheduler->quantum
                              : process.burst_time;

            printf("[Scheduler] Ejecutando proceso PID %d por %d ms\n", process.pid, time_to_run);
            usleep(time_to_run * 1000); // Simular ejecución

            process.burst_time -= time_to_run;

            if (process.burst_time > 0) {
                printf("[Scheduler] Proceso PID %d vuelve a READY (restante: %d ms)\n",
                       process.pid, process.burst_time);
                enqueue_process(&process_queue, process);
            } else {
                printf("[Scheduler] Proceso PID %d finalizado\n", process.pid);
            }
        }
    }

    return NULL;
}

// Función principal
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <política> [quantum]\n", argv[0]);
        printf("Políticas disponibles: FCFS, RR\n");
        exit(1);
    }

    // Configurar el Scheduler
    Scheduler scheduler = {
        .quantum = (argc == 3) ? atoi(argv[2]) : 100 // Quantum predeterminado: 100 ms
    };
    strcpy(scheduler.policy, argv[1]);

    // Configuración inicial del sistema
    SystemConfig config = {
        .clock_frequency = 100,
        .timer_interval = 500,
        .process_frequency = 1000
    };

    // Crear hilos
    pthread_t clock_tid, timer_tid, process_gen_tid, scheduler_tid;
    pthread_create(&clock_tid, NULL, clock_thread, &config);
    pthread_create(&timer_tid, NULL, timer_thread, &config);
    pthread_create(&process_gen_tid, NULL, process_generator_thread, &config);
    pthread_create(&scheduler_tid, NULL, scheduler_thread, &scheduler);

    // Esperar a los hilos
    pthread_join(clock_tid, NULL);
    pthread_join(timer_tid, NULL);
    pthread_join(process_gen_tid, NULL);
    pthread_join(scheduler_tid, NULL);

    return 0;
}
