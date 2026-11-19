# Proyecto integrador — Tercera parte: Coloreado de grafos

Trabajo en equipo · 100 puntos · Curso O2026_MSC2230A

**Entrega en Canvas:** martes 24 de noviembre de 2026 a las 18:59. Disponible desde el 3 de noviembre a las 22:00. La plataforma acepta archivos PDF, DOC, DOCX, ZIP y RAR.

**Productos:** aplicación y código fuente, reporte técnico y video explicativo. El objetivo experimental es determinar si MRV y LCV mejoran la calidad o la eficiencia del coloreado frente a los algoritmos base, usando el polinomio y el número cromático como referencia.

Fuentes: [Consigna en Canvas](https://iteso.instructure.com/courses/58054/assignments/1222162) · [Especificaciones completas del proyecto](https://iteso.instructure.com/courses/58054/files/13282338?wrap=1).

## 1. Objetivo general

Desarrollar una aplicación con interfaz gráfica que reciba y visualice grafos simples no dirigidos, calcule de manera exacta su polinomio cromático P(G,q) y número cromático χ(G), y permita comparar tres algoritmos de coloreado en su versión base y con las heurísticas MRV, LCV o ambas. La comparación se basará en cuatro experimentos y métricas reproducibles; no se utilizará backtracking para el coloreado.

## 2. Alcance y algoritmos obligatorios

- Algoritmo secuencial o Voraz (con el ordenamiento por grado y desempate por “grado de error” descrito en la actividad original).
- Algoritmo Welsh–Powell.
- Algoritmo Matula–Marble–Isaacson (MMI).
- Cálculo exacto de P(G,q) mediante eliminación–contracción y de χ(G) a partir del polinomio.
- Modos seleccionables para cada algoritmo: básico, +MRV, +LCV, +MRV+LCV (12 combinaciones en total).

Nota metodológica: aplicar MRV altera la selección de vértices definida por los algoritmos base. Por tanto, los modos con heurísticas se evaluarán como variantes del algoritmo y no se afirmará que preservan exactamente el procedimiento clásico.

## 3. Entrada del grafo: formato obligatorio

La captura se realizará mediante una lista de vértices y una lista de aristas especificadas por sus dos puntos extremos. El usuario podrá introducir la información en campos de texto o cargar un archivo .txt con el mismo formato. Se deberán conservar los vértices aislados.

```text
4 4
A B C D
A B
A C
B C
B D
```

Interpretación: primera línea “n m” (cantidad de vértices y aristas); segunda línea: n identificadores únicos separados por espacios; después, m líneas “origen destino”, una por arista. En el ejemplo: V={A,B,C,D}, E={AB, AC, BC, BD}.

- Solo se aceptan grafos simples no dirigidos: sin lazos, sin aristas duplicadas y con extremos definidos en V. El orden de los extremos no importa (A B equivale a B A).
- Validaciones: n y m enteros no negativos; identificadores no repetidos; número correcto de aristas; extremos existentes; archivo legible; mensajes de error claros. El grafo vacío puede rechazarse expresamente o tratarse con χ(G)=0, indicando la convención.
- Se introduce aparte el número de colores disponibles k (k≥1) y los nombres/orden de prioridad de esos k colores. No debe confundirse k, parámetro de la prueba, con χ(G).

Internamente se permite lista de adyacencia, matriz de adyacencia o combinación de estructuras; documentar la elección. Para eliminación–contracción deben preservarse correctamente los subproblemas.

## 4. Interfaz gráfica mínima

- Área de entrada: campo o importación de archivo, opción de validar/cargar grafo y controles de k y prioridades de colores.
- Vista gráfica obligatoria: dibujar todos los vértices etiquetados y las aristas del grafo cargado; mostrar los colores asignados tras cada ejecución. Deben verse también los vértices aislados.
- Panel de análisis exacto: botón “Calcular polinomio y número cromático”, visualización de P(G,q) (forma simbólica), χ(G), y P(G,k) para el k seleccionado.
- Panel de coloreado: elegir cualquiera de los tres algoritmos; seleccionar entre básico, MRV, LCV y MRV+LCV; ejecutar y mostrar “se encontró/no se encontró una k-coloración”, asignación o vértice del fallo.
- Panel comparativo: tabla de métricas y opción de reiniciar/restaurar el mismo grafo y k para repetir cada configuración. Exportar resultados a CSV o incluir capturas legibles en el reporte.

## 5. Reglas técnicas para el coloreado y las heurísticas

Coloreado válido: todo vértice recibe un color del conjunto disponible y ningún par adyacente comparte color. Los algoritmos de coloreado son constructivos, sin deshacer decisiones ni realizar backtracking.

- MRV (Minimum Remaining Values): en cada paso seleccionar, entre los vértices no coloreados, aquel con menos colores legales disponibles según los vecinos ya coloreados. Desempatar con la prioridad de vértices del algoritmo base y finalmente con el identificador.
- LCV (Least Constraining Value): para el vértice elegido, probar primero el color legal que reduce menos los dominios de sus vecinos aún no coloreados; desempatar por la prioridad de colores que estableció el usuario.
- Básico: usar el orden y la elección de color del algoritmo original. LCV sin MRV conserva el orden de selección original; MRV sin LCV usa el primer color legal según prioridades.
- Si ningún color es legal para el vértice elegido, detener esa ejecución e informar el conflicto. La salida correcta es “no se encontró una k-coloración con esta estrategia”; no significa que tal coloración sea imposible.

Advertencia para interpretar LCV: si todos los vértices comparten los mismos k colores y las únicas restricciones son desigualdades entre vecinos, frecuentemente todos los colores legales causan la misma pérdida inmediata en los dominios de vecinos. Es válido y relevante obtener empates y ausencia de mejora: deberán registrarse y explicarse.

## 6. Cálculo exacto del polinomio cromático

Implementar eliminación–contracción para una arista e:

P(G,q) = P(G − e,q) − P(G / e,q)

Casos base: si G no tiene aristas, P(G,q)=q^|V|; si una contracción produce un lazo, el subproblema tiene polinomio 0. Las aristas paralelas pueden consolidarse sin modificar las coloraciones propias.

Mostrar el polinomio simplificado o factorizado (con coeficientes enteros), la evaluación P(G,k) y χ(G)=min{q≥1 : P(G,q)>0} para grafos no vacíos. El polinomio cuenta coloraciones propias con colores etiquetados; no obliga a usar los k colores.

Para un grafo pequeño, incluir en el reporte el árbol o registro de llamadas G−e y G/e. Se puede usar memoización. Advertir que el cálculo simbólico puede ser costoso en grafos grandes.

## 7. Cuatro experimentos obligatorios

### Experimento 1. Referencia exacta: P(G,q) y χ(G)

Para cada grafo de prueba, mostrar el dibujo, V, E, el polinomio y χ(G); evaluar P(G,k) al menos para k=χ(G)−1 (si k≥1), k=χ(G) y k=χ(G)+1. Para un grafo pequeño, documentar eliminación–contracción.

Métricas: |V|, |E|, χ(G), P(G,k), número de subproblemas generados y tiempo de cálculo exacto. Concluir para qué k existe matemáticamente una k-coloración.

### Experimento 2. Algoritmos originales

Con el mismo conjunto de grafos y los k del experimento 1, ejecutar Voraz, Welsh–Powell y MMI en modo básico. Comparar sus resultados frente a la referencia exacta.

Métricas: éxito/fracaso, número de vértices coloreados, colores distintos utilizados, verificaciones de conflicto y tiempo. Identificar casos en que P(G,k)>0 y, aun así, la heurística falla.

### Experimento 3. Efecto de MRV

Repetir exactamente los casos del experimento 2 activando únicamente MRV en cada uno de los tres algoritmos. Mantener mismos k, grafo y reglas de desempate.

Métricas: las mismas del experimento 2, más el número de selecciones de vértice y de empates MRV. Comparar los modos base y +MRV por algoritmo. Explicar cuándo cambió el orden de coloración y si ello afectó el éxito.

### Experimento 4. Efecto incremental de LCV y combinación MRV+LCV

Repetir los mismos casos con +LCV y +MRV+LCV en cada algoritmo. Comparar las cuatro configuraciones (básico, MRV, LCV, MRV+LCV) para distinguir el efecto individual y combinado.

Métricas: las anteriores, más número de evaluaciones LCV y empates entre colores legales. Registrar si LCV cambió efectivamente la elección de color o si la mejora fue nula.

## 8. Casos de prueba obligatorios

Aplicar los cuatro experimentos a los mismos grafos. Mínimo: (a) el grafo de la Actividad 17, (b) un ciclo impar, (c) un grafo bipartito y (d) un grafo irregular diseñado por el equipo donde el orden de coloreado pueda influir. Se permite añadir casos pequeños para ilustrar mejor el efecto de MRV.

Caso de verificación (Actividad 17): V={A,B,C,D}; E={AB,AC,BC,BD}.

P(G,q)=q(q−1)²(q−2) = q⁴−4q³+5q²−2q; χ(G)=3.

| k | P(G,k) | Referencia exacta |
| --- | --- | --- |
| 1 | 0 | No existe 1-coloración |
| 2 | 0 | No existe 2-coloración |
| 3 | 12 | Sí existe 3-coloración |
| 4 | 72 | Sí existe 4-coloración |

La entrada visualizada debe coincidir con las aristas de la lista. No se requiere probar todos los valores posibles de k ni todos los grafos existentes.

## 9. Métricas y protocolo de comparación

| Métrica | Definición operativa |
| --- | --- |
| Éxito | 1 si todos los vértices están coloreados sin conflictos; 0 en otro caso. |
| Vértices coloreados | Cantidad asignada antes de terminar o detectar un fallo. |
| Colores utilizados | Cantidad de colores distintos en la asignación completa; si falla, indicar “parcial”. |
| Verificaciones | Número de consultas a la restricción “¿vecinos comparten este color?” durante la ejecución. |
| Tiempo (ms) | Tiempo de ejecución del algoritmo, sin carga del archivo ni dibujo en pantalla. |
| Selecciones / empates MRV | Veces que se evaluaron dominios para escoger vértice y veces que hubo empate mínimo. |
| Evaluaciones / empates LCV | Candidatos legales puntuados y elecciones con igual puntuación mínima. |
| Exactitud de referencia | P(G,k) y χ(G); mostrar por separado subproblemas y tiempo del cálculo exacto. |

Para poder comparar el número de verificaciones, todos los equipos deberán definir y utilizar un contador consistente, incluyendo las comprobaciones necesarias para actualizar MRV y LCV. Describir si se usa caché de dominios. El tiempo depende del equipo; reportar entorno y, de preferencia, mediana de 5 ejecuciones.

Cada caso debe usar exactamente el mismo grafo, k, nombres/prioridad de colores y orden de entrada para las cuatro modalidades de un algoritmo. Los desempates serán deterministas y documentados. Cuando no se encuentre k-coloración, registrar el primer vértice sin color legal y sus vecinos ya coloreados.

## 10. Tabla de resultados requerida

Incluir al menos una tabla como la siguiente por grafo y valor de k. Los valores se llenarán con resultados reales; no se aceptan datos inventados.

| Algoritmo | Modo | Éxito | Colores | Verificaciones | Tiempo (ms) |
| --- | --- | --- | --- | --- | --- |
| Voraz | Básico / MRV / LCV / ambos | — | — | — | — |
| Welsh–Powell | Básico / MRV / LCV / ambos | — | — | — | — |
| MMI | Básico / MRV / LCV / ambos | — | — | — | — |

Complementar con un breve análisis: ¿qué estrategias consiguieron una k-coloración cuando P(G,k)>0? ¿Cambió MRV el orden? ¿LCV cambió la elección o empató? ¿Hubo mejoras de éxito, verificaciones o tiempo? No confundir un fallo heurístico con inexistencia matemática.

## 11. Entregables

Archivo ZIP/RAR que contenga código fuente completo, instrucciones de instalación/ejecución y archivos de entrada de todos los grafos de prueba.

Aplicación ejecutable o reproducible con interfaz, vista gráfica, análisis exacto, selector de algoritmo y los cuatro modos de heurísticas.

Reporte en PDF o Word: portada, objetivo, decisiones de implementación y estructuras de datos, explicación de algoritmos, capturas de interfaz, los cuatro experimentos y tablas de métricas.

Video explicativo del equipo de 3 a 5 minutos, conforme a los requisitos indicados; entregado en MP4 o mediante enlace accesible.

Conclusiones: relación entre P(G,k), χ(G) y resultados heurísticos; beneficios y limitaciones observados de MRV y LCV; limitaciones del método exacto; referencias bibliográficas.

### Video de presentación del proyecto (obligatorio, 10 puntos).

Cada equipo entregará un video de entre 3 y 5 minutos de duración (máximo 5 minutos) en el que sus integrantes expliquen el proyecto y demuestren brevemente el funcionamiento de la aplicación.

Contenido mínimo del video: (1) objetivo del proyecto y formato de entrada del grafo; (2) demostración de la interfaz, visualización del grafo y cálculo de P(G,q) y χ(G); (3) selección y ejecución de al menos uno de los tres algoritmos en modo básico y con MRV, LCV o ambas heurísticas; (4) presentación de métricas de los cuatro experimentos y explicación de si hubo mejoras; (5) conclusión del equipo.

Todos los integrantes deberán participar de forma identificable en la explicación. El video debe permitir leer la interfaz y los resultados; puede elaborarse mediante grabación de pantalla con narración. No es necesario mostrar los rostros.

Entrega: incluir el video en formato MP4 dentro del ZIP/RAR o proporcionar un enlace accesible sin solicitar permisos adicionales. Indicar el enlace en el reporte si se usa esta modalidad. El archivo o enlace debe estar disponible al momento de la evaluación.

Criterio de evaluación del video (10 puntos): demostración correcta del funcionamiento (4), explicación de polinomio/número cromático y comparación MRV/LCV con métricas (3), claridad y participación del equipo (2), duración entre 3 y 5 minutos y accesibilidad del archivo/enlace (1).

## 12. Rúbrica de evaluación (100 puntos)

| Criterio | Puntos |
| --- | --- |
| Entrada validada, formato y visualización del grafo | 15 |
| Cálculo exacto correcto de P(G,q) y χ(G), con evidencia | 20 |
| Implementación de los tres algoritmos base | 20 |
| Modos MRV, LCV y MRV+LCV y sus reglas documentadas | 15 |
| Cuatro experimentos, métricas reproducibles y comparación | 15 |
| Reporte, conclusiones, referencias y entrega reproducible | 5 |
| Video explicativo del equipo (3–5 minutos) | 10 |
| TOTAL | 100 |

## 13. Referencias y notas

Material de clase: “Reporte proyecto coloreado de grafos (2)”; “Actividad 17 – Polinomio y número cromático por eliminación–absorción”.

Russell, S. J., & Norvig, P. (2021). Artificial Intelligence: A Modern Approach (4.ª ed.). Pearson. Capítulo 5: problemas de satisfacción de restricciones (MRV y LCV).

La formulación de estas variantes constructivas sin backtracking y el protocolo de experimentos constituyen una ampliación propuesta para este proyecto; no sustituyen las definiciones de los algoritmos clásicos.

