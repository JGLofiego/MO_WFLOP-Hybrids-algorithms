#include "../../headers/global_modules/local_search/ils_decomposition.h"
#include "../../headers/global_modules/generate_initial_population/population.h"
#include "../../headers/global_modules/generate_initial_population/generate_rSolution.h"
#include "../../headers/global_modules/genetic_operators/mutation.h"
#include "../../headers/global_modules/local_search/neighborhood.h"
#include "../../headers/metaheuristics/moead/modules/tchebycheff.h"
#include "../../headers/metaheuristics/moead/modules/generate_weight_vectors.h"
#include "../../headers/metaheuristics/moead/modules/generate_neighborhood.h"
#include "../../headers/metaheuristics/moead/modules/get_best_z_point.h"
#include "../../headers/metaheuristics/comolsd/modules/calculate_ipbi.h"
#include "../../headers/global_modules/local_search/ils.h"
#include "../../headers/globals.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <random>
#include <cmath>

using namespace std;

extern string perturbation_type_str;
extern int countRevalue;
extern string ils_variant_str;

// Enum for ILS/D variant
enum class ILSDVariant {
    CILS,
    IILS
};

// Enum for aggregation type
enum class AggregationType {
    Chebyshev,
    IPBI
};

// Compute scalar value for a solution
double scalar_value(const pair<double, double>& fitness, 
                    const pair<double, double>& lambda,
                    const pair<double, double>& z_point,
                    AggregationType agg_type) {
    if (agg_type == AggregationType::IPBI) {
        return calculate_ipbi(fitness, lambda, z_point);
    }
    pair<double, double> f = fitness;
    pair<double, double> l = lambda;
    pair<double, double> z = z_point;
    return calculate_gte(f, l, z);
}

// Single-objective local search for a subproblem
Solution solve_single_objective_ls(
    Solution current,
    const pair<double, double>& lambda,
    const pair<double, double>& z_point,
    int num_neighbors,
    bool improved_neighbor,
    AggregationType agg_type
) {
    Solution best = current;
    double best_val = scalar_value(best.fitness, lambda, z_point, agg_type);
    
    vector<Solution*> neighbors = getNeighborhoodStd(&current, num_neighbors);
    
    for (Solution* neighbor : neighbors) {
        double neighbor_val = scalar_value(neighbor->fitness, lambda, z_point, agg_type);
        if (neighbor_val < best_val) {
            best = *neighbor;
            best_val = neighbor_val;
        }
    }
    
    for (Solution* s : neighbors) delete s;
    
    return best;
}

// Perturb a solution using mutation
Solution perturb_solution(Solution s, PerturbationType type, int& consecutive_no_improvement) {
    int k;
    switch(type) {
        case PerturbationType::FixedWeak:
            k = 5;
            break;
        case PerturbationType::FixedModerate:
            k = 25;
            break;
        case PerturbationType::Random:
            k = 1 + rand() % 25;
            break;
        case PerturbationType::Adaptive:
            k = min(1 + consecutive_no_improvement, 30);
            break;
        default:
            k = 1;
    }
    mutationN(s, k);
    return s;
}

// TODO Fix the implementation, remove the while loop made for testing

// Local search wrapper — matches the local search interface
vector<Solution*>* ils_decomposition_local_search(vector<Solution*> population, bool improvedNeighbor){
    // Convert vector<Solution*> to vector<Solution>
    vector<Solution> pop;
    for(Solution* s : population){
        pop.push_back(*s);
    }

    ILSDVariant variant = (ils_variant_str == "IILS") ? ILSDVariant::IILS : ILSDVariant::CILS;
    PerturbationType pert_type = parsePerturbationType(perturbation_type_str);
    AggregationType agg_type = AggregationType::Chebyshev;

    int num_subproblems = SIZE_OF_POPULATION;

    // Step 1: Generate weight vectors
    vector<pair<double, double>> weight_vectors = build_weight_vector(num_subproblems);
    
    // Step 2: Initialize population (one solution per subproblem)
    while (pop.size() < num_subproblems) {
        Solution s;
        s = generate_solution();
        pop.push_back(s);
    }
    
    // Step 3: Initialize z_point (ideal reference point)
    pair<double, double> z_point = get_best_z_point(pop);
    
    // Step 4: Build subproblem neighborhoods (for CILS variant)
    vector<vector<int>> neighborhoods;
    if (variant == ILSDVariant::CILS) {
        neighborhoods = build_neighborhood(num_neighbors, weight_vectors, num_subproblems);
    }
    
    // Step 5: Initial local search phase — run LS on each subproblem
    for (int i = 0; i < num_subproblems; i++) {
        pop[i] = solve_single_objective_ls(
            pop[i], weight_vectors[i], z_point,
            num_neighbors, improvedNeighbor, agg_type
        );
    }
    
    // Step 6: Main ILS/D loop
    int consecutive_no_improvement = 0;
            
    // Iterate subproblems in random order
    vector<int> order(num_subproblems);
    for (int i = 0; i < num_subproblems; i++) order[i] = i;
    static mt19937 rng(random_device{}());
    shuffle(order.begin(), order.end(), rng);
    
    for (int idx : order) {
        if (variant == ILSDVariant::IILS) {
            // Independent: perturb own solution, LS w.r.t. own subproblem
            consecutive_no_improvement = 0;
            Solution current = pop[idx];
            
            // LS phase
            Solution local_opt = solve_single_objective_ls(
                current, weight_vectors[idx], z_point,
                num_neighbors, improvedNeighbor, agg_type
            );
            
            // Perturb
            Solution perturbed = perturb_solution(local_opt, pert_type, consecutive_no_improvement);
            
            // LS again from perturbed
            Solution new_opt = solve_single_objective_ls(
                perturbed, weight_vectors[idx], z_point,
                num_neighbors, improvedNeighbor, agg_type
            );
            
            double new_val = scalar_value(new_opt.fitness, weight_vectors[idx], z_point, agg_type);
            double current_val = scalar_value(pop[idx].fitness, weight_vectors[idx], z_point, agg_type);
            
            if (new_val < current_val) {
                pop[idx] = new_opt;
                consecutive_no_improvement = 0;
            } else {
                consecutive_no_improvement++;
            }
            
            // Update z_point
            for (const auto& sol : pop) {
                z_point.first = max(z_point.first, sol.fitness.first);
                z_point.second = max(z_point.second, sol.fitness.second);
            }
        } else {
            // CILS: pick random neighbor j, perturb x^j, LS w.r.t. subproblem idx
            if (neighborhoods.empty() || neighborhoods[idx].empty()) continue;
            
            // Pick random neighbor
            int j = neighborhoods[idx][rand() % neighborhoods[idx].size()];
            consecutive_no_improvement = 0;
            
            Solution current_j = pop[j];
            
            // LS phase from x^j w.r.t. subproblem idx
            Solution local_opt = solve_single_objective_ls(
                current_j, weight_vectors[idx], z_point,
                num_neighbors, improvedNeighbor, agg_type
            );
            
            // Perturb
            Solution perturbed = perturb_solution(local_opt, pert_type, consecutive_no_improvement);
            
            // LS again from perturbed w.r.t. subproblem idx
            Solution new_opt = solve_single_objective_ls(
                perturbed, weight_vectors[idx], z_point,
                num_neighbors, improvedNeighbor, agg_type
            );
            
            double new_val = scalar_value(new_opt.fitness, weight_vectors[idx], z_point, agg_type);
            double current_val = scalar_value(pop[idx].fitness, weight_vectors[idx], z_point, agg_type);
            
            if (new_val < current_val) {
                pop[idx] = new_opt;
                consecutive_no_improvement = 0;
            } else {
                consecutive_no_improvement++;
            }
            
            // Cooperative replacement: try to improve neighbors of idx
            for (int k : neighborhoods[idx]) {
                double k_val = scalar_value(pop[k].fitness, weight_vectors[k], z_point, agg_type);
                double new_k_val = scalar_value(new_opt.fitness, weight_vectors[k], z_point, agg_type);
                if (new_k_val < k_val) {
                    pop[k] = new_opt;
                }
            }
            
            // Update z_point
            for (const auto& sol : pop) {
                z_point.first = max(z_point.first, sol.fitness.first);
                z_point.second = max(z_point.second, sol.fitness.second);
            }
        }
    }
    
    // Convert back to vector<Solution*>
    vector<Solution*>* result = new vector<Solution*>();
    for(Solution& s : pop){
        result->push_back(new Solution(s));
    }

    return result;
}
