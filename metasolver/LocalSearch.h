/*
 * LocalSearch.h
 *
 * Clase base para métodos de búsqueda local.
 * Define la estructura común de algoritmos que mejoran una solución
 * existente mediante búsqueda en vecindarios.
 *
 *  Created on: 01-06-2026
 */

#ifndef LOCALSEARCH_H_
#define LOCALSEARCH_H_

#include "SearchStrategy.h"
#include "NeighborhoodOperator.h"
#include <vector>

using namespace std;

namespace metasolver {

/**
 * Clase base para estrategias de búsqueda local.
 * Define operadores de vecindario y proporciona funcionalidad
 * común para diferentes tipos de búsqueda local.
 */
class LocalSearch : public SearchStrategy {
public:
	/**
	 * Constructor
	 * @param evl Evaluador de acciones
	 * @param name Nombre de la estrategia
	 */
	LocalSearch(ActionEvaluator* evl, const string& name = "LocalSearch") 
		: SearchStrategy(evl), name(name), current_neighborhood(0) {}
	
	virtual ~LocalSearch();
	
	/**
	 * Agrega un operador de vecindario a la lista de operadores.
	 * El orden de inserción es importante para VNS.
	 * @param op Operador a agregar
	 */
	void addNeighborhoodOperator(NeighborhoodOperator* op) {
		operators.push_back(op);
	}
	
	/**
	 * Obtiene el número de operadores registrados
	 */
	int getNumberOfOperators() const {
		return operators.size();
	}
	
	/**
	 * Obtiene un operador específico por índice
	 */
	NeighborhoodOperator* getOperator(int index) const {
		if (index >= 0 && index < (int)operators.size())
			return operators[index];
		return NULL;
	}
	
	/**
	 * Obtiene el índice del operador actual
	 */
	int getCurrentNeighborhoodIndex() const {
		return current_neighborhood;
	}
	
	/**
	 * Cambia al siguiente operador de vecindario
	 * @return true si avanzó a un operador nuevo, false si llegó al final
	 */
	bool nextNeighborhood() {
		if (current_neighborhood < (int)operators.size() - 1) {
			current_neighborhood++;
			return true;
		}
		return false;
	}
	
	/**
	 * Reinicia al primer operador de vecindario
	 */
	void resetNeighborhoodIndex() {
		current_neighborhood = 0;
	}
	
	/**
	 * Obtiene el operador actual
	 */
	NeighborhoodOperator* getCurrentOperator() const {
		return getOperator(current_neighborhood);
	}
	
	// Getters
	const string& getName() const { return name; }
	
protected:
	string name;
	vector<NeighborhoodOperator*> operators;  // Lista de operadores de vecindario
	int current_neighborhood;  // Índice del operador actual
};

} /* namespace metasolver */

#endif /* LOCALSEARCH_H_ */
