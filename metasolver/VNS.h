/*
 * VNS.h
 *
 * Variable Neighborhood Search (VNS).
 * 
 * Algoritmo de búsqueda local que explora sistemáticamente
 * múltiples vecindarios para escapar de óptimos locales.
 * 
 * Referencia: Mladenović & Hansen (1997)
 *
 *  Created on: 01-06-2026
 */

#ifndef VNS_H_
#define VNS_H_

#include "LocalSearch.h"
#include <map>

using namespace std;

namespace metasolver {

/**
 * Variante de Variable Neighborhood Search.
 * Implementa Variable Neighborhood Descent (VND) combinado con perturbación.
 * 
 * Flujo:
 * 1. Comienza con una solución inicial (ej: BSG)
 * 2. Aplica VND: explora todos los vecindarios secuencialmente
 * 3. Si mejora: reinicia en N1
 * 4. Si no mejora: cambia a siguiente vecindario
 * 5. Si exploró todos sin mejora: perturba y repite
 * 6. Continúa hasta timeout
 */
class VNS : public LocalSearch {
public:
	/**
	 * Constructor
	 * @param evl Evaluador de acciones
	 * @param max_perturbation_strength Fuerza máxima de perturbación (0.0-1.0)
	 * @param vnd_iterations Iteraciones máximas de VND por ciclo
	 */
	VNS(ActionEvaluator* evl, double max_perturbation_strength = 0.3, int vnd_iterations = 5)
		: LocalSearch(evl, "VNS"), 
		  max_perturbation(max_perturbation_strength),
		  max_vnd_iterations(vnd_iterations),
		  perturbation_strength(0.1),
		  cycles(0),
		  total_improvements(0),
		  no_improvement_count(0) {}
	
	virtual ~VNS();
	
	/**
	 * Ejecuta el algoritmo VNS.
	 * Itera sobre múltiples vecindarios hasta alcanzar timeout.
	 * @param s Estado inicial (solución completa)
	 * @param tl Tiempo límite en segundos
	 * @param bt Tiempo de comienzo
	 * @return Valor de la mejor solución encontrada
	 */
	virtual double run(State& s, double tl = 99999.9, clock_t bt = clock());
	
	/**
	 * Un ciclo completo de VNS:
	 * 1. Variable Neighborhood Descent (VND)
	 * 2. Si mejora, reinicia; si no, perturba
	 */
	bool vnsCycle(State*& s_current, State*& s_best);
	
	/**
	 * Variable Neighborhood Descent (VND).
	 * Explora secuencialmente todos los vecindarios.
	 * Se detiene al encontrar mejora o explorar todos.
	 * @return true si encontró mejora en algún vecindario
	 */
	bool variableNeighborhoodDescent(State*& s_current, State*& s_best);
	
	/**
	 * Perturbación: modifica la solución para escapar óptimos locales.
	 * Aplica movimientos aleatorios de forma controlada.
	 * @param s Solución a perturbar
	 */
	void perturb(State*& s);
	
	/**
	 * Aumenta gradualmente la fuerza de perturbación si no hay mejoras.
	 */
	void increasePerturbationStrength() {
		perturbation_strength = min(perturbation_strength * 1.2, max_perturbation);
		no_improvement_count++;
	}
	
	/**
	 * Reinicia la fuerza de perturbación tras encontrar mejora.
	 */
	void resetPerturbationStrength() {
		perturbation_strength = 0.1;
		no_improvement_count = 0;
	}
	
	// Getters para estadísticas
	long getCycles() const { return cycles; }
	long getImprovements() const { return total_improvements; }
	double getPerturbationStrength() const { return perturbation_strength; }
	
private:
	double max_perturbation;           // Fuerza máxima de perturbación
	int max_vnd_iterations;            // Máximo de iteraciones VND por ciclo
	double perturbation_strength;      // Fuerza actual de perturbación (adaptativa)
	long cycles;                       // Número de ciclos VNS completados
	long total_improvements;           // Total de mejoras encontradas
	int no_improvement_count;          // Contador de ciclos sin mejora
};

} /* namespace metasolver */

#endif /* VNS_H_ */
