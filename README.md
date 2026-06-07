# BSS for CLP

Repositorio reducido para conservar el nucleo del algoritmo aplicado al
Container Loading Problem y los benchmarks necesarios para ejecutarlo.
`Basado en el excelente Metasolver de I. Araya`
- Este fork tiene como única finalidad la experimentación y aprendizaje en base al material original.

## Estructura

- `metasolver/`: abstracciones de estado, acciones y estrategias de busqueda.
- `metasolver/strategies/`: implementacion de Beam Stack Search y Greedy.
- `problems/clp/`: modelo del problema CLP y ejecutable principal.
- `problems/clp/benchs/`: instancias benchmark conservadas.
- `lib/bullet-2.80/src/`: headers y librerias de Bullet usados por el build.

## Que cambio

La estrategia `BSG` fue reemplazada internamente por una version Beam Stack
Search (BSS), manteniendo la misma interfaz publica. No se modificaron las
estructuras core del problema CLP ni el algoritmo VCS.

### Flujo Base (BSS)

1. VCS evalua y ordena las acciones candidatas de cada estado.
2. Greedy completa cada candidato para estimar su calidad.
3. BSS conserva los mejores candidatos segun el ancho del beam.
4. Los candidatos que antes se descartaban quedan guardados en una pila.
5. Cuando una rama activa se agota, BSS vuelve al ultimo nivel con alternativas
   pendientes y continua la busqueda hasta acabar el tiempo o la pila.

### MCTS como Heuristica Complementaria (Opcional)

Se incorporo MCTS (Monte Carlo Tree Search) como reranker opcional que:

1. **Actua como filtro de verificacion**: Toma los candidatos que VCS ordeno.
2. **Simula con rollouts**: Ejecuta varias simulaciones de cada candidato usando:
   - Acciones seleccionadas por VCS durante cada paso del rollout.
   - Profundidad configurable de rollout.
3. **Reordena candidatos**: Basandose en:
   - Promedio de recompensas (valor MCTS).
   - Posicion original en VCS (opcional).
   - Mejor valor visto en simulaciones.
4. **Permite rescate de candidatos**: Si un candidato que VCS rankeo bajo obtiene
   mejores resultados en simulacion, MCTS lo sube en la lista.
5. **Mantiene compatibilidad**: Si MCTS no esta habilitado, el flujo es identico
   a BSS puro.

En la salida normal veras mensajes como:

```text
[BSS] new best_solution_found (...): ...
```

Eso indica que la busqueda encontro una mejor solucion completa.

## Compilar

```sh
cmake -S . -B build
cmake --build build
```

## Ejecutar

### Sin MCTS (Baseline)

```sh
./build/BSG_CLP problems/clp/benchs/BR/BR2.txt -i 47 -t 10 -f BR
```

Si estas dentro de la carpeta `build/`, usa la ruta relativa al directorio
padre:

```sh
./BSG_CLP ../problems/clp/benchs/BR/BR2.txt -i 47 -t 10 -f BR
```

Otros ejemplos:

```sh
./build/BSG_CLP problems/clp/benchs/BR/BR4.txt -i 1 -t 5 -f BR
./build/BSG_CLP problems/clp/benchs/BR/BR8.txt -i 0 --min_fr=0.98 -t 30 -f BR
```

### Con MCTS (Reranking de Candidatos)

MCTS se activa con la bandera `--mcts` y se configura con parametros opcionales:

```sh
./build/BSG_CLP problems/clp/benchs/BR/BR2.txt -i 47 -t 10 -f BR \
  --mcts --mcts_iter=50 --mcts_depth=10 --mcts_width=3 --mcts_c=1.4
```

Equivalente con valores por defecto:

```sh
./build/BSG_CLP problems/clp/benchs/BR/BR2.txt -i 47 -t 10 -f BR --mcts
```

**Nota**: Los parametros MCTS solo se usan si `--mcts` esta habilitado.

Opciones principales:

```text
CONFIGURACION BASICA:
-i[int]                 Instancia
-f[string]              Formato: BR, BRw o 1C
--min_fr=[double]       Volumen minimo ocupado por bloque
-t[int], --timelimit    Tiempo maximo
--seed=[int]            Semilla aleatoria
--alpha=[double]        Parametro alpha
--beta=[double]         Parametro beta
--gamma=[double]        Parametro gamma
--delta=[double]        Parametro delta
-p[double]              Parametro p
--fsb                   Usar bloques con soporte completo
--trace                 Activar trazas

MCTS - MONTE CARLO TREE SEARCH (Opcional):
--mcts                  Activar MCTS como reranker de candidatos VCS
--mcts_iter=[int]       Iteraciones MCTS por estado expandido (default: 50)
--mcts_top=[int]        Cantidad de candidatos VCS reevaluados por MCTS
                        0 = todos los candidatos (default: 0)
--mcts_depth=[int]      Profundidad maxima de rollout (default: 10)
--mcts_width=[int]      Acciones VCS consideradas en cada paso del rollout
                        (default: 3)
--mcts_c=[double]       Constante de exploracion UCB (default: 1.4)
--mcts_vcs_weight=[double]
                        Peso del ranking original VCS en reordenamiento
                        (default: 0.0)
--mcts_mcts_weight=[double]
                        Peso del promedio MCTS en reordenamiento
                        (default: 1.0)
--mcts_no_greedy        Desactivar completacion greedy despues de rollout
```

## Decisiones de Diseño: MCTS Limpio

### Principios

1. **No contaminar core CLP**: VCS y clpState se quedan intactos.
2. **Reranker, no reemplazo**: MCTS verifica candidatos VCS, no genera nuevos.
3. **Backward compatible**: Sin `--mcts`, el flujo es identico a BSS puro.
4. **Overhead controlable**: Parametros permiten ajustar tiempo/calidad.

### Separacion Clara

- **MCTSGreedy.h/cpp**: Logica MCTS aislada (simulacion, UCB, rollout).
- **BSG.h/cpp**: Solo llama `mcts->rerank_candidates()` si esta habilitado.
- **main_clp.cpp**: Configura parametros desde CLI.

### Evaluacion Comparativa

Para comparar sin MCTS vs con MCTS en el mismo benchmark:

```bash
# Linea base (sin MCTS)
time ./build/BSG_CLP problems/clp/benchs/BR/BR2.txt -i 47 -t 60 -f BR

# Con MCTS (iter=50, profundidad 10)
time ./build/BSG_CLP problems/clp/benchs/BR/BR2.txt -i 47 -t 60 -f BR \
  --mcts --mcts_iter=50 --mcts_depth=10 --mcts_width=3 --mcts_c=1.4

# Con MCTS mas agresivo (iter=100, profundidad 15)
time ./build/BSG_CLP problems/clp/benchs/BR/BR2.txt -i 47 -t 60 -f BR \
  --mcts --mcts_iter=100 --mcts_depth=15 --mcts_width=5 --mcts_c=1.4
```

Las primeras lineas de salida mostraran:
- `mcts: off` → modo baseline
- `mcts: iter=... depth=... width=... c=...` → modo MCTS con configuracion

### Riesgos Mitigados

| Riesgo | Mitigacion |
|--------|-----------|
| Costo temporal alto | `--mcts_iter` y `--mcts_depth` controlan esfuerzo |
| Fuga de memoria | Cada simulacion clona y libera correctamente |
| Comparacion injusta | Mismo timelimit para ambos modos |

## Tests

```sh
ctest --test-dir build
```
