/*
 * BlockSwapOperator.h
 *
 * Operador de vecindario: Intercambio de Bloques
 *
 * Principio: Intercambia las posiciones de dos bloques colocados.
 * Si el intercambio es válido (sin solapamientos), es un movimiento válido.
 * 
 * Complejidad: O(|blocks|²) por vecino
 *
 *  Created on: 01-06-2026
 */

#ifndef BLOCKSWAPOPERATOR_H_
#define BLOCKSWAPOPERATOR_H_

#include "NeighborhoodOperator.h"
#include "../problems/clp/clpState.h"

using namespace std;
using namespace clp;

namespace metasolver {

/**
 * Operador BlockSwap para Container Loading Problem.
 * 
 * Intercambia posiciones de dos bloques colocados.
 * Validaciones:
 * - Ambos bloques deben caber en las posiciones del otro
 * - No debe haber solapamiento
 * - Ambos deben seguir dentro del contenedor
 */
class BlockSwapOperator : public NeighborhoodOperator {
public:
	/**
	 * Constructor
	 * @param evaluator Evaluador para validar swaps
	 */
	BlockSwapOperator(ActionEvaluator* evaluator)
		: NeighborhoodOperator("BlockSwap"), 
		  evaluator(evaluator),
		  max_swap_attempts(10) {}
	
	virtual ~BlockSwapOperator() {}
	
	/**
	 * Genera un vecino intercambiando dos bloques aleatorios.
	 * @param s Estado actual (clpState)
	 * @param s_best Mejor solución (no usado)
	 * @return Nuevo estado con bloques intercambiados, o NULL si no es válido
	 */
	virtual State* generateNeighbor(State& s, State* s_best = NULL);
	
	/**
	 * Genera múltiples vecinos intercambiando pares diferentes de bloques.
	 * @param s Estado actual
	 * @param neighbors Lista de vecinos generados
	 * @param max_neighbors Si > 0, limita número de vecinos
	 */
	virtual void generateNeighborhood(State& s, list<State*>& neighbors, int max_neighbors = 0);
	
	/**
	 * Busca el mejor par de bloques a intercambiar.
	 * Explora todas las combinaciones.
	 * @param s Estado actual
	 * @param s_best Mejor solución conocida
	 * @param evaluator Evaluador
	 * @return Mejor vecino encontrado (mejora), o NULL
	 */
	virtual State* findBestNeighbor(State& s, State* s_best, ActionEvaluator* evaluator);
	
	/**
	 * Búsqueda exhaustiva: intenta intercambiar TODOS los pares válidos.
	 * @param s Estado actual (se modifica)
	 * @param s_best Mejor solución (se actualiza si mejora)
	 * @param evaluator Evaluador
	 * @return true si encontró mejora
	 */
	virtual bool exhaustiveSearch(State& s, State*& s_best, ActionEvaluator* evaluator);
	
	/**
	 * Establece máximo de intentos de swap
	 */
	void setMaxSwapAttempts(int max_attempts) {
		max_swap_attempts = max_attempts;
	}
	
private:
	ActionEvaluator* evaluator;
	int max_swap_attempts;
	
	/**
	 * Valida si dos bloques pueden intercambiar posiciones.
	 * @param s Estado actual
	 * @param block1_idx Índice del primer bloque en el contenedor
	 * @param block2_idx Índice del segundo bloque en el contenedor
	 * @return true si el intercambio es válido
	 */
	bool isValidSwap(const clpState& s, int block1_idx, int block2_idx) const;
	
	/**
	 * Realiza el intercambio de dos bloques.
	 * Pre: isValidSwap debe retornar true
	 * @param s Estado donde realizar el swap
	 * @param block1_idx Índice del primer bloque
	 * @param block2_idx Índice del segundo bloque
	 * @return Nuevo estado con bloques intercambiados
	 */
	clpState* performSwap(clpState& s, int block1_idx, int block2_idx);
};

} /* namespace metasolver */

#endif /* BLOCKSWAPOPERATOR_H_ */
