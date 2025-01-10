#include "definitions.h"
#include <stdio.h>
void enqueue_process(ProcessQueue *pq, PCB process)
{
    pthread_mutex_lock(&pq->mutex);
    if (pq->size < MAX_PROCESSES)
    {
        pq->queue[pq->rear] = process;
        pq->rear = (pq->rear + 1) % MAX_PROCESSES;
        pq->size++;
        printf("[QUEUE] Process PID %d queued (burst_time: %d ms). Processes ready: %d\n",
               process.pid, process.burst_time, pq->size);
    }
    else
    {
        printf("[QUEUE] Queue is full. Couldn't queue process PID %d\n", process.pid);
    }
    pthread_mutex_unlock(&pq->mutex);
    pthread_cond_signal(&pq->cond); // Notificar que hay un nuevo proceso
}

// Función para desencolar un proceso
PCB dequeue_process(ProcessQueue *pq)
{
    pthread_mutex_lock(&pq->mutex);
    PCB process = {.pid = -1, .burst_time = 0, .state = FINISHED};

    if (pq->size > 0)
    {
        process = pq->queue[pq->front];
        pq->front = (pq->front + 1) % MAX_PROCESSES;
        pq->size--;
    }
    pthread_mutex_unlock(&pq->mutex);
    return process;
}


// Función para encolar un proceso en la cola de prioridad (ordenado por burst_time)
void enqueue_priority_process(PriorityProcessQueue *pq, PCB process)
{
    pthread_mutex_lock(&pq->mutex);
    
    if (pq->size < MAX_PROCESSES)
    {
        // Encuentra la posición en la que debe insertarse el proceso
        int i = pq->size - 1;
        while (i >= 0 && pq->queue[i].burst_time > process.burst_time)
        {
            pq->queue[i + 1] = pq->queue[i];
            i--;
        }

        // Inserta el proceso en la posición correcta
        pq->queue[i + 1] = process;
        pq->size++;

        printf("[Queue] Proceso PID %d encolado (burst_time: %d ms). Procesos en cola: %d\n",
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
PCB dequeue_priority_process(PriorityProcessQueue *pq)
{
    pthread_mutex_lock(&pq->mutex);
    PCB process = {.pid = -1, .burst_time = 0, .state = FINISHED};

    if (pq->size > 0)
    {
        // Desencola el primer proceso (el de menor burst_time)
        process = pq->queue[0];
        
        // Mueve todos los procesos hacia la izquierda
        for (int i = 0; i < pq->size - 1; i++)
        {
            pq->queue[i] = pq->queue[i + 1];
        }

        pq->size--;
    }
    pthread_mutex_unlock(&pq->mutex);
    return process;
}
