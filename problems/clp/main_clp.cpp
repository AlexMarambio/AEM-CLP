/*
 * main_clp.cpp
 *
 *  Created on: 29 jun. 2017
 *      Author: iaraya
 */

#include <iostream>
#include <fstream>
#include "args.hxx"
//#include "objects/State.cpp"
#include "clpState.h"
#include "VCS_Function.h"
#include "VCS_Function.h"
#include "SpaceSet.h"
#include "Greedy.h"
#include "DoubleEffort.h"
#include "GlobalVariables.h"
#include "BSG.h"
#include "../metasolver/SolverConfig.h"
#include "../metasolver/strategies/BSG_VCS_MCTS.h"

bool global::TRACE = false;

using namespace std;

// para ejecutar (menos de 30 tipos de caja): BSG_CLP problems/clp/benchs/BR/BR7.txt 1 1.0 30 4.0 1.0 0.2 0.04 1.0 0.0 0.0 0 0
// para ejecutar (mas de 30 tipos de caja): BSG_CLP problems/clp/benchs/BR/BR8.txt 1 0.98 30 4.0 1.0 0.2 0.04 1.0 0.0 0.0 0 0




int main(int argc, char** argv){


	args::ArgumentParser parser("********* BSG-CLP *********.", "BSG Solver for CLP.");
	args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
	args::ValueFlag<int> _inst(parser, "int", "Instance", {'i'});
	args::ValueFlag<string> _format(parser, "string", "Format: (BR, BRw, 1C)", {'f'});
	args::ValueFlag<double> _min_fr(parser, "double", "Minimum volume occupied by a block (proportion)", {"min_fr"});
	args::ValueFlag<int> _maxtime(parser, "int", "Timelimit", {'t', "timelimit"});
	args::ValueFlag<int> _seed(parser, "int", "Random seed", {"seed"});
	args::ValueFlag<double> _alpha(parser, "double", "Alpha parameter", {"alpha"});
	args::ValueFlag<double> _beta(parser, "double", "Beta parameter", {"beta"});
	args::ValueFlag<double> _gamma(parser, "double", "Gamma parameter", {"gamma"});
	args::ValueFlag<double> _delta(parser, "double", "Delta parameter", {"delta"});
	args::ValueFlag<double> _p(parser, "double", "p parameter", {'p'});
	args::ValueFlag<string> _mode(parser, "string", "Solver mode: bsg_vcs, bsg_vcs_mcts, compare", {"mode"});
	args::ValueFlag<int> _beams(parser, "int", "Beam width", {"beams"});
	args::ValueFlag<int> _max_level_size(parser, "int", "Max number of expanded nodes per level", {"max_level_size"});
	args::ValueFlag<int> _top_k_vcs(parser, "int", "Top K candidates selected by VCS", {"top_k_vcs"});
	args::ValueFlag<int> _mcts_iterations(parser, "int", "MCTS iterations", {"mcts_iterations"});
	args::ValueFlag<int> _mcts_depth(parser, "int", "MCTS depth limit", {"mcts_depth_limit"});
	args::ValueFlag<double> _mcts_time(parser, "double", "MCTS time limit seconds", {"mcts_time_limit_seconds"});
	args::ValueFlag<double> _exploration(parser, "double", "MCTS exploration constant", {"exploration_constant"});
	args::ValueFlag<double> _vcs_rollout(parser, "double", "VCS rollout bias", {"vcs_rollout_bias"});
	args::ValueFlag<double> _reward_fill(parser, "double", "Reward weight fill rate", {"reward_fill_rate"});
	args::ValueFlag<double> _reward_compact(parser, "double", "Reward weight compactness", {"reward_compactness"});
	args::ValueFlag<double> _reward_contact(parser, "double", "Reward weight contact surface", {"reward_contact_surface"});
	args::ValueFlag<double> _reward_fragmentation(parser, "double", "Reward weight fragmentation", {"reward_fragmentation"});
	args::Flag _json(parser, "double", "json output tuple: (loaded, remaining, utilization)", {"json"});
	args::Flag _verbose(parser, "layout", "Show the actions to reach the solution", {"verbose"});
	args::ValueFlag<int> _verbose2(parser, "layout", "Show the actions to reach the solution (v2). Should be indicated the number of actions per state", {"verbose2"});

	args::Flag fsb(parser, "fsb", "full-support blocks", {"fsb"});
	args::Flag trace(parser, "trace", "Trace", {"trace"});
	args::Positional<std::string> _file(parser, "instance-set", "The name of the instance set");

	cout.precision(8);
	try
	{
		parser.ParseCLI(argc, argv);

	}
	catch (args::Help&)
	{
		std::cout << parser;
		return 0;
	}
	catch (args::ParseError& e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return 1;
	}
	catch (args::ValidationError& e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return 1;
	}

	string file=_file.Get();
	int inst=(_inst)? _inst.Get():0;
	double min_fr=(_min_fr)? _min_fr.Get():0.98;
	int maxtime=(_maxtime)? _maxtime.Get():100;

	double alpha=4.0, beta=1.0, gamma=0.2, delta=1.0, p=0.04, maxtheta=0.0;
	if(_maxtime) maxtime=_maxtime.Get();
	if(_alpha) alpha=_alpha.Get();
	if(_beta) beta=_beta.Get();
	if(_gamma) gamma=_gamma.Get();
	if(_delta) delta=_delta.Get();
	if(_p) p=_p.Get();


	string format="BR";
	if(_format) format=_format.Get();

	clpState::Format f;
	if(format=="BR")
		f=clpState::BR;
	else if(format=="BRw")
		f=clpState::BRw;
	else if(format=="1C")
		f=clpState::_1C;

	int seed=(_seed)? _seed.Get():1;
	srand(seed);

	global::TRACE = trace;

SolverConfig config;
string mode = (_mode)? _mode.Get(): "bsg_vcs";
if(_beams) config.beam_width = _beams.Get();
if(_max_level_size) config.max_level_size = _max_level_size.Get();
if(_top_k_vcs) config.top_k_vcs = _top_k_vcs.Get();
if(_mcts_iterations) config.mcts_iterations = _mcts_iterations.Get();
if(_mcts_depth) config.mcts_depth_limit = _mcts_depth.Get();
if(_mcts_time) config.mcts_time_limit_seconds = _mcts_time.Get();
if(_exploration) config.exploration_constant = _exploration.Get();
if(_vcs_rollout) config.vcs_rollout_bias = _vcs_rollout.Get();
if(_reward_fill) config.reward_weight_fill_rate = _reward_fill.Get();
if(_reward_compact) config.reward_weight_compactness = _reward_compact.Get();
if(_reward_contact) config.reward_weight_contact_surface = _reward_contact.Get();
if(_reward_fragmentation) config.reward_weight_fragmentation = _reward_fragmentation.Get();

	cout << "File("<< format <<"): " << file << endl;
	cout << "Instance:" << inst+1 << endl;
	cout << "min_fr:" << min_fr << endl;
	cout << "Maxtime:" << maxtime << endl;

	double r=0.0; //0.0
    //bool kdtree= false;

    Block::FSB=fsb;
    clpState* s0 = NULL;
    try {
      s0 = new_state(file,inst, min_fr, 10000, f);
    } catch (const exception& e) {
      cerr << "Error cargando instancia: " << e.what() << endl;
      return 1;
    }

    cout << "n_blocks:"<< s0->get_n_valid_blocks() << endl;

    clock_t begin_time=clock();

    VCS_Function* vcs = new VCS_Function(s0->nb_left_boxes, *s0->cont,
    alpha, beta, gamma, p, delta, 0.0, r);

	//for(int i=0;i<10000; i++)
	//	exp->best_action(*s0);

	cout << "greedy" << endl;
    SearchStrategy *gr = new Greedy (vcs);

	cout << "bsg" << endl;
    BSG *bsg= new BSG(vcs,*gr, config.beam_width, 0.0, config.max_level_size);

	cout << "bsg_vcs_mcts" << endl;
    BSG_VCS_MCTS *bsg_mcts = new BSG_VCS_MCTS(vcs,*gr, config);

    struct SolverOutcome {
        string name;
        SearchStrategy* wrapper;
        State* state_clone;
        double fill_rate;
        double runtime;
        size_t nodes_explored;
        const State* best_state;
        MCTSResult mcts_report;
        bool has_mcts;
    };

    auto execute_solver = [&](SearchStrategy& base_solver, State* clone, const string& name)->SolverOutcome {
        SolverOutcome outcome;
        outcome.name = name;
        outcome.wrapper = new DoubleEffort(base_solver);
        outcome.state_clone = clone;
        clock_t solver_start = clock();
        outcome.fill_rate = outcome.wrapper->run(*clone, maxtime, solver_start);
        outcome.runtime = outcome.wrapper->get_time();
        outcome.best_state = outcome.wrapper->get_best_state();
        outcome.nodes_explored = 0;
        outcome.has_mcts = false;

        BSG* base_bsg = dynamic_cast<BSG*>(&base_solver);
        if(base_bsg) outcome.nodes_explored = base_bsg->get_nodes_explored();
        BSG_VCS_MCTS* base_mcts = dynamic_cast<BSG_VCS_MCTS*>(&base_solver);
        if(base_mcts){
            outcome.nodes_explored = base_mcts->get_nodes_explored();
            outcome.mcts_report = base_mcts->get_last_mcts_report();
            outcome.has_mcts = true;
        }
        return outcome;
    };

    SolverOutcome outcome;
    SolverOutcome outcome_compare;

    if(mode == "bsg_vcs"){
        outcome = execute_solver(*bsg, s0->clone(), "BSG_VCS");
        cout << "% volume utilization" << endl;
        cout << outcome.fill_rate * 100 << endl;
        cout << "blocks placed: " << (outcome.best_state ? outcome.best_state->get_path().size() : 0) << endl;
        cout << "nodes explored: " << outcome.nodes_explored << endl;
        cout << "runtime: " << outcome.runtime << " sec" << endl;
    } else if(mode == "bsg_vcs_mcts"){
        outcome = execute_solver(*bsg_mcts, s0->clone(), "BSG_VCS_MCTS");
        cout << "% volume utilization" << endl;
        cout << outcome.fill_rate * 100 << endl;
        cout << "blocks placed: " << (outcome.best_state ? outcome.best_state->get_path().size() : 0) << endl;
        cout << "nodes explored: " << outcome.nodes_explored << endl;
        cout << "runtime: " << outcome.runtime << " sec" << endl;
        cout << "mcts iterations: " << outcome.mcts_report.iterations << endl;
        cout << "mcts avg depth: " << outcome.mcts_report.average_depth << endl;
        cout << "mcts max depth: " << outcome.mcts_report.max_depth << endl;
    } else if(mode == "compare"){
        SolverOutcome bsg_out = execute_solver(*bsg, s0->clone(), "BSG_VCS");
        SolverOutcome mcts_out = execute_solver(*bsg_mcts, s0->clone(), "BSG_VCS_MCTS");

        cout << "--------------------------------------------------" << endl;
        cout << "Algoritmo      Fill %     Tiempo(s)   Nodos" << endl;
        cout << "--------------------------------------------------" << endl;
        cout << bsg_out.name << "      " << bsg_out.fill_rate*100 << "      " << bsg_out.runtime << "      " << bsg_out.nodes_explored << endl;
        cout << mcts_out.name << "      " << mcts_out.fill_rate*100 << "      " << mcts_out.runtime << "      " << mcts_out.nodes_explored << endl;
        cout << "--------------------------------------------------" << endl;
        double fill_improvement = bsg_out.fill_rate > 0.0 ? 100.0 * (mcts_out.fill_rate - bsg_out.fill_rate) / bsg_out.fill_rate : 0.0;
        double runtime_ratio = bsg_out.runtime > 0.0 ? mcts_out.runtime / bsg_out.runtime : 0.0;
        cout << "Improvement Fill %: " << fill_improvement << "" << endl;
        cout << "Runtime Ratio: " << runtime_ratio << endl;
        outcome = mcts_out;
        outcome_compare = bsg_out;
    } else {
        cout << "Unknown mode: " << mode << ". Use bsg_vcs, bsg_vcs_mcts or compare." << endl;
        return 1;
    }

    SearchStrategy* de = outcome.wrapper;
    State& s_copy = *outcome.state_clone;

    if(mode == "compare"){
        // If compare, we keep the last run state for verbose, using the MCTS outcome.
        de = outcome.wrapper;
    }
	// << " " << de->get_best_state()->get_value2() << " " << eval*de->get_best_state()->get_value2() << endl;


  if(_verbose || _verbose2){
	list<const Action*>& actions= dynamic_cast<const clpState*>(de->get_best_state())->get_path();
	
	clpState* s00 = dynamic_cast<clpState*> (s0->clone());
	for (const Block* block:s00->valid_blocks){
		cout << "block: " << block->id << " " << *block << endl;
		for (auto pair:block->nb_boxes){
			cout << " -- box: " << *pair.first << " n: " << pair.second << endl;
		}
	}

	cout << "Solve steps: " << endl;

	for(auto action:actions){
		const clpAction* clp_action = dynamic_cast<const clpAction*> (action);

		list< Action* > best_actions;
		if (_verbose2){
			gr->get_best_actions(*s00, best_actions, _verbose2.Get());
			best_actions.push_back(new clpAction(*clp_action));
		}

		s00->transition(*clp_action);
		cout << "selected block:" << clp_action->block.id << " space:" << clp_action->space.get_location(clp_action->block) << endl;

		set<int> visited;
		if(_verbose2){
			for (auto act:best_actions){
				const clpAction* clp_act = dynamic_cast<const clpAction*> (act);
				if (visited.find(clp_act->block.id) != visited.end()) continue;
				visited.insert(clp_act->block.id);

				std::cout << "  action block:" << clp_act->block.id << " eval: ";
				for (auto it = clp_act->metrics.begin(); it != clp_act->metrics.end(); ++it) 
					std::cout << *it << " ";
				
				std::cout << std::endl;
			}
		}
		
	}


}


   if(_json){
	   	bool first;
		cout << "{\"remaining\" :["; first=true;
		for(auto b:dynamic_cast<const clpState*>(de->get_best_state())->nb_left_boxes)
		    if(b.second > 0){
			   if(first)  first=false; else cout << "," ;
			   cout << "[" << b.first->get_id() << "," << b.second << "]";
			}
		cout << "], \"loaded\" :["; first=true;
		for(auto b:dynamic_cast<const clpState*>(de->get_best_state())->nb_left_boxes){
			int load = s0->nb_left_boxes[b.first] -b.second;
			if(load>0){
			   if(first)  first=false; else cout << "," ;
			   cout << "[" <<  b.first->get_id() << "," << load << "]";
			}
		}
cout << "], \"utilization\" : " << dynamic_cast<const clpState*>(de->get_best_state())->get_value() << "}"<<endl;
	}


}
