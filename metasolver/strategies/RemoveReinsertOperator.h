/*
 * RemoveReinsertOperator.h
 *
 * Operador de vecindario: Remover y Reinsertar
 *
 * Principio: Remueve un bloque de su posición actual y lo reinserta
 * en una posición potencialmente mejor dentro del contenedor.
 * 
 * Variantes:
 * 1. RemoveReinsert-Box: Remueve una caja individual
 * 2. RemoveReinsert-Block: Remueve un bloque completo
 *
 * Complejidad: O(|blocks| × |spaces|) por vecino
 *
 *  Created on: 01-06-2026
 */

#ifndef REMOVEREINSERTOPERATOR_H_
#define REMOVEREINSERTOPERATOR_H_

#include "NeighborhoodOperator.h"
#include "../problems/clp/clpState.h"

using namespace std;
using namespace clp;

namespace metasolver {

/**
 * Operador RemoveReinsert para el Container Loading Problem.
 * 
 * Funciona con clpState:
 * - Identifica bloques colocados en el contenedor
 * - Remueve un bloque
 * - Regenera espacios libres
 * - Reinenta colocar el bloque en mejor posición
 * - Si no mejora, deshace el cambio
 */
class RemoveReinsertOperator : public NeighborhoodOperator {
public:
	/**
	 * Constructor
	 * @param evaluator Evaluador para comparar soluciones
	 * @param block_level true=operar en bloques, false=en cajas individuales
	 */
	RemoveReinsertOperator(ActionEvaluator* evaluator, bool block_level = true)
		: NeighborhoodOperator("RemoveReinsert"), 
		  evaluator(evaluator), 
		  block_level(block_level),
		  max_reinsertion_attempts(5) {}
	
	virtual ~RemoveReinsertOperator() {}
	
	/**
	 * Genera un vecino aleatorio removiendo e reinsertando un bloque.
	 * @param s Estado actual (clpState)
	 * @param s_best Mejor solución (no usado en este operador)
	 * @return Nuevo estado vecino con bloque reinsertado
	 */
	virtual State* generateNeighbor(State& s, State* s_best = NULL);
	
	/**
	 * Genera múltiples vecinos removiendo diferentes bloques.
	 * @param s Estado actual
	 * @param neighbors Lista de vecinos generados
	 * @param max_neighbors Si > 0, limita el número de vecinos
	 */
	virtual void generateNeighborhood(State& s, list<State*>& neighbors, int max_neighbors = 0);
	
	/**
	 * Busca el mejor bloque a remover y reinsertar.
	 * Explora todos los bloques colocados.
	 * @param s Estado actual
	 * @param s_best Mejor solución conocida
	 * @param evaluator Evaluador de acciones
	 * @return Mejor vecino encontrado (mejora), o NULL
	 */
	virtual State* findBestNeighbor(State& s, State* s_best, ActionEvaluator* evaluator);
	
	/**
	 * Búsqueda exhaustiva: intenta remover y reinsertar TODOS los bloques.
	 * @param s Estado actual (se modifica)
	 * @param s_best Mejor solución encontrada
	 * @param evaluator Evaluador de acciones
	 * @return true si se encontró mejora
	 */
	virtual bool exhaustiveSearch(State& s, State*& s_best, ActionEvaluator* evaluator);
	
	/**
	 * Establece el número máximo de intentos de recolocación
	 */
	void setMaxReinsertionAttempts(int max_attempts) {
		max_reinsertion_attempts = max_attempts;
	}
	
private:
	ActionEvaluator* evaluator;
	bool block_level;                   // true = operar en bloques, false = en cajas
	int max_reinsertion_attempts;       // Máximo número de posiciones a intentar
	
	/**
	 * Obtiene el índice de un bloque colocado en el contenedor.
	 * (Ayuda interna para identificar bloques)
	 */
	int getBlockIndexInContainer(const clpState& s, const Block& block) const;
	
	/**
	 * Intenta reinsertar un bloque en una posición mejor.
	 * @param s Estado donde reinsertar
	 * @param block Bloque a reinsertar
	 * @return Nuevo estado con bloque reinsertado, o NULL si no es posible
	 */
	clpState* attemptReinsertion(clpState& s, const Block& block);
};

} /* namespace metasolver */

#endif /* REMOVEREINSERTOPERATOR_H_ */
