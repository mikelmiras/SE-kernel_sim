# Introducción

El propósito del proyecto descrito en este reporte es la implementación de un sistema de planificación de procesos que simule un ambiente multitarea en el que varios procesos compiten por la CPU. Este sistema utiliza múltiples hilos de ejecución (threads) para gestionar los procesos, asignarles la CPU y asegurar que se ejecuten en el orden correcto de acuerdo con las políticas de planificación especificadas.

Se exploran las técnicas de planificación de procesos más comunes, como First Come First Served (FCFS) y Shortest Job First (SJF), las cuales se emplean para organizar los procesos según diferentes criterios de eficiencia. FCFS es un enfoque sencillo en el que los procesos se ejecutan en el orden en que llegan, mientras que SJF prioriza los procesos con tiempos de ejecución más cortos. En este contexto, el sistema implementado tiene la capacidad de simular ambas políticas, permitiendo comparar su desempeño en diferentes escenarios.

La simulación también incluye la gestión de un sistema de interrupciones a través de un reloj simulado, el cual genera eventos a intervalos regulares para activar y desencadenar las transiciones de estado de los procesos. Este sistema se implementa utilizando una arquitectura multihilo, donde los hilos representan tanto los procesos que se ejecutan en la CPU como los encargados de la gestión de eventos, la programación y el seguimiento de los estados de los procesos.


# Descripción general del sistema
El sistema desarrollado es una simulación de la planificación de procesos en un sistema operativo multitarea. Este simula un entorno donde múltiples procesos son generados, encolados y ejecutados por la CPU de acuerdo con una política de planificación. A continuación, se describe cada uno de los componentes que forman parte de este sistema.

1- **Generación de procesos**: El sistema tiene un hilo dedicado a la generación de procesos. Este hilo se encarga de crear nuevos procesos a intervalos regulares, de acuerdo con un multiplicador de generación de procesos que se ajusta en la configuración del sistema. Cada proceso tiene un identificador único (PID), un tiempo de ejecución estimado (burst time) y un estado, que puede ser READY, RUNNING, o FINISHED. Los procesos son generados aleatoriamente con un tiempo de ejecución dentro de un rango determinado.

2- **Encolado de Procesos**:

-   **Cola de procesos (ProcessQueue)**: Esta cola contiene los procesos listos para ser ejecutados, siguiendo la política de planificación First Come, First Served (FCFS), donde los procesos se ejecutan en el orden en que llegaron.
- **Cola de Procesos Prioritarios (PriorityProcessQueue)**: Esta cola es utilizada cuando se emplea la política Shortest Job First (SJF), donde los procesos con menor tiempo de ejecución tienen mayor prioridad para ser ejecutados primero.

Ambas colas están protegidas por mecanismos de sincronización como mutexes y condiciones, lo que asegura un acceso seguro a las colas en un entorno multihilo.

3- **Hilos de planificación y ejecución**: El sistema está compuesto por varios hilos de ejecución, cada uno con una función específica:
- **Hilo del Reloj (Clock Thread)**: Este hilo simula un reloj de alta precisión que genera eventos a intervalos regulares, lo que permite sincronizar la ejecución de los procesos y coordinar el avance del sistema.

- **Hilo Generador de Procesos (Process Generator Thread)**: Se encarga de generar nuevos procesos en función del intervalo de tiempo configurado.

- **Hilo del Planificador (Scheduler Thread)**: Este hilo se encarga de asignar los procesos listos a los trabajadores disponibles (hilos de la CPU), respetando la política de planificación seleccionada. Si hay procesos disponibles y trabajadores libres, el planificador les asigna los procesos, actualizando su estado y el estado de los trabajadores.

- **Hilos de Trabajadores (Worker Threads)**: Son los hilos que representan los núcleos de la CPU. Cada uno de estos hilos ejecuta un proceso a la vez, tomando el siguiente proceso disponible según la política de planificación. Los trabajadores pueden estar en tres estados: **READY** (listos para ejecutar), **RUNNING** (ejecutando), o **FINISHED** (finalizados).

4- **Manejo de Temporizadores**: El sistema utiliza un hilo de temporizador (Timer Thread) que coordina el avance del sistema mediante la señalización de eventos a intervalos regulares. Este hilo asegura que los hilos de ejecución, como el planificador o los trabajadores, puedan reaccionar en los momentos correctos.

- **First Come, First Served (FCFS)**: En esta política, los procesos se ejecutan en el orden en que llegan. No se realiza ninguna consideración de priorización, por lo que el primer proceso que entra en el sistema es el primero en ser ejecutado.

- **Shortest Job First (SJF)**: Aquí, los procesos con los tiempos de ejecución más cortos tienen prioridad para ser ejecutados antes que los procesos con tiempos de ejecución más largos.

Ambas políticas se implementan mediante estructuras de datos específicas (colas) y los procesos se gestionan según el criterio correspondiente.

6- **Sincronización de hilos**: Debido a que el sistema utiliza múltiples hilos para ejecutar diferentes funciones, la sincronización entre ellos es fundamental. Se utilizan mutexes y variables de condición (condition variables) para garantizar que los recursos compartidos, como las colas de procesos y las señales de reloj, sean gestionados de manera segura y eficiente.

7- **Configuración del Sistema**: El sistema es altamente configurable mediante un archivo de configuración. Entre las opciones configurables se incluyen el número de núcleos de CPU (hilos de trabajo), la frecuencia del reloj (en milisegundos), el multiplicador de generación de procesos (que determina la cantidad de procesos generados) y la política de planificación a utilizar (FCFS o SJF). Estos parámetros son leídos desde un archivo de configuración y aplicados al sistema al momento de su inicio.

Este diseño modular permite adaptar fácilmente el sistema a diferentes escenarios de simulación y realizar pruebas comparativas entre las distintas políticas de planificación.

# Diseño e implementación del sistema
La implementación del sistema sigue un enfoque modular y orientado a hilos, donde cada componente del sistema se gestiona de forma independiente, pero trabajando en conjunto. A continuación, se detallan las funciones clave del código que implementan la lógica del sistema.

1- **Generación de Procesos (generate_processes)**:

- La función ``generate_processes()`` es responsable de crear los procesos de forma aleatoria, asignándoles un identificador único (PID) y un tiempo de ejecución estimado dentro de un rango configurable. 

- Los procesos generados son enviados a la cola correspondiente según la política de planificación que se haya seleccionado. En la versión actual, si se utiliza la política **First Come, First Served (FCFS)**, los procesos se encolan en la ProcessQueue. Si se selecciona la política **Shortest Job First (SJF)**, se encolan en la ``PriorityProcessQueue``.

- La función es ejecutada periódicamente en un hilo, lo que asegura que se generen procesos a intervalos regulares.

2- **Encolado de Procesos (enqueue_process)**:

- La función ``enqueue_process()`` se encarga de insertar los procesos generados en la cola correspondiente. Dependiendo de la política de planificación, los procesos se añaden de manera que se respete el orden de ejecución deseado.

- Si la política de planificación es **FCFS**, los procesos se agregan a la ``ProcessQueue`` de manera secuencial. Para **SJF**, los procesos se insertan en la ``PriorityProcessQueue`` de acuerdo con su tiempo de ejecución, asegurando que el proceso con el menor tiempo de ejecución esté al frente de la cola.

3- **Planificación (scheduler_thread)**:

- La función ``scheduler_thread()`` implementa el comportamiento del planificador. Este hilo controla qué proceso se ejecutará a continuación. Si hay procesos listos para ejecutarse y trabajadores disponibles, el planificador asigna un proceso al siguiente hilo de trabajo disponible.

- En esta función, se implementan las políticas de planificación **FCFS** y **SJF**. Para **FCFS**, el planificador simplemente asigna el primer proceso de la cola ``ProcessQueue``. Para **SJF**, el planificador selecciona el proceso con el menor tiempo de ejecución de la PriorityProcessQueue.

- Además, el planificador realiza la sincronización entre los hilos utilizando mutexes y variables de condición, garantizando que los recursos sean compartidos de forma segura y eficiente.

4- **Ejecución de Procesos (worker_thread)**:

- La función ``worker_thread()`` representa los núcleos de la CPU. Cada hilo de trabajo toma un proceso de la cola (según lo asignado por el planificador) y lo ejecuta.

- La ejecución de un proceso es simulada mediante un retraso de tiempo proporcional al tiempo de ejecución estimado del proceso (simulado con ``usleep()``), y el proceso pasa por los estados **READY**, **RUNNING**, y **FINISHED**.

- Durante la ejecución, se realiza la sincronización para garantizar que los hilos no accedan de manera conflictiva a los recursos compartidos.

5- **Temporizador (timer_thread)**:

- La función ``timer_thread()`` simula el reloj del sistema y se ejecuta a intervalos regulares, señalando a los demás hilos cuando deben realizar una acción.

- Este hilo es crucial para mantener el flujo de ejecución en sincronía. Emite señales periódicas para la creación de procesos y el avance de la planificación, asegurando que cada componente del sistema pueda ejecutar sus funciones en el momento adecuado.

7- **Interfaz de Configuración (get_config_value)**:
- Esta función se encarga de leer los parámetros de configuración desde un archivo externo. Estos parámetros incluyen:
    - El número de núcleos de CPU disponibles (hilos de trabajo).
    - El intervalo de tiempo entre la generación de procesos.
    - La política de planificación a utilizar (FCFS o SJF).
    - La frecuencia del reloj (en milisegundos).

- Estos valores son utilizados para configurar el sistema y permitir que el comportamiento del mismo sea modificado sin necesidad de modificar el código directamente.

8- **Sincronización (mutexes y variables de condición)**:

- Para manejar la concurrencia de los hilos y evitar condiciones de carrera, el sistema emplea diversas técnicas de sincronización, como mutexes (``pthread_mutex_t``) y variables de condición (``pthread_cond_t``).

- Por ejemplo, las funciones que manipulan las colas de procesos utilizan mutexes para asegurar que un proceso no sea encolado o desencolado por múltiples hilos al mismo tiempo. Las variables de condición se utilizan para notificar a los hilos cuando es el momento de ejecutar un proceso o cuando hay nuevos procesos en la cola que deben ser atendidos.