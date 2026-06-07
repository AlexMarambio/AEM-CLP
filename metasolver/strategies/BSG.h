/*
 * BSGpath.h
 *
 *  Created on: 12 jul. 2017
 *      Author: iaraya
 */

#ifndef STRATEGIES_BSG_H_
#define STRATEGIES_BSG_H_

#include "../SearchStrategy.h"
#include "MCTSGreedy.h"

namespace metasolver {



/**
 * Beam Stack Search sobre la interfaz clasica de BSG.
 *
 * La evaluacion de acciones sigue delegada en ActionEvaluator (VCS en CLP) y
 * la estimacion de una solucion completa sigue delegada en la estrategia greedy.
 * La diferencia con beam search clasico es que los candidatos podados por el
 * ancho de beam se guardan en una pila para retomarlos mediante backtracking.
 */
class BSG : public SearchStrategy {
public:
	/**
	 * Constructor
	 * @param greedy The underlying greedy algorithm
	 * @param expander
	 * @param beams the number of beams
	 * @p_elite the proportion of beams in the elite set (0.0, means 1 beam)
	 * @max_level_size the maximum number of expanded nodes by level of the tree
	 */
		BSG(ActionEvaluator* evl, SearchStrategy& greedy, int beams, double p_elite=0.0, int max_level_size=0, bool plot=false,
				const MCTSGreedy::Params& mcts_params=MCTSGreedy::Params()) :
			SearchStrategy(evl), greedy(greedy), beams(beams),
			max_level_size((max_level_size==0)? beams*beams:max_level_size),
			p_elite(p_elite), n_elite(max(1, (int)(p_elite*beams))), shuffle_best_path(false), plot(plot),
			mcts(NULL) {
				set_mcts_params(mcts_params);
			}


	virtual ~BSG();

    virtual void set_shuffle_best_path(bool b){
        shuffle_best_path=b;
    }

	/*
	 * Initialize the variables of the specific strategy
	 */
	virtual void initialize (State* s=NULL){
		clear_beam_stack();
		if(best_state) {delete best_state; best_state=NULL;}
	}

	/**
	 * Performs an iteration of the strategy
	 */
	virtual list<State*> next(list<State*>& S) ;


	virtual void clean(list<State*>& S){
		if(!plot)
			while(!S.empty()){ delete S.front(), S.pop_front(); }

	}

	/*
	 * Esta funcion "duplica" el esfuerzo de la estrategia.
	 * Si el esfuerzo es duplicado satisfactoriamente retorna true.
	 */
	virtual bool double_effort() {
	    int w= ((double)max_level_size/(double)beams) + 0.5;
		beams = double(beams)*sqrt(2) + 0.5;
		w=  double(w)*sqrt(2) +0.5;
		max_level_size = w*beams;
		n_elite = max(1, (int)(p_elite*beams));



		if(beams > 10000) return false;
		return true;
	}

		void set_beams(int b){beams=b;}

		void set_mcts_params(const MCTSGreedy::Params& params){
			if(mcts){
				delete mcts;
				mcts=NULL;
			}

			if(params.enabled && params.iterations>0)
				mcts = new MCTSGreedy(evl, greedy, params);
		}


	protected:

	struct Candidate {
		Candidate(double value, State* base_state, State* final_state) :
			value(value), base_state(base_state), final_state(final_state) { }

		double value;
		State* base_state;
		State* final_state;
	};

	struct BeamFrame {
		list<Candidate*> deferred;
	};

	typedef multimap<double, Candidate*> CandidateMap;

	State* promote_candidate(Candidate* candidate);
	list<State*> resume_from_beam_stack();
	void save_deferred_candidates(CandidateMap& ranked_candidates, CandidateMap::iterator first);
	void delete_candidate(Candidate* candidate);
	void clear_candidates(list<Candidate*>& candidates);
	void clear_beam_stack();

	list<BeamFrame> beam_stack;

	SearchStrategy& greedy;

	/**
	 * Number of beams
	 */
	int beams;

	/*
	 * Max number of expanded nodes in a level of the tree
	 */
	int max_level_size;

	double p_elite;
	int n_elite;

		bool shuffle_best_path;

		bool plot;

		MCTSGreedy* mcts;

	};

} /* namespace clp */

#endif /* STRATEGIES_BSG_H_ */
