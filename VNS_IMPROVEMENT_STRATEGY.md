# VNS Improvement Strategy - Plan de Mejora

## Diagnóstico Actual

**Rendimiento:**
- BR0.txt (homogéneo): VNS = 91.66% ✅ Matching BSG
- BR4.txt (heterogéneo): VNS = 81.37% vs BSG = 95.59% ❌ Diferencia: -14.22%
- BR8.txt (heterogéneo): VNS = 83.58% vs BSG = 94.92% ❌ Diferencia: -11.34%

**Root cause:** Dos de tres operadores (BlockSwap, BlockRotate) son stubs.

---

## Plan de Mejora (Ordenado por Impacto)

### 🔴 PRIORIDAD 1: Completar Implementación de Operadores
**Impacto: MÁXIMO (++)  |  Esfuerzo: 40-45 min  |  Riesgo: Bajo**

#### 1a. Implementar BlockSwapOperator::performSwap()
- **Problema actual:** Stub, siempre retorna NULL
- **Mejora:** Intercambiar posiciones de dos bloques colocados
- **Impacto esperado:** +5-10% en BR4/BR8
- **Tiempo:** ~15 min

#### 1b. Implementar BlockRotateOperator::attemptRotationAndReplacement()
- **Problema actual:** Stub, siempre retorna NULL
- **Mejora:** Rotar bloque y reinsertar en nueva orientación
- **Impacto esperado:** +3-8% en BR4/BR8
- **Tiempo:** ~20 min

---

### 🟠 PRIORIDAD 2: Mejorar Estrategia de Perturbación
**Impacto: ALTO (+)  |  Esfuerzo: 15-20 min  |  Riesgo: Bajo**

#### 2a. Perturbación Inteligente (vs Aleatoria)
**Mejora en VNS.cpp::perturb():**

```cpp
// ACTUAL (Aleatoria):
for (int i = 0; i < num_perturbations; i++) {
    int random_op = rand() % getNumberOfOperators();
    State* s_perturbed = op->generateNeighbor(*s, NULL);
}

// PROPUESTO (Inteligente):
// 1. Usar operadores en ciclo (N1 → N2 → N3 → N1)
// 2. Variar intensidad según operador (N1 es suave, N3 es fuerte)
// 3. Aplicar múltiples perturbaciones del mismo operador
```

**Implementación:**
```cpp
void VNS::perturb(State*& s) {
    int num_perturbations = max(1, (int)(perturbation_strength * 2));
    
    for (int i = 0; i < num_perturbations && get_time() < timelimit; i++) {
        // Usar operadores en orden (ciclo)
        int op_index = (i % getNumberOfOperators());
        NeighborhoodOperator* op = getOperator(op_index);
        
        if (op != NULL) {
            for (int j = 0; j < (i+1); j++) {  // Aumentar intensidad
                State* s_perturbed = op->generateNeighbor(*s, NULL);
                if (s_perturbed != NULL) {
                    if (s != NULL && s != s_perturbed) delete s;
                    s = s_perturbed;
                }
            }
        }
    }
}
```

**Impacto esperado:** +2-4%  
**Tiempo:** ~15 min

---

### 🟡 PRIORIDAD 3: Ajustar Parámetros de VNS
**Impacto: MODERADO (+)  |  Esfuerzo: 10-15 min  |  Riesgo: Muy bajo**

#### 3a. Aumentar max_perturbation
**Actual:** `0.3` (máximo 30% de bloques removidos)  
**Propuesto:** `0.4-0.5` (40-50% para más exploración)

```cpp
// En main_clp.cpp línea 162:
// VNS* vns = new VNS(vcs, 0.3, 5);  // ACTUAL
VNS* vns = new VNS(vcs, 0.5, 7);     // PROPUESTO
```

**Impacto:** +1-2% en instancias heterogéneas  
**Razón:** Más exploración en fase de perturbación

#### 3b. Aumentar max_vnd_iterations
**Actual:** `5` iteraciones de VND  
**Propuesto:** `7-10` iteraciones

**Impacto:** +1-3% (mejor aprovechamiento de tiempo)

#### 3c. Ajustar initial_perturbation_strength
**Actual:** Empieza en 0.1, sube a max en pasos de 1.2x  
**Propuesto:** Empezar en 0.15, ajustar incremento

```cpp
// En VNS.cpp::resetPerturbationStrength():
// perturbation_strength = 0.1;  // ACTUAL
perturbation_strength = 0.15;     // PROPUESTO (más agresivo inicialmente)
```

**Impacto esperado:** +1-2%  
**Tiempo:** ~10 min

---

### 🟢 PRIORIDAD 4: Mejorar RemoveReinsertOperator
**Impacto: MODERADO (+)  |  Esfuerzo: 20-25 min  |  Riesgo: Bajo-medio**

#### 4a. Usar Reinserción Múltiple
**Problema actual:** Una sola reinserción por bloque removido  
**Mejora:** Intentar múltiples posiciones para maximizar mejora

```cpp
// En RemoveReinsertOperator.cpp::findBestNeighbor():

// ACTUAL:
State* best_neighbor = NULL;
for (...) {
    State* candidate = ...
    if (candidate better than best_neighbor)
        best_neighbor = candidate;
}

// PROPUESTO: Más determinista, menos aleatorio
// - Seleccionar bloques malos (peor fit) preferentemente
// - Para cada bloque removido, buscar entre top-3 posiciones
// - Mantener mejor de las N búsquedas
```

**Impacto esperado:** +1-3%  
**Tiempo:** ~15 min

#### 4b. Priorizar Bloques Grandes
**Idea:** Remover primero bloques grandes (más espacio liberado)

```cpp
// Ordenar bloques por volumen antes de iterar
sort(blocks.begin(), blocks.end(), 
     [](Block* a, Block* b) { 
         return a->volume() > b->volume(); 
     });
```

**Impacto:** +0.5-1%  
**Tiempo:** ~10 min

---

### 💙 PRIORIDAD 5: Agregar Rastreo de Operador Útil
**Impacto: BAJO (→)  |  Esfuerzo: 20-25 min  |  Riesgo: Medio**

#### 5a. Frecuencia de Operadores
**Idea:** Rastrear cuál operador hace más mejoras, usarlo más.

```cpp
// En VNS.h:
vector<long> operator_improvements;
vector<long> operator_evaluations;

// En VNS.cpp::variableNeighborhoodDescent():
if (val_neighbor > s_best->get_value()) {
    operator_improvements[op_index]++;
}
operator_evaluations[op_index]++;

// En perturb(): probabilidad proporcional a mejoras
double improvement_rate = (double)operator_improvements[i] / 
                          max(1L, operator_evaluations[i]);
```

**Impacto:** +0.5-1.5%  
**Tiempo:** ~20 min

---

## Orden Recomendado de Implementación

```
Sesión 1 (45 min - MÁS CRÍTICO):
├─ BlockSwapOperator implementation (15 min) ← MÁXIMO IMPACTO
├─ BlockRotateOperator implementation (20 min) ← MÁXIMO IMPACTO
└─ Compilar y validar en BR4/BR8 (10 min)

Sesión 2 (30 min - OPTIMIZACIÓN):
├─ Mejora: Perturbación Inteligente (15 min) ← ALTO IMPACTO
├─ Ajuste: Parámetros VNS (10 min) ← FÁCIL
└─ Recompilar y validar (5 min)

Sesión 3 (20 min - FINO):
├─ RemoveReinsertOperator mejorado (15 min)
└─ Test final (5 min)
```

---

## Resultados Esperados

**Escenario 1 (Prioridad 1 solo - 45 min):**
- BR0: 91.66% → 91.66% ✅
- BR4: 81.37% → 88-92% (estimado +7-11%)
- BR8: 83.58% → 88-92% (estimado +5-9%)
- **Impacto total:** BSG matching o superior

**Escenario 2 (Prioridades 1+2+3 - 2 horas):**
- BR0-BR3: 91-95% → 92-96% (+1-2%)
- BR4-BR7: 85-93% → 89-96% (+4-10%)
- BR8-BR15: 80-92% → 88-96% (+6-12%)
- **Impacto total:** 4-6% mejora promedio

**Escenario 3 (Todas las prioridades - 3+ horas):**
- Posible mejora 5-8% en promedio
- VNS potentially mejor que BSG en instancias grandes

---

## Monitoreo de Progreso

Crear tabla de progreso:

| Mejora | Estado | Impacto Est. | Tiempo | Resultado |
|--------|--------|-------------|---------|-----------|
| BlockSwap | ⏳ TODO | +5-10% | 15 min | - |
| BlockRotate | ⏳ TODO | +3-8% | 20 min | - |
| Pert. Inteligente | ⏳ TODO | +2-4% | 15 min | - |
| Parámetros | ⏳ TODO | +1-3% | 10 min | - |
| RemoveReinserter | ⏳ TODO | +1-2% | 15 min | - |

---

## Estrategia de Testing

Para cada mejora, ejecutar:
```bash
# Compilar
cd build && make

# Test en instancia de cada tipo
./BSG_CLP ../problems/clp/benchs/BR/BR0.txt -i 0 -t 15 -f BR --strategy vns
./BSG_CLP ../problems/clp/benchs/BR/BR4.txt -i 1 -t 15 -f BR --strategy vns
./BSG_CLP ../problems/clp/benchs/BR/BR10.txt -i 5 -t 15 -f BR --strategy vns

# Comparar
./BSG_CLP ../problems/clp/benchs/BR/BR0.txt -i 0 -t 15 -f BR
./BSG_CLP ../problems/clp/benchs/BR/BR4.txt -i 1 -t 15 -f BR
./BSG_CLP ../problems/clp/benchs/BR/BR10.txt -i 5 -t 15 -f BR
```

---

## Notas Importantes

1. **BlockSwap/BlockRotate son críticos** - Sin implementar, VNS no puede competir
2. **Perturbación inteligente es fácil y efectivo** - Cambio simple con impacto
3. **Parámetros son delicados** - Aumentar ambos simultáneamente
4. **Testing es esencial** - Validar después de cada cambio
5. **Tiempo disponible es limitado** - Priorizar Prioridad 1 primero

---

## ¿Por Qué VNS Debería Ser Mejor que BSG?

1. **VNS explora múltiples vecindarios** - BSG es solo búsqueda por haz único
2. **VNS escapa de óptimos locales** - BSG se queda atrapado fácilmente
3. **VNS adapta perturbación** - Según progreso de búsqueda
4. **VNS es más flexible** - Múltiples operadores = múltiples estrategias

**La clave:** Mantener diversidad de búsqueda mientras se mejora localmente.

---

## Próximo Paso Recomendado

**EMPEZAR CON PRIORIDAD 1:** Implementar BlockSwapOperator y BlockRotateOperator.

Esto es crítico porque sin estos dos operadores, VNS está usando solo 1/3 de su potencial.

