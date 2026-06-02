/*
 * RemoveReinsertOperator.cpp
 *
 *  Created on: 01-06-2026
 */

#include "RemoveReinsertOperator.h"
#include "ActionEvaluator.h"
#include <cstdlib>
#include <algorithm>

using namespace std;
using namespace clp;

namespace metasolver {

State* RemoveReinsertOperator::generateNeighbor(State& s, State* s_best) {
	clpState* clp_s = dynamic_cast<clpState*>(&s);
	if (clp_s == NULL) return NULL;
	
	// Obtener todas las acciones posibles desde el estado actual
	list<Action*> actions;
	clp_s->get_actions(actions);
	
	if (actions.empty()) {
		// Si no hay acciones disponibles, retornar clon del estado
		return clp_s->clone();
	}
	
	// Seleccionar una acción aleatoria
	int random_idx = rand() % actions.size();
	auto it = actions.begin();
	advance(it, random_idx);
	Action* selected_action = *it;
	
	// Crear nuevo estado aplicando la acción
	clpState* s_new = dynamic_cast<clpState*>(clp_s->clone());
	if (s_new != NULL) {
		s_new->transition(*selected_action);
	}
	
	// Limpiar acciones
	for (auto& action : actions) {
		delete action;
	}
	
	return s_new;
}

void RemoveReinsertOperator::generateNeighborhood(State& s, list<State*>& neighbors, int max_neighbors) {
	clpState* clp_s = dynamic_cast<clpState*>(&s);
	if (clp_s == NULL) return;
	
	// Obtener todas las acciones posibles
	list<Action*> actions;
	clp_s->get_actions(actions);
	
	int generated = 0;
	for (auto& action : actions) {
		if (max_neighbors > 0 && generated >= max_neighbors) break;
		
		clpState* s_new = dynamic_cast<clpState*>(clp_s->clone());
		if (s_new != NULL) {
			s_new->transition(*action);
			neighbors.push_back(s_new);
			generated++;
		}
	}
	
	// Limpiar acciones
	for (auto& action : actions) {
		delete action;
	}
}

State* RemoveReinsertOperator::findBestNeighbor(State& s, State* s_best, ActionEvaluator* evaluator) {
	clpState* clp_s = dynamic_cast<clpState*>(&s);
	clpState* clp_best = (s_best != NULL) ? dynamic_cast<clpState*>(s_best) : NULL;
	
	if (clp_s == NULL) return NULL;
	
	// Obtener todas las acciones posibles
	list<Action*> actions;
	clp_s->get_actions(actions);
	
	if (actions.empty()) {
		for (auto& action : actions) delete action;
		return NULL;
	}
	
	State* best_neighbor = NULL;
	double best_value = (clp_best != NULL) ? clp_best->get_value() : clp_s->get_value();
	
	// Evaluar hasta max_reinsertion_attempts acciones aleatorias
	int attempts = min(max_reinsertion_attempts, (int)actions.size());
	for (int i = 0; i < attempts; i++) {
		int random_idx = rand() % actions.size();
		auto it = actions.begin();
		advance(it, random_idx);
		Action* action = *it;
		
		clpState* neighbor = dynamic_cast<clpState*>(clp_s->clone());
		if (neighbor != NULL) {
			neighbor->transition(*action);
			evaluations++;
			double neighbor_value = neighbor->get_value();
			
			if (neighbor_value > best_value) {
				if (best_neighbor != NULL) delete best_neighbor;
				best_neighbor = neighbor;
				best_value = neighbor_value;
			} else {
				delete neighbor;
			}
		}
	}
	
	// Limpiar acciones
	for (auto& action : actions) {
		delete action;
	}
	
	return best_neighbor;
}

bool RemoveReinsertOperator::exhaustiveSearch(State& s, State*& s_best, ActionEvaluator* evaluator) {
	clpState* clp_s = dynamic_cast<clpState*>(&s);
	if (clp_s == NULL) return false;
	
	// Obtener todas las acciones posibles
	list<Action*> actions;
	clp_s->get_actions(actions);
	
	bool found_improvement = false;
	double initial_best = s_best->get_value();
	
	// Probar cada acción
	for (auto& action : actions) {
		clpState* neighbor = dynamic_cast<clpState*>(clp_s->clone());
		if (neighbor != NULL) {
			neighbor->transition(*action);
			evaluations++;
			double neighbor_value = neighbor->get_value();
			
			if (neighbor_value > s_best->get_value()) {
				if (s_best != clp_s) delete s_best;
				s_best = neighbor;
				found_improvement = true;
			} else {
				delete neighbor;
			}
		}
	}
	
	// Limpiar acciones
	for (auto& action : actions) {
		delete action;
	}
	
	return found_improvement;
}

int RemoveReinsertOperator::getBlockIndexInContainer(const clpState& s, const Block& block) const {
	// TODO: Implementar búsqueda de índice de bloque
	return -1;
}

clpState* RemoveReinsertOperator::attemptReinsertion(clpState& s, const Block& block) {
	// TODO: Implementar lógica de reinserción
	return NULL;
}

} /* namespace metasolver */


