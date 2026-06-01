# BSG for CLP

Repositorio reducido para conservar el nucleo del algoritmo BSG aplicado al
Container Loading Problem y los benchmarks necesarios para ejecutarlo.
`Basado en el excelente Metasolver de I. Araya`
- Este fork tiene como única finalidad la experimentación y aprendizaje en base al material original.

## Estructura

- `metasolver/`: abstracciones de estado, acciones y estrategias de busqueda.
- `metasolver/strategies/`: implementacion de BSG, Greedy y DoubleEffort.
- `problems/clp/`: modelo del problema CLP y ejecutable principal.
- `problems/clp/benchs/`: instancias benchmark conservadas.
- `lib/bullet-2.80/src/`: headers y librerias de Bullet usados por el build.

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
