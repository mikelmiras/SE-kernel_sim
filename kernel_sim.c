#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_PROCESSES 100

// Estructura para representar un PCB (Proceso)
typedef struct {
    int pid; // Identificador único del proceso
} PCB;

// Cola de procesos
typedef struct {
    PCB queue[MAX_PROCESSES];   // Arreglo para almacenar procesos
    int front, rear, size;      // Índices y tamaño actual de la cola
    pthread_mutex_t mutex;      // Mutex para sincronización
    pthread_cond_t cond;        // Condición para notificar disponibilidad
} ProcessQueue;

// Configuración del sistema
typedef struct {
    int clock_frequency;    // Frecuencia del reloj (ms)
    int timer_interval;     // Intervalo del temporizador (ms)
    int process_frequency;  // Frecuencia del generador de procesos (ms)
} SystemConfig;

// Cola global de procesos
ProcessQueue process_queue = {
    .front = 0,
    .rear = 0,
    .size = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER
};

// Función para encolar procesos
void enqueue_process(ProcessQueue *pq, PCB process) {
    pthread_mutex_lock(&pq->mutex);
    if (pq->size < MAX_PROCESSES) {
        pq->queue[pq->rear] = process;
        pq->rear = (pq->rear + 1) % MAX_PROCESSES;
        pq->size++;
        printf("[Queue] Proceso PID %d encolado. Tamaño actual: %d\n", process.pid, pq->size);
    } else {
        printf("[Queue] Cola llena, no se puede encolar el proceso PID %d\n", process.pid);
    }
    pthread_mutex_unlock(&pq->mutex);
    pthread_cond_signal(&pq->cond); // Notificar que hay un nuevo proceso
}

// Hilo del reloj
void *clock_thread(void *arg) {
    SystemConfig *config = (SystemConfig *)arg;
    while (1) {
        usleep(config->clock_frequency * 1000); // Intervalo en ms
        printf("[Clock] Clock Pulse\n");
    }
    return NULL;
}

// Hilo del temporizador
void *timer_thread(void *arg) {
    SystemConfig *config = (SystemConfig *)arg;
    while (1) {
        usleep(config->timer_interval * 1000); // Intervalo en ms
        printf("[Timer] Timer Tick\n");
    }
    return NULL;
}

// Hilo generador de procesos
void *process_generator_thread(void *arg) {
    SystemConfig *config = (SystemConfig *)arg;
    int pid_counter = 1; // Contador para asignar PIDs únicos
    while (1) {
        usleep(config->process_frequency * 1000); // Intervalo en ms
        PCB new_process = { .pid = pid_counter++ };
        enqueue_process(&process_queue, new_process);
    }
    return NULL;
}

// Función principal
int main(int argc, char *argv[]) {
    // Configuración inicial del sistema
    SystemConfig config = {
        .clock_frequency = 100,   // Frecuencia del reloj en ms
        .timer_interval = 500,    // Intervalo del temporizador en ms
        .process_frequency = 1000 // Frecuencia de generación de procesos en ms
    };

    // Hilos para cada subsistema
    pthread_t clock_tid, timer_tid, process_gen_tid;

    // Crear los hilos
    pthread_create(&clock_tid, NULL, clock_thread, &config);
    pthread_create(&timer_tid, NULL, timer_thread, &config);
    pthread_create(&process_gen_tid, NULL, process_generator_thread, &config);

    // Esperar a que los hilos terminen (en este diseño no ocurre)
    pthread_join(clock_tid, NULL);
    pthread_join(timer_tid, NULL);
    pthread_join(process_gen_tid, NULL);

    return 0;
}

