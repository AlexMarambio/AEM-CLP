/*
 * VNS.cpp
 *
 * Implementación de Variable Neighborhood Search
 *
 *  Created on: 01-06-2026
 */

#include "VNS.h"
#include "GlobalVariables.h"
#include "ActionEvaluator.h"
#include <cstdlib>
#include <cmath>
#include <iostream>

using namespace std;

namespace metasolver {

VNS::~VNS() {
	// Los operadores son responsabilidad del llamador
}

double VNS::run(State& s, double tl, clock_t bt) {
	timelimit = tl;
	begin_time = bt;
	
	State* s_current = s.clone();
	best_state = s_current->clone();
	
	cycles = 0;
	total_improvements = 0;
	resetPerturbationStrength();
	
	if (global::TRACE) {
		cerr << "VNS: Iniciando búsqueda..." << endl;
		cerr << "  Vecindarios: " << getNumberOfOperators() << endl;
		cerr << "  Tiempo límite: " << tl << " segundos" << endl;
	}
	
	// Ciclo principal de VNS
	while (get_time() < timelimit) {
		resetNeighborhoodIndex();
		
		// Variable Neighborhood Descent
		bool improved = variableNeighborhoodDescent(s_current, best_state);
		
		if (!improved) {
			// Si no mejoró, perturba para escapar óptimo local
			if (get_time() < timelimit * 0.9) {  // No perturbar en últimos 10% del tiempo
				perturb(s_current);
				increasePerturbationStrength();
			} else {
				break;  // Si estamos al final del tiempo, termina
			}
		} else {
			resetPerturbationStrength();
			total_improvements++;
		}
		
		cycles++;
		
		if (global::TRACE && cycles % 10 == 0) {
			cerr << "VNS ciclo " << cycles << ": valor=" << best_state->get_value() 
			     << " tiempo=" << get_time() << "s" << endl;
		}
	}
	
	if (global::TRACE) {
		cerr << "VNS finalizado:" << endl;
		cerr << "  Ciclos: " << cycles << endl;
		cerr << "  Mejoras: " << total_improvements << endl;
		cerr << "  Valor final: " << best_state->get_value() << endl;
		cerr << "  Tiempo total: " << get_time() << "s" << endl;
	}
	
	double result = best_state->get_value();
	
	// Limpieza
	if (s_current != best_state) delete s_current;
	
	return result;
}

bool VNS::variableNeighborhoodDescent(State*& s_current, State*& s_best) {
	bool found_improvement = false;
	int vnd_iter = 0;
	
	while (vnd_iter < max_vnd_iterations && get_time() < timelimit) {
		resetNeighborhoodIndex();
		bool improved_in_cycle = false;
		
		// Itera sobre todos los vecindarios
		while (getCurrentOperator() != NULL && get_time() < timelimit) {
			NeighborhoodOperator* op = getCurrentOperator();
			
			// Busca el mejor vecino en este vecindario
			State* s_neighbor = op->findBestNeighbor(*s_current, s_best, evl);
			
			if (s_neighbor != NULL) {
				// Se encontró mejora
				double val_neighbor = s_neighbor->get_value();
				double val_current = s_current->get_value();
				
				if (val_neighbor > val_current) {
					// Aceptar el vecino
					if (s_current != s_best) delete s_current;
					s_current = s_neighbor;
					
					// Actualizar mejor solución
					if (val_neighbor > s_best->get_value()) {
						if (s_best != s_current) delete s_best;
						s_best = s_current->clone();
						found_improvement = true;
					}
					
					// Reiniciar en primer vecindario
					improved_in_cycle = true;
					break;  // Vuelve a N1
				} else {
					delete s_neighbor;
				}
			}
			
			// Pasar al siguiente vecindario
			if (!nextNeighborhood()) {
				break;
			}
		}
		
		if (!improved_in_cycle) {
			break;  // No mejoró en ningún vecindario
		}
		
		vnd_iter++;
	}
	
	return found_improvement;
}

void VNS::perturb(State*& s) {
	if (getCurrentOperator() == NULL && getNumberOfOperators() > 0) {
		resetNeighborhoodIndex();
	}
	
	// Aplica perturbación usando operadores aleatorios
	int num_perturbations = max(1, (int)(perturbation_strength * getNumberOfOperators()));
	
	for (int i = 0; i < num_perturbations && get_time() < timelimit; i++) {
		int random_op = rand() % getNumberOfOperators();
		NeighborhoodOperator* op = getOperator(random_op);
		
		if (op != NULL) {
			State* s_perturbed = op->generateNeighbor(*s, NULL);
			if (s_perturbed != NULL) {
				if (s != NULL) delete s;
				s = s_perturbed;
			}
		}
	}
}

} /* namespace metasolver */
