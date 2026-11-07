#include "../../headers/global_modules/local_search/ils.h"
#include "../../headers/global_modules/generate_initial_population/population.h"
#include "../../headers/global_modules/generate_initial_population/generate_rSolution.h"
#include "../../headers/global_modules/genetic_operators/mutation.h"
#include "../../headers/global_modules/local_search/pareto_ls.h"
#include "../../headers/global_modules/local_search/anytime_pls.h"
#include "../../headers/global_modules/dominates.h"
#include "../../headers/global_modules/isEqual.h"
#include "../../headers/global_modules/local_search/neighborhood.h"
#include "../../headers/global_modules/local_search/ils.h"
#include "../../headers/globals.h"
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <utility>
#include <random>
#include <algorithm>
#include <string>

using namespace std;

// Perturbation helpers (from original ils.cpp)
PerturbationType parsePerturbationType(const string& typeStr){
    if(typeStr == "fixed_weak") return PerturbationType::FixedWeak;
    if(typeStr == "fixed_moderate") return PerturbationType::FixedModerate;
    if(typeStr == "random") return PerturbationType::Random;
    if(typeStr == "adaptive") return PerturbationType::Adaptive;
    return PerturbationType::Adaptive;
}

int getPerturbationStrength(PerturbationType type, int& consecutive_no_improvement){
    switch(type){
        case PerturbationType::FixedWeak:
            return 5;
        case PerturbationType::FixedModerate:
            return 500;
        case PerturbationType::Random:
            return 5 + rand() % 10;
        case PerturbationType::Adaptive:
            return min(5 + consecutive_no_improvement, 15);
        default:
            return 1;
    }
}

Solution perturb(Solution s, PerturbationType type, int& consecutive_no_improvement){
    int k = getPerturbationStrength(type, consecutive_no_improvement);
    mutationN(s, k);
    return s;
}

// TODO fix the implementation and remove the while made for testing

// Local search that implements the ILS logic
vector<Solution*> * ils_local_search_impl(vector<Solution>& pop, bool improvedNeighbor){
    ParetoSetLS * p = new ParetoSetLS();
    vector<Solution*> * newPop = new vector<Solution*>();

    for(Solution * s: pareto->getElementos()){
        pareto->adicionarSol(s);
    }

    PerturbationType perturbationType = parsePerturbationType(perturbation_type_str);

    Solution * current_solution = new Solution();
    Solution * perturbed = new Solution();
    Solution * local_optimum;
    Solution * local_optimum_perturbed;

    int consecutive_no_improvement = 0;

    bool has_improved = false;
    for(Solution solution: pop){
        has_improved = false;

        // Local search on each solution
        vector<Solution*> neighborhood;
        Solution * next = new Solution(solution);
        Solution * solToReturn = new Solution(solution);

        if(improvedNeighbor){
            neighborhood = getNeighborhoodImproved(next, num_neighbors);
        } else{
            neighborhood = getNeighborhoodStd(next, num_neighbors);
        }

        for(int i = 0; i < num_neighbors; i++){
            if(p->adicionarSol(neighborhood[i])){
                has_improved = true;
                *solToReturn = *neighborhood[i];
            }
        }

        for(Solution* s: neighborhood) delete s;
        delete next;

        local_optimum = solToReturn;

        *perturbed = perturb(*local_optimum, perturbationType, consecutive_no_improvement);

        // LS on perturbed
        neighborhood.clear();
        next = new Solution(*perturbed);
        solToReturn = new Solution(*perturbed);

        if(improvedNeighbor){
            neighborhood = getNeighborhoodImproved(next, num_neighbors);
        } else{
            neighborhood = getNeighborhoodStd(next, num_neighbors);
        }

        has_improved = false;
        for(int i = 0; i < num_neighbors; i++){
            if(p->adicionarSol(neighborhood[i])){
                has_improved = true;
                *solToReturn = *neighborhood[i];
            }
        }

        for(Solution* s: neighborhood) delete s;
        delete next;

        local_optimum_perturbed = solToReturn;
        has_improved = p->adicionarSol(local_optimum_perturbed);

        while(has_improved){
            has_improved = false;
            *perturbed = perturb(*local_optimum_perturbed, perturbationType, consecutive_no_improvement);

            neighborhood.clear();
            next = new Solution(*perturbed);
            solToReturn = new Solution(*perturbed);

            if(improvedNeighbor){
                neighborhood = getNeighborhoodImproved(next, num_neighbors);
            } else{
                neighborhood = getNeighborhoodStd(next, num_neighbors);
            }

            for(int i = 0; i < num_neighbors; i++){
                if(p->adicionarSol(neighborhood[i])){
                    has_improved = true;
                    *solToReturn = *neighborhood[i];
                }
            }

            for(Solution* s: neighborhood) delete s;
            delete next;

            local_optimum_perturbed = solToReturn;
            has_improved = p->adicionarSol(local_optimum_perturbed);
        }

        newPop->push_back(new Solution(*local_optimum_perturbed));
    }

        
    delete current_solution;
    delete perturbed;
    delete local_optimum;
    delete local_optimum_perturbed;
    delete p;

    return newPop;
}

// Local search wrapper — matches the local search interface
vector<Solution*>* ils_local_search(vector<Solution*> population, bool improvedNeighbor){
    // Convert vector<Solution*> to vector<Solution>
    vector<Solution> pop;
    for(Solution* s : population){
        pop.push_back(*s);
    }

    // Run the core ILS logic
    vector<Solution*>* result = ils_local_search_impl(pop, improvedNeighbor);

    return result;
}
