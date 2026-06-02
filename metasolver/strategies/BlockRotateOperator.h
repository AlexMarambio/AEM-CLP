/*
 * BlockRotateOperator.h
 *
 * Operador de vecindario: Rotación de Bloques
 *
 * Principio: Cambia la orientación de un bloque y lo recoloca
 * en su mejor posición compatible con la nueva orientación.
 * 
 * Nota: Solo aplicable a bloques con orientaciones variables.
 * Para bloques FSB, la rotación es más restrictiva.
 *
 * Complejidad: O(|blocks| × |spaces|) por vecino
 *
 *  Created on: 01-06-2026
 */

#ifndef BLOCKROTATEOPERATOR_H_
#define BLOCKROTATEOPERATOR_H_

#include "NeighborhoodOperator.h"
#include "../problems/clp/clpState.h"

using namespace std;
using namespace clp;

namespace metasolver {

/**
 * Operador BlockRotate para Container Loading Problem.
 * 
 * Rota un bloque colocado y lo reinenta en su nueva configuración.
 * 
 * Proceso:
 * 1. Selecciona un bloque colocado
 * 2. Remueve el bloque
 * 3. Aplica rotación (intercambia dimensiones)
 * 4. Reinenta colocar en mejor posición
 * 5. Acepta si mejora
 */
class BlockRotateOperator : public NeighborhoodOperator {
public:
	/**
	 * Tipos de rotación disponibles
	 */
	enum RotationType {
		ROTATE_LWH,  // (l, w, h) → (w, h, l)
		ROTATE_ALL,  // Intenta todas las 6 rotaciones posibles
		ROTATE_2D    // Solo rotaciones 2D (mantiene altura)
	};
	
	/**
	 * Constructor
	 * @param evaluator Evaluador para validar rotaciones
	 * @param rotation_type Tipo de rotación a aplicar
	 */
	BlockRotateOperator(ActionEvaluator* evaluator, RotationType rotation_type = ROTATE_ALL)
		: NeighborhoodOperator("BlockRotate"), 
		  evaluator(evaluator),
		  rotation_type(rotation_type),
		  max_rotation_attempts(5) {}
	
	virtual ~BlockRotateOperator() {}
	
	/**
	 * Genera un vecino rotando un bloque aleatorio.
	 * @param s Estado actual (clpState)
	 * @param s_best Mejor solución (no usado)
	 * @return Nuevo estado con bloque rotado y recolocado, o NULL
	 */
	virtual State* generateNeighbor(State& s, State* s_best = NULL);
	
	/**
	 * Genera múltiples vecinos rotando diferentes bloques.
	 * @param s Estado actual
	 * @param neighbors Lista de vecinos generados
	 * @param max_neighbors Si > 0, limita número de vecinos
	 */
	virtual void generateNeighborhood(State& s, list<State*>& neighbors, int max_neighbors = 0);
	
	/**
	 * Busca el mejor bloque a rotar.
	 * @param s Estado actual
	 * @param s_best Mejor solución conocida
	 * @param evaluator Evaluador
	 * @return Mejor vecino encontrado (mejora), o NULL
	 */
	virtual State* findBestNeighbor(State& s, State* s_best, ActionEvaluator* evaluator);
	
	/**
	 * Búsqueda exhaustiva: intenta rotar TODOS los bloques.
	 * @param s Estado actual (se modifica)
	 * @param s_best Mejor solución (se actualiza si mejora)
	 * @param evaluator Evaluador
	 * @return true si encontró mejora
	 */
	virtual bool exhaustiveSearch(State& s, State*& s_best, ActionEvaluator* evaluator);
	
	/**
	 * Establece máximo de intentos de rotación
	 */
	void setMaxRotationAttempts(int max_attempts) {
		max_rotation_attempts = max_attempts;
	}
	
	/**
	 * Establece tipo de rotación
	 */
	void setRotationType(RotationType rt) {
		rotation_type = rt;
	}
	
private:
	ActionEvaluator* evaluator;
	RotationType rotation_type;
	int max_rotation_attempts;
	
	/**
	 * Obtiene las posibles rotaciones de un bloque.
	 * Retorna lista de nuevas dimensiones (l, w, h) según rotation_type.
	 * @param original_dims Dimensiones originales
	 * @return Lista de rotaciones válidas como Vector3
	 */
	list<Vector3> getPossibleRotations(const Vector3& original_dims) const;
	
	/**
	 * Intenta rotar un bloque y recolocarlo óptimamente.
	 * @param s Estado donde realizar la rotación
	 * @param block_idx Índice del bloque a rotar en el contenedor
	 * @param new_dims Nuevas dimensiones tras rotación
	 * @return Nuevo estado con bloque rotado y recolocado, o NULL si falla
	 */
	clpState* attemptRotationAndReplacement(clpState& s, int block_idx, const Vector3& new_dims);
};

} /* namespace metasolver */

#endif /* BLOCKROTATEOPERATOR_H_ */
