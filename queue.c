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

