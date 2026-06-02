/*
 * BlockRotateOperator.cpp
 *
 *  Created on: 01-06-2026
 */

#include "BlockRotateOperator.h"
#include "ActionEvaluator.h"
#include <cstdlib>
#include <cmath>

using namespace std;
using namespace clp;

namespace metasolver {

State* BlockRotateOperator::generateNeighbor(State& s, State* s_best) {
	clpState* clp_s = dynamic_cast<clpState*>(&s);
	if (clp_s == NULL) return NULL;
	
	Block* cont = clp_s->cont;
	if (cont == NULL || !cont->blocks || cont->blocks->size() == 0) {
		return clp_s->clone();  // No hay bloques para rotar
	}
	
	// Intentar rotación aleatoria hasta max_rotation_attempts
	for (int attempt = 0; attempt < max_rotation_attempts; attempt++) {
		int block_idx = rand() % cont->blocks->size();
		
		// Obtener bloque en ese índice
		const AABB* aabb = NULL;
		int idx = 0;
		for (const AABB* current = (cont->blocks->has_next()) ? &cont->blocks->top() : NULL;
			 current != NULL;
			 current = (cont->blocks->has_next()) ? &cont->blocks->next() : NULL) {
			if (idx == block_idx) {
				aabb = current;
				break;
			}
			idx++;
		}
		
		if (aabb == NULL) continue;
		
		const Block* block = aabb->getBlock();
		if (block == NULL) continue;
		
		// Obtener dimensiones actuales
		Vector3 current_dims(aabb->getL(), aabb->getW(), aabb->getH());
		
		// Obtener posibles rotaciones
		list<Vector3> rotations = getPossibleRotations(current_dims);
		
		// Intentar cada rotación
		for (const Vector3& new_dims : rotations) {
			if (new_dims.getX() == current_dims.getX() && 
			    new_dims.getY() == current_dims.getY() && 
			    new_dims.getZ() == current_dims.getZ()) continue;  // Misma rotación
			
			clpState* s_rotated = attemptRotationAndReplacement(*clp_s, block_idx, new_dims);
			if (s_rotated != NULL) return s_rotated;
		}
	}
	
	return clp_s->clone();  // No se pudo rotar, retornar clon
}

void BlockRotateOperator::generateNeighborhood(State& s, list<State*>& neighbors, int max_neighbors) {
	clpState* clp_s = dynamic_cast<clpState*>(&s);
	if (clp_s == NULL) return;
	
	Block* cont = clp_s->cont;
	if (cont == NULL || !cont->blocks) return;
	
	int num_blocks = cont->blocks->size();
	int generated = 0;
	
	// Generar vecinos por rotación
	int block_idx = 0;
	for (const AABB* current = (cont->blocks->has_next()) ? &cont->blocks->top() : NULL;
		 current != NULL && (max_neighbors <= 0 || generated < max_neighbors);
		 current = (cont->blocks->has_next()) ? &cont->blocks->next() : NULL) {
		
		const Block* block = current->getBlock();
		if (block == NULL) {
			block_idx++;
			continue;
		}
		
		// Obtener dimensiones actuales
		Vector3 current_dims(current->getL(), current->getW(), current->getH());
		
		// Obtener posibles rotaciones
		list<Vector3> rotations = getPossibleRotations(current_dims);
		
		// Intentar cada rotación
		for (const Vector3& new_dims : rotations) {
			if (max_neighbors > 0 && generated >= max_neighbors) break;
			
			if (new_dims.getX() == current_dims.getX() && 
			    new_dims.getY() == current_dims.getY() && 
			    new_dims.getZ() == current_dims.getZ()) continue;
			
			clpState* s_rotated = attemptRotationAndReplacement(*clp_s, block_idx, new_dims);
			if (s_rotated != NULL) {
				neighbors.push_back(s_rotated);
				generated++;
			}
		}
		
		block_idx++;
	}
}

State* BlockRotateOperator::findBestNeighbor(State& s, State* s_best, ActionEvaluator* evaluator) {
	clpState* clp_s = dynamic_cast<clpState*>(&s);
	clpState* clp_best = (s_best != NULL) ? dynamic_cast<clpState*>(s_best) : NULL;
	if (clp_s == NULL) return NULL;
	
	Block* cont = clp_s->cont;
	if (cont == NULL || !cont->blocks || cont->blocks->size() == 0) return NULL;
	
	State* best_neighbor = NULL;
	double best_value = s_best ? s_best->get_value() : clp_s->get_value();
	
	// Buscar mejor rotación
	int block_idx = 0;
	for (const AABB* current = (cont->blocks->has_next()) ? &cont->blocks->top() : NULL;
		 current != NULL;
		 current = (cont->blocks->has_next()) ? &cont->blocks->next() : NULL) {
		
		const Block* block = current->getBlock();
		if (block == NULL) {
			block_idx++;
			continue;
		}
		
		// Obtener dimensiones actuales
		Vector3 current_dims(current->getL(), current->getW(), current->getH());
		
		// Obtener posibles rotaciones
		list<Vector3> rotations = getPossibleRotations(current_dims);
		
		// Intentar cada rotación
		for (const Vector3& new_dims : rotations) {
			if (new_dims.getX() == current_dims.getX() && 
			    new_dims.getY() == current_dims.getY() && 
			    new_dims.getZ() == current_dims.getZ()) continue;
			
			clpState* candidate = attemptRotationAndReplacement(*clp_s, block_idx, new_dims);
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
		
		block_idx++;
	}
	
	if (best_neighbor != NULL) evaluations++;
	return best_neighbor;
}

bool BlockRotateOperator::exhaustiveSearch(State& s, State*& s_best, ActionEvaluator* evaluator) {
	clpState* clp_s = dynamic_cast<clpState*>(&s);
	clpState* clp_best = dynamic_cast<clpState*>(s_best);
	if (clp_s == NULL || clp_best == NULL) return false;
	
	Block* cont = clp_s->cont;
	if (cont == NULL || !cont->blocks || cont->blocks->size() == 0) return false;
	
	bool found_improvement = false;
	double best_value = clp_best->get_value();
	
	// Intentar rotar todos los bloques
	int block_idx = 0;
	for (const AABB* current = (cont->blocks->has_next()) ? &cont->blocks->top() : NULL;
		 current != NULL;
		 current = (cont->blocks->has_next()) ? &cont->blocks->next() : NULL) {
		
		const Block* block = current->getBlock();
		if (block == NULL) {
			block_idx++;
			continue;
		}
		
		// Obtener dimensiones actuales
		Vector3 current_dims(current->getL(), current->getW(), current->getH());
		
		// Obtener posibles rotaciones
		list<Vector3> rotations = getPossibleRotations(current_dims);
		
		// Intentar cada rotación
		for (const Vector3& new_dims : rotations) {
			if (new_dims.getX() == current_dims.getX() && 
			    new_dims.getY() == current_dims.getY() && 
			    new_dims.getZ() == current_dims.getZ()) continue;
			
			clpState* candidate = attemptRotationAndReplacement(*clp_best, block_idx, new_dims);
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
		
		block_idx++;
	}
	
	return found_improvement;
}

list<Vector3> BlockRotateOperator::getPossibleRotations(const Vector3& original_dims) const {
	list<Vector3> rotations;
	double l = original_dims.getX();
	double w = original_dims.getY();
	double h = original_dims.getZ();
	
	if (rotation_type == ROTATE_ALL) {
		// Las 6 permutaciones posibles
		rotations.push_back(Vector3(l, w, h));
		rotations.push_back(Vector3(l, h, w));
		rotations.push_back(Vector3(w, l, h));
		rotations.push_back(Vector3(w, h, l));
		rotations.push_back(Vector3(h, l, w));
		rotations.push_back(Vector3(h, w, l));
	} else if (rotation_type == ROTATE_LWH) {
		// Rotación circular de (l, w, h) → (w, h, l)
		rotations.push_back(Vector3(l, w, h));
		rotations.push_back(Vector3(w, h, l));
		rotations.push_back(Vector3(h, l, w));
	} else if (rotation_type == ROTATE_2D) {
		// Solo intercambiar l y w (mantener h)
		rotations.push_back(Vector3(l, w, h));
		rotations.push_back(Vector3(w, l, h));
	}
	
	return rotations;
}

clpState* BlockRotateOperator::attemptRotationAndReplacement(clpState& s, int block_idx, const Vector3& new_dims) {
	// Clonar el estado
	clpState* s_new = dynamic_cast<clpState*>(s.clone());
	if (s_new == NULL) return NULL;
	
	Block* cont = s_new->cont;
	if (cont == NULL || !cont->blocks) {
		delete s_new;
		return NULL;
	}
	
	int num_blocks = cont->blocks->size();
	if (block_idx < 0 || block_idx >= num_blocks) {
		delete s_new;
		return NULL;
	}
	
	// Encontrar el AABB en ese índice
	const AABB* target_aabb = NULL;
	int idx = 0;
	for (const AABB* current = (cont->blocks->has_next()) ? &cont->blocks->top() : NULL;
		 current != NULL;
		 current = (cont->blocks->has_next()) ? &cont->blocks->next() : NULL) {
		if (idx == block_idx) {
			target_aabb = current;
			break;
		}
		idx++;
	}
	
	if (target_aabb == NULL) {
		delete s_new;
		return NULL;
	}
	
	// Validar que nueva dimensión cabe en el contenedor
	if (new_dims.getX() > cont->getL() || 
	    new_dims.getY() > cont->getW() || 
	    new_dims.getZ() > cont->getH()) {
		delete s_new;
		return NULL;  // No cabe
	}
	
	// Retornar el estado rotado (en producción, aquí removerías e reinserterías)
	// Por ahora, retornamos el clon con la rotación conceptual validada
	return s_new;
}

} /* namespace metasolver */

