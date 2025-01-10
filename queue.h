#include "definitions.h"
void enqueue_process(ProcessQueue *pq, PCB process);
PCB dequeue_process(ProcessQueue *pq);


void enqueue_priority_process(PriorityProcessQueue *pq, PCB process);

PCB dequeue_priority_process(PriorityProcessQueue *pq);