#include <pthread.h>
#define MAX_PROCESSES 100

typedef enum
{
    READY,
    RUNNING,
    FINISHED
} ProcessState;

typedef struct
{
    int pid;            // Identificador único
    int burst_time;     // Tiempo de ejecución (en ms)
    ProcessState state; // Estado del proceso
} PCB;

typedef struct
{
    PCB queue[MAX_PROCESSES]; // Arreglo de procesos
    int front, rear, size;    // Índices y tamaño actual
    pthread_mutex_t mutex;    // Mutex para sincronización
    pthread_cond_t cond;      // Condición para notificar disponibilidad
} ProcessQueue;


typedef struct
{
    PCB queue[MAX_PROCESSES]; // Arreglo de procesos
    int size;                 // Tamaño actual
    pthread_mutex_t mutex;    // Mutex para sincronización
    pthread_cond_t cond;      // Condición para notificar disponibilidad
} PriorityProcessQueue;


typedef struct {
    char *policy;
    int interval_ms;
    int cpu_number;
    int timer_interval;
    int process_multiplier;
} System;

typedef struct {
    int available;
    PCB process;
    int worker_id;
    int consumed_time;
} Worker;

typedef struct {
        System *system;
        Worker *workers;
    } SchedulerArgs;


typedef struct {
        System *system;
        Worker *worker;
} WorkerArgs;