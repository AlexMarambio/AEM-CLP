# Informe de cambios: backpropagation y memoria global en BSG_VCS_MCTS

## 1. Cambios realizados y motivacion

El codigo ya tenia backpropagation local dentro de MCTS: cada simulacion elegia un nodo, ejecutaba un rollout, calculaba un reward y luego actualizaba los nodos del camino con `visits` y `total_value`. Eso ocurria en `MCTS::backpropagate`.

El problema era que ese aprendizaje se perdia al terminar cada busqueda MCTS local. En `BSG_VCS_MCTS`, se creaba un objeto `MCTS` nuevo para cada estado del beam, por lo que las estadisticas aprendidas no sobrevivian entre estados ni entre niveles del arbol BSG. La mejora implementada agrega una memoria global persistente para que BSG_VCS_MCTS acumule experiencia durante toda la ejecucion.

### Memoria global de decisiones

Se agregaron los archivos:

- `metasolver/mcts/MCTSStats.h`
- `metasolver/mcts/MCTSStats.cpp`

Estos archivos definen una memoria global basada en claves de decision. Para cada decision se guarda:

- cantidad de visitas
- reward total acumulado
- mejor reward observado

Esto permite que una decision temprana que aparece en caminos buenos acumule evidencia historica y pueda influir en decisiones futuras.

### Claves estables para acciones CLP

Se agregaron los archivos:

- `metasolver/mcts/DecisionKey.h`
- `metasolver/mcts/DecisionKey.cpp`

La funcion `make_decision_key(const State& state, const Action& action)` transforma una accion en una clave de texto estable. Esto era necesario porque no sirve identificar acciones por punteros: el codigo clona estados y acciones constantemente, por lo que dos acciones equivalentes pueden estar en direcciones de memoria distintas.

Para acciones CLP, la clave incluye:

- profundidad del estado
- dimensiones del bloque
- volumen ocupado del bloque
- cantidad de cajas del bloque
- composicion por tipo de caja
- limites del espacio libre
- posicion final donde se colocaria el bloque

Con eso, el algoritmo puede reconocer decisiones equivalentes aunque hayan sido generadas en otra busqueda MCTS local.

### Conexion con backpropagation

Se modificaron:

- `metasolver/mcts/MCTS.h`
- `metasolver/mcts/MCTS.cpp`

El constructor de `MCTS` ahora acepta opcionalmente un puntero a `MCTSStats`:

```cpp
MCTS(ActionEvaluator* evaluator, const SolverConfig& config, MCTSStats* global_stats = nullptr);
```

Durante `MCTS::backpropagate`, ademas de actualizar la estadistica local del nodo:

```cpp
node->visits++;
node->total_value += reward;
```

tambien se actualiza la memoria global cuando el nodo tiene padre y accion:

```cpp
global_stats->update(make_decision_key(*node->parent->state, *node->action), reward);
```

Esto hace que el reward final de una simulacion se propague a todas las decisiones del camino, no solo dentro del arbol MCTS temporal.

### Memoria persistente en BSG_VCS_MCTS

Se modificaron:

- `metasolver/strategies/BSG_VCS_MCTS.h`
- `metasolver/strategies/BSG_VCS_MCTS.cpp`

`BSG_VCS_MCTS` ahora tiene un miembro:

```cpp
MCTSStats mcts_stats;
```

Y cada vez que crea un MCTS local, le pasa esa memoria:

```cpp
MCTS mcts(evl, config, &mcts_stats);
```

Esto es lo que permite que el aprendizaje sobreviva entre llamadas sucesivas a MCTS.

### Uso de memoria global en el scoring

En la segunda etapa, la memoria global empezo a influir en los scores usados para ordenar acciones candidatas.

Antes, `MCTSResult::action_scores` usaba solo el promedio local:

```cpp
child->total_value / child->visits
```

Ahora, si existe estadistica global suficiente para esa decision, se mezcla el score local con el promedio historico:

```cpp
score = (1.0 - weight) * local_score + weight * global_score;
```

Esto permite que decisiones parecidas a otras que antes llevaron a buenos rollouts reciban una ventaja moderada en futuras selecciones.

La mezcla es configurable para poder comparar facilmente:

- `mcts_global_score_weight = 0.0`: no usa memoria global en scoring
- `mcts_global_score_weight = 0.25`: usa 25% memoria global y 75% score local
- `mcts_global_score_weight = 1.0`: usa solo memoria global cuando exista

### Reporte de metricas

Se extendio `MCTSResult` con metricas nuevas:

- `global_stats_size`
- `global_stats_updates`
- `global_average_reward`
- `global_score_hits`
- `average_global_score`

Estas metricas permiten saber si la memoria se esta llenando y si realmente esta influyendo en el score final.

Tambien se modifico `problems/clp/main_clp.cpp` para imprimir esas metricas en modo `bsg_vcs_mcts` y `compare`.

### Compilacion

Se modifico `CMakeLists.txt` para incluir los nuevos archivos:

- `metasolver/mcts/DecisionKey.cpp`
- `metasolver/mcts/MCTSStats.cpp`

Sin este cambio, las nuevas clases compilarian como headers pero fallaria el enlace del ejecutable.

## 2. Manual de funciones, clases y flags agregados

### Clase `MCTSStats`

Archivo:

```text
metasolver/mcts/MCTSStats.h
```

Sirve como memoria global de aprendizaje para MCTS.

Funciones principales:

```cpp
void update(const std::string& key, double reward);
```

Actualiza la estadistica de una decision. Incrementa visitas, suma reward y actualiza el mejor reward observado.

```cpp
bool find(const std::string& key, MCTSActionStats& stats) const;
```

Busca una decision en la memoria. Retorna `true` si existe.

```cpp
double mean_value(const std::string& key) const;
```

Retorna el reward promedio de una decision. Si no existe, retorna `0.0`.

```cpp
int size() const;
```

Retorna cuantas decisiones distintas hay en memoria.

```cpp
int total_updates() const;
```

Retorna cuantas actualizaciones totales se han hecho.

```cpp
double average_reward() const;
```

Retorna el promedio global de todos los rewards almacenados.

### Struct `MCTSActionStats`

Archivo:

```text
metasolver/mcts/MCTSStats.h
```

Campos:

```cpp
int visits;
double total_reward;
double max_reward;
```

Representa la estadistica acumulada de una decision especifica.

### Funcion `make_decision_key`

Archivo:

```text
metasolver/mcts/DecisionKey.h
```

Firma:

```cpp
std::string make_decision_key(const State& state, const Action& action);
```

Construye una clave estable para identificar decisiones. En CLP usa informacion del bloque, espacio y profundidad. Si la accion no es `clpAction`, genera una clave generica basada en la profundidad.

Uso esperado:

```cpp
std::string key = make_decision_key(*node->parent->state, *node->action);
global_stats->update(key, reward);
```

### Constructor nuevo de `MCTS`

Archivo:

```text
metasolver/mcts/MCTS.h
```

Firma:

```cpp
MCTS(ActionEvaluator* evaluator, const SolverConfig& config, MCTSStats* global_stats = nullptr);
```

El tercer parametro es opcional. Si se pasa `nullptr`, MCTS funciona como antes, solo con estadisticas locales. Si se pasa una memoria `MCTSStats`, el backpropagation tambien actualiza la memoria global.

Uso actual en `BSG_VCS_MCTS`:

```cpp
MCTS mcts(evl, config, &mcts_stats);
```

### Metodo `fill_global_stats_report`

Archivo:

```text
metasolver/mcts/MCTS.cpp
```

Firma:

```cpp
void fill_global_stats_report(MCTSResult& report) const;
```

Copia al reporte las metricas actuales de memoria global:

- cantidad de decisiones aprendidas
- cantidad de actualizaciones
- reward promedio global

### Nuevos campos de `MCTSResult`

Archivo:

```text
metasolver/mcts/MCTS.h
```

Campos agregados:

```cpp
int global_stats_size;
int global_stats_updates;
double global_average_reward;
int global_score_hits;
double average_global_score;
```

Significado:

- `global_stats_size`: cuantas claves distintas existen en la memoria global.
- `global_stats_updates`: cuantas veces se actualizo la memoria global.
- `global_average_reward`: reward promedio de todas las actualizaciones globales.
- `global_score_hits`: cuantas acciones candidatas usaron memoria global para calcular su score.
- `average_global_score`: promedio de los scores globales usados en esa busqueda.

### Nuevos parametros de `SolverConfig`

Archivo:

```text
metasolver/SolverConfig.h
```

Campos agregados:

```cpp
double mcts_global_score_weight;
int mcts_min_global_visits;
```

Valores por defecto:

```cpp
mcts_global_score_weight(0.25)
mcts_min_global_visits(2)
```

`mcts_global_score_weight` controla cuanto pesa la memoria global en el score final.

`mcts_min_global_visits` controla cuantas visitas debe tener una decision antes de confiar en su promedio historico.

### Flag `--mcts_global_score_weight`

Define el peso de la memoria global en el score de una accion.

Ejemplos:

```bash
--mcts_global_score_weight 0
```

Desactiva la influencia global en el scoring. La memoria puede seguir registrandose, pero no cambia la decision.

```bash
--mcts_global_score_weight 0.25
```

Usa 25% memoria global y 75% score local.

```bash
--mcts_global_score_weight 1.0
```

Usa solo el score global cuando exista memoria suficiente.

### Flag `--mcts_min_global_visits`

Define el minimo de visitas globales necesarias para usar una decision en el scoring.

Ejemplos:

```bash
--mcts_min_global_visits 1
```

Usa la memoria muy temprano. Sirve para pruebas cortas, pero puede ser ruidoso.

```bash
--mcts_min_global_visits 2
```

Valor por defecto. Exige un minimo de repeticion antes de usar la memoria.

```bash
--mcts_min_global_visits 5
```

Mas conservador. Reduce el riesgo de sesgarse por pocos rollouts.

### Ejemplos de ejecucion

Ejecutar BSG con MCTS y memoria global activa:

```bash
./build/BSG_CLP problems/clp/benchs/BR/BR4.txt -i 1 -t 1 -f BR --mode bsg_vcs_mcts
```

Ejecutar con memoria global registrada pero sin influir en el score:

```bash
./build/BSG_CLP problems/clp/benchs/BR/BR4.txt -i 1 -t 1 -f BR --mode bsg_vcs_mcts --mcts_global_score_weight 0
```

Ejecutar una prueba corta donde sea facil observar `global_score_hits`:

```bash
./build/BSG_CLP problems/clp/benchs/BR/BR4.txt -i 1 -t 1 -f BR --mode bsg_vcs_mcts --mcts_iterations 5 --mcts_depth_limit 3 --mcts_time_limit_seconds 0.1 --mcts_global_score_weight 0.25 --mcts_min_global_visits 1
```

Comparar contra BSG_VCS:

```bash
./build/BSG_CLP problems/clp/benchs/BR/BR4.txt -i 1 -t 10 -f BR --mode compare
```

### Como interpretar las metricas

Si `mcts global stats` es mayor que cero, la memoria global esta almacenando decisiones.

Si `mcts global updates` crece, el backpropagation esta alimentando esa memoria.

Si `mcts global score hits` es mayor que cero, la segunda etapa esta activa: alguna accion candidata uso memoria global para ajustar su score.

Si `mcts global score hits` es cero, puede ser por tres razones:

- `--mcts_global_score_weight 0`
- ninguna accion candidata tenia memoria previa
- las acciones tenian menos visitas que `--mcts_min_global_visits`

Para depurar rapido, se puede bajar temporalmente:

```bash
--mcts_min_global_visits 1
```

