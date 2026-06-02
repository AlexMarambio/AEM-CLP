/*
 * NeighborhoodOperator.h
 *
 * Interfaz para operadores de movimiento en búsqueda local.
 * Cada operador define un vecindario específico que se explora
 * en la Variable Neighborhood Search (VNS).
 *
 *  Created on: 01-06-2026
 */

#ifndef NEIGHBORHOODOPERATOR_H_
#define NEIGHBORHOODOPERATOR_H_

#include "State.h"
#include <list>
#include <string>

using namespace std;

namespace metasolver {

/**
 * Interfaz para operadores de vecindario.
 * Un operador de vecindario define cómo generar movimientos
 * (vecinos) a partir de una solución actual.
 */
class NeighborhoodOperator {
public:
	NeighborhoodOperator(const string& name) : name(name), evaluations(0) {}
	
	virtual ~NeighborhoodOperator() {}
	
	/**
	 * Genera un vecino de la solución actual aplicando este operador.
	 * @param s Estado actual
	 * @param s_best Mejor solución encontrada (para referencia)
	 * @return Nuevo estado vecino, o NULL si no puede generarse
	 */
	virtual State* generateNeighbor(State& s, State* s_best = NULL) = 0;
	
	/**
	 * Genera múltiples vecinos de la solución actual.
	 * @param s Estado actual
	 * @param neighbors Lista donde se almacenan los vecinos generados
	 * @param max_neighbors Máximo número de vecinos a generar (0 = sin límite)
	 */
	virtual void generateNeighborhood(State& s, list<State*>& neighbors, int max_neighbors = 0) = 0;
	
	/**
	 * Busca el mejor vecino en el vecindario.
	 * @param s Estado actual
	 * @param s_best Mejor solución encontrada
	 * @param evaluator Evaluador de acciones
	 * @return Mejor vecino encontrado, o NULL si no hay mejora
	 */
	virtual State* findBestNeighbor(State& s, State* s_best, ActionEvaluator* evaluator) = 0;
	
	/**
	 * Realiza una búsqueda exhaustiva en el vecindario.
	 * @param s Estado actual
	 * @param s_best Mejor solución encontrada
	 * @param evaluator Evaluador de acciones
	 * @return true si se encontró mejora
	 */
	virtual bool exhaustiveSearch(State& s, State*& s_best, ActionEvaluator* evaluator) = 0;
	
	// Getters
	const string& getName() const { return name; }
	long getEvaluations() const { return evaluations; }
	void resetEvaluations() { evaluations = 0; }
	
protected:
	string name;
	long evaluations;  // Contador de evaluaciones realizadas
};

} /* namespace metasolver */

#endif /* NEIGHBORHOODOPERATOR_H_ */
