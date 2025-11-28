# Ordenamiento por Ranking Paralelo con MPI

Este proyecto implementa el algoritmo de **Ordenamiento por Ranking (Ranking Sort)** utilizando el estándar MPI (Message Passing Interface). El diseño se basa en una topología lógica de malla cuadrada de procesadores ($P = p \times p$).

## Descripción General

El objetivo del algoritmo no es mover los elementos físicamente para ordenarlos, sino calcular el **Ranking Global** (posición final) de cada elemento en el conjunto de datos distribuidos.

El sistema simula una matriz de procesadores donde:

  * $N$: Número total de elementos.
  * $P$: Número total de procesos (debe ser un cuadrado perfecto, $P = p^2$).
  * Cada proceso se identifica por coordenadas `(fila, columna)`.

-----

## Fases del Algoritmo

El algoritmo se implementó en 5 fases secuenciales, descritas a continuación:

### Fase 1: Optimización (Input + Gossip Combinados)

En lugar de realizar una distribución seguida de un intercambio de datos (Gossip), **se implementó una distribución directa basada en aritmética modular**.

  * **Lógica:** Se asignan bloques de datos de tamaño $N/p$ directamente a los procesos correspondientes según la fórmula: `group_id % p == rank % p`.
  * **Resultado:** Todos los procesos de una misma columna reciben, desde el inicio, una copia idéntica del subconjunto de datos que les corresponde.
  * **Justificación:** Esta optimización elimina una fase completa de comunicación masiva (Gossip), reduciendo significativamente la latencia inicial y el ancho de banda utilizado.

### Fase 2: Broadcast Horizontal

En esta fase, el proceso situado en la diagonal principal de cada fila actúa como "root" y distribuye sus datos a los demás procesos de su misma fila.

  * **Implementación:** Se utiliza `MPI_Bcast` sobre el comunicador de fila.
  * **Flujo:** El proceso `(i, i)` envía su arreglo a todos los procesos `(i, j)`.

### Fase 3: Ordenamiento Local (Local Sort)

Cada proceso ordena su arreglo de datos **original** (el recibido en la Fase 1) utilizando `std::sort`.

### Fase 4: Ranking Local

Se calcula la posición relativa de los elementos recibidos (del Broadcast) con respecto a los datos locales ordenados.

  * **Lógica:** Para cada elemento $x$ del arreglo *broadcasted*, se cuenta cuántos elementos en el arreglo *local* son menores o iguales a $x$.
  * **Implementación:** Se utilizó la función `std::upper_bound`.

### Fase 5: Reduce Horizontal

Se agregan los resultados parciales calculados en la fase anterior para obtener el ranking global.

  * **Implementación:** Se utiliza `MPI_Reduce` con la operación `MPI_SUM` sobre el comunicador de fila.
  * **Flujo:** Los conteos locales de todos los procesos de la fila $i$ se suman y se envían de vuelta al proceso diagonal $(i, i)$.
  * **Resultado Final:** El proceso diagonal obtiene un arreglo con los rankings finales de sus elementos, lo que indica la posición exacta que ocuparían en el arreglo global totalmente ordenado.
