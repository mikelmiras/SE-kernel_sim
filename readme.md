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