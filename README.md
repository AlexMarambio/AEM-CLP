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

El flujo actual es:

1. VCS evalua y ordena las acciones candidatas de cada estado.
2. Greedy completa cada candidato para estimar su calidad.
3. BSS conserva los mejores candidatos segun el ancho del beam.
4. Los candidatos que antes se descartaban quedan guardados en una pila.
5. Cuando una rama activa se agota, BSS vuelve al ultimo nivel con alternativas
   pendientes y continua la busqueda hasta acabar el tiempo o la pila.

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

Opciones principales:

```text
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
```

## Tests

```sh
ctest --test-dir build
```
