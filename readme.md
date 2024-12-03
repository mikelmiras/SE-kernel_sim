# Introducción

Este proyecto consiste en un simulador de kernel de sistema operativo implementado en lenguaje C. El objetivo es simular el comportamiento de un sistema con múltiples hilos, que incluye componentes clave como el reloj, temporizador, generador de procesos y un Scheduler/Dispatcher para planificar la ejecución de procesos.

El sistema debe gestionar procesos en una cola compartida, implementar políticas de planificación como First-Come, First-Served (FCFS) y Round Robin (RR), y garantizar la sincronización y comunicación entre los diferentes hilos mediante el uso de mutex y variables de condición.

## Requisitos del Proyecto

### Componentes Principales
1. Reloj (Clock):
    - Genera ciclos de tiempo (ticks) con una frecuencia configurable.
2. Temporizador (Timer):
    - Genera señales periódicas (timer ticks) con una frecuencia fija.
3. Generador de Procesos (Process Generator):
    - Crea procesos con tiempos de ejecución aleatorios.
    - Encola los procesos en la cola compartida.
4. Scheduler/Dispatcher:
    - Planifica los procesos para su ejecución en función de políticas seleccionadas:
        - **FCFS**: Ejecuta procesos en orden de llegada.
        - **Round Robin**: Ejecuta procesos en intervalos de tiempo fijo (quantum).
    - Coordina los cambios de contexto entre procesos.
## Estructuras de Datos
1. Process Control Block (PCB):
    - Representa un proceso.
    - Campos principales:
```c
typedef struct {
    int pid;                 // Identificador único
    int burst_time;          // Tiempo de ejecución restante
    ProcessState state;      // READY, RUNNING o FINISHED
} PCB;
```
2. Cola de Procesos:
    - Cola circular para almacenar procesos.
    - Métodos para encolar (`enqueue_process`) y desencolar (`dequeue_process`).
3. Configuración del Sistema:
    - Frecuencias configurables para el reloj, temporizador y generación de procesos.
    - Política de planificación y quantum para el Scheduler.
## Diseño del Sistema

### 1. Hilos del Sistema
#### **Reloj (Clock)**

- Marca los ciclos (ticks) con una frecuencia fija.
- Simula el tiempo del sistema.

#### **Temporizador (Timer)**

- Genera eventos periódicos que activan al Scheduler y otros subsistemas.

#### **Generador de Procesos**

- Crea procesos con tiempos de ejecución aleatorios.
- Encola los procesos en la cola compartida, protegida por un mutex.
#### **Scheduler/Dispatcher**

- Desencola procesos de la cola compartida.
- Ejecuta los procesos según la política configurada:
    - **FCFS**: Ejecuta procesos completos en el orden en que llegan.
    - **RR**: Ejecuta procesos en intervalos fijos (`quantum`) y reencola procesos si aún tienen tiempo de ejecución restante.

### 2. Políticas de Planificación
#### First-Come, First-Served (FCFS)

- #### Descripción:
    - Ejecuta procesos en el orden en que llegan.
    - No permite interrupciones hasta que el proceso finalice.
- #### Implementación:
    - Se desencola un proceso y se ejecuta completamente.
    - Cambia el estado del proceso de READY a RUNNING y luego a FINISHED.


#### Round Robin (RR)

- #### Descripción:
    - Divide el tiempo de CPU en intervalos fijos (quantum).
    - Si un proceso no termina en su quantum, se reencola con el tiempo restante.
- #### Implementación:
    - Ejecuta el proceso durante `min(burst_time, quantum)`.
    - Si el tiempo restante es mayor que el quantum, el proceso se reencola en la cola compartida.

### 3. Sincronización
El sistema utiliza mutex y variables de condición para garantizar que los hilos trabajen de forma coordinada:

- **Mutex**: Protege el acceso concurrente a la cola de procesos.
- **Variables de condición**: Permiten notificar al Scheduler cuando hay procesos en la cola.

## Implementación

El código fuente del sistema se organiza en los siguientes componentes:

1. Funciones del Sistema
    - `enqueue_process`: Encola un proceso en la cola compartida.
    - `dequeue_process`: Desencola un proceso de la cola compartida.
    - `dispatch_process`: Simula la ejecución de un proceso.
2. Hilos del Sistema
    - `clock_thread`: Simula los ticks del reloj.
    - `timer_thread`: Genera señales periódicas del temporizador.
    - `process_generator_thread`: Genera procesos con tiempos de ejecución aleatorios.
    - `scheduler_thread`: Planifica y ejecuta procesos según la política configurada.
3. Configuración
    - Los parámetros del sistema (frecuencias y política de planificación) se configuran al inicio y se pasan como argumentos al programa.
4. Ejecución
    - Se compila el código usando gcc y se ejecuta especificando la política y el quantum (en el caso de Round Robin).
## Ejecución del Sistema

### Compilación
```bash
gcc -pthread -o kernel_simulator kernel_simulator.c
```

### Ejecución
1. Política FCFS: 
```bash
./kernel_simulator FCFS
```
2. Política Round Robin (quantum = 200 ms):
```bash
./kernel_simulator RR 200
```
## Flujo de Ejecución

1. Inicialización del Sistema:
    -   Se crean las estructuras de datos y los hilos del sistema.
    - Se configuran las frecuencias y la política de planificación.
2. Generación de Procesos:
    - El generador de procesos crea procesos periódicamente y los encola en la cola compartida.
3. Planificación y Ejecución:
    - El Scheduler desencola los procesos y los ejecuta según la política configurada.
    - En Round Robin, los procesos con tiempo restante se reencolan.
4. Finalización:
    - El proceso se marca como FINISHED cuando su tiempo de ejecución llega a 0.
## Resultados Esperados

1. FCFS:
    - Los procesos se ejecutan en el orden en que llegan.
    - Cada proceso se ejecuta completamente antes de que otro comience.
2. Round Robin:
    - Los procesos se ejecutan en intervalos de tiempo fijos.
    - Los procesos que no terminan en su quantum se reencolan hasta que finalizan.
## Conclusiones

- El simulador implementa un kernel básico con soporte para múltiples hilos y sincronización mediante mutex y variables de condición.
- Las políticas de planificación FCFS y Round Robin están correctamente implementadas y configuran comportamientos diferentes en el sistema.
- El sistema puede ampliarse fácilmente para incluir nuevas políticas o funcionalidades adicionales.

## Posibles Extensiones

1. Implementar políticas adicionales como:
    - Shortest Job Next (SJN).
    - Priority Scheduling.
2. Incluir métricas de desempeño:
    - Tiempo de espera promedio.
    - Tiempo de respuesta promedio.
3. Simular múltiples CPUs o núcleos.