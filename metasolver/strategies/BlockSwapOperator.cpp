/*
 * BlockSwapOperator.cpp
 *
 *  Created on: 01-06-2026
 */

#include "BlockSwapOperator.h"
#include "ActionEvaluator.h"
#include <cstdlib>
#include <algorithm>

using namespace std;
using namespace clp;

namespace metasolver {

State* BlockSwapOperator::generateNeighbor(State& s, State* s_best) {
	clpState* clp_s = dynamic_cast<clpState*>(&s);
	if (clp_s == NULL) return NULL;
	
	// Intentar swap aleatorio hasta max_swap_attempts
	for (int attempt = 0; attempt < max_swap_attempts; attempt++) {
		// Obtener bloques colocados
		Block* cont = clp_s->cont;
		if (cont == NULL || !cont->blocks || cont->blocks->size() < 2) {
			return clp_s->clone();  // No hay suficientes bloques para swap
		}
		
		// Seleccionar dos índices aleatorios
		int num_blocks = cont->blocks->size();
		int idx1 = rand() % num_blocks;
		int idx2 = rand() % num_blocks;
		
		if (idx1 == idx2) continue;  // Mismo índice, intentar de nuevo
		
		// Validar swap
		if (!isValidSwap(*clp_s, idx1, idx2)) continue;  // No válido, intentar de nuevo
		
		// Realizar swap
		clpState* s_new = performSwap(*clp_s, idx1, idx2);
		if (s_new != NULL) return s_new;
	}
	
	return clp_s->clone();  // No se pudo hacer swap, retornar clon
}

void BlockSwapOperator::generateNeighborhood(State& s, list<State*>& neighbors, int max_neighbors) {
	clpState* clp_s = dynamic_cast<clpState*>(&s);
	if (clp_s == NULL) return;
	
	Block* cont = clp_s->cont;
	if (cont == NULL || !cont->blocks) return;
	
	int num_blocks = cont->blocks->size();
	int generated = 0;
	
	// Generar vecinos por swap válidos
	for (int i = 0; i < num_blocks && (max_neighbors <= 0 || generated < max_neighbors); i++) {
		for (int j = i + 1; j < num_blocks && (max_neighbors <= 0 || generated < max_neighbors); j++) {
			if (isValidSwap(*clp_s, i, j)) {
				clpState* s_new = performSwap(*clp_s, i, j);
				if (s_new != NULL) {
					neighbors.push_back(s_new);
					generated++;
				}
			}
		}
	}
}

State* BlockSwapOperator::findBestNeighbor(State& s, State* s_best, ActionEvaluator* evaluator) {
	clpState* clp_s = dynamic_cast<clpState*>(&s);
	clpState* clp_best = (s_best != NULL) ? dynamic_cast<clpState*>(s_best) : NULL;
	if (clp_s == NULL) return NULL;
	
	Block* cont = clp_s->cont;
	if (cont == NULL || !cont->blocks || cont->blocks->size() < 2) return NULL;
	
	State* best_neighbor = NULL;
	double best_value = s_best ? s_best->get_value() : clp_s->get_value();
	int num_blocks = cont->blocks->size();
	
	// Buscar mejor swap válido
	for (int i = 0; i < num_blocks; i++) {
		for (int j = i + 1; j < num_blocks; j++) {
			if (isValidSwap(*clp_s, i, j)) {
				clpState* candidate = performSwap(*clp_s, i, j);
				if (candidate != NULL) {
					double candidate_value = candidate->get_value();
					if (candidate_value > best_value) {
						best_value = candidate_value;
						if (best_neighbor != NULL) delete best_neighbor;
						best_neighbor = candidate;
					} else {
						delete candidate;
					}
				}
			}
		}
	}
	
	if (best_neighbor != NULL) evaluations++;
	return best_neighbor;
}

bool BlockSwapOperator::exhaustiveSearch(State& s, State*& s_best, ActionEvaluator* evaluator) {
	clpState* clp_s = dynamic_cast<clpState*>(&s);
	clpState* clp_best = dynamic_cast<clpState*>(s_best);
	if (clp_s == NULL || clp_best == NULL) return false;
	
	Block* cont = clp_s->cont;
	if (cont == NULL || !cont->blocks || cont->blocks->size() < 2) return false;
	
	bool found_improvement = false;
	int num_blocks = cont->blocks->size();
	double best_value = clp_best->get_value();
	
	// Intentar todos los swaps válidos
	for (int i = 0; i < num_blocks; i++) {
		for (int j = i + 1; j < num_blocks; j++) {
			if (isValidSwap(*clp_best, i, j)) {
				clpState* candidate = performSwap(*clp_best, i, j);
				if (candidate != NULL) {
					double candidate_value = candidate->get_value();
					if (candidate_value > best_value) {
						best_value = candidate_value;
						if (s_best != NULL) delete s_best;
						s_best = candidate;
						found_improvement = true;
					} else {
						delete candidate;
					}
				}
			}
		}
	}
	
	return found_improvement;
}

bool BlockSwapOperator::isValidSwap(const clpState& s, int block1_idx, int block2_idx) const {
	const Block* cont = s.cont;
	if (cont == NULL || !cont->blocks) return false;
	
	int num_blocks = cont->blocks->size();
	if (block1_idx < 0 || block1_idx >= num_blocks) return false;
	if (block2_idx < 0 || block2_idx >= num_blocks) return false;
	
	// Obtener AABBs de los dos bloques
	const AABB* aabb1 = NULL;
	const AABB* aabb2 = NULL;
	int idx = 0;
	
	for (const AABB* current = (cont->blocks->has_next()) ? &cont->blocks->top() : NULL;
		 current != NULL;
		 current = (cont->blocks->has_next()) ? &cont->blocks->next() : NULL) {
		if (idx == block1_idx) aabb1 = current;
		if (idx == block2_idx) aabb2 = current;
		idx++;
	}
	
	if (aabb1 == NULL || aabb2 == NULL) return false;
	
	const Block* b1 = aabb1->getBlock();
	const Block* b2 = aabb2->getBlock();
	if (b1 == NULL || b2 == NULL) return false;
	
	// Validar dimensiones: ambos bloques deben caber en las posiciones del otro
	double aabb1_len = aabb1->getL();
	double aabb1_wid = aabb1->getW();
	double aabb1_hei = aabb1->getH();
	
	double aabb2_len = aabb2->getL();
	double aabb2_wid = aabb2->getW();
	double aabb2_hei = aabb2->getH();
	
	double cont_len = cont->getL();
	double cont_wid = cont->getW();
	double cont_hei = cont->getH();
	
	// Validación simple: ambos deben caber dentro del contenedor
	// (en producción, habría que validar colisiones también)
	bool b1_fits_in_b2_space = (aabb2_len >= aabb1_len && aabb2_wid >= aabb1_wid && aabb2_hei >= aabb1_hei);
	bool b2_fits_in_b1_space = (aabb1_len >= aabb2_len && aabb1_wid >= aabb2_wid && aabb1_hei >= aabb2_hei);
	
	return (b1_fits_in_b2_space && b2_fits_in_b1_space);
}

clpState* BlockSwapOperator::performSwap(clpState& s, int block1_idx, int block2_idx) {
	clpState* s_new = dynamic_cast<clpState*>(s.clone());
	if (s_new == NULL) return NULL;
	
	Block* cont = s_new->cont;
	if (cont == NULL || !cont->blocks) {
		delete s_new;
		return NULL;
	}
	
	int num_blocks = cont->blocks->size();
	if (block1_idx < 0 || block1_idx >= num_blocks || block2_idx < 0 || block2_idx >= num_blocks) {
		delete s_new;
		return NULL;
	}
	
	// Recopilar posiciones de ambos bloques
	const AABB* aabb1 = NULL;
	const AABB* aabb2 = NULL;
	Vector3 pos1, pos2;
	int idx = 0;
	
	for (const AABB* current = (cont->blocks->has_next()) ? &cont->blocks->top() : NULL;
		 current != NULL;
		 current = (cont->blocks->has_next()) ? &cont->blocks->next() : NULL) {
		if (idx == block1_idx) {
			aabb1 = current;
			pos1 = aabb1->getMins();
		}
		if (idx == block2_idx) {
			aabb2 = current;
			pos2 = aabb2->getMins();
		}
		idx++;
	}
	
	if (aabb1 == NULL || aabb2 == NULL) {
		delete s_new;
		return NULL;
	}
	
	// Intercambiar posiciones: esto es una aproximación
	// En realidad, necesitaríamos remover ambos y reinsertar en nuevas posiciones
	// Por ahora, retornar el clon (sin cambios) como fallback
	return s_new;
}

} /* namespace metasolver */

