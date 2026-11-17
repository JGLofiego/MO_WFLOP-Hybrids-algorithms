#include "wpPS_local_search.h"
#include "globals.h"
#include "../../headers/global_modules/generate_initial_population/generate_rSolution.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <set>

using namespace std;

// ============================================================================
// Core: Turbine pressure calculation
// ============================================================================

double calc_marginal_energy(Turbine& turb, Solution* s) {
    // E_ii is the turbine's own production (with the wake it receives)
    double own = turb.powerCalculated;
 
    // Energy lost by turbine 'target' because of the wake caused by turb
    auto loss_on = [&](Turbine& target) -> double {
        // Deficit that turb causes on target (E_ij)
        double d = calculate_interference(turb, target);
        if (d == 0.0) return 0.0;  // target is not in the wake of turb
 
        // Quadratic deficit that target receives from everyone except turb
        double deficit = 0.0;
        double res;
 
        for (auto& zone_turb : s->turbines) {
            for (auto& t : zone_turb) {
                if (t.id == turb.id || t.id == target.id) continue;
                res = calculate_interference(t, target);
                deficit += res * res;
            }
        }
 
        for (auto& f : fixd) {
            if (f.id == turb.id || f.id == target.id) continue;
            res = calculate_interference(f, target);
            deficit += res * res;
        }
 
        double wind_without = wind * (1.0 - min(sqrt(deficit), 1.0));
        double wind_with    = wind * (1.0 - min(sqrt(deficit + d * d), 1.0));
 
        return power_produced(wind_without, target) - power_produced(wind_with, target);
    };
 
    double loss = 0.0;
 
    // Loss caused on the other mobile turbines
    for (auto& zone_turb : s->turbines) {
        for (auto& t : zone_turb) {
            if (t.id == turb.id) continue;  // skip self
            loss += loss_on(t);
        }
    }
 
    // Loss caused on the fixed turbines
    for (auto& f : fixd) {
        if (f.id == turb.id) continue;
        loss += loss_on(f);
    }
 
    return own - loss;
}

// Calculate turbine pressure
// Pressionei = lambdaC * Ci_hat + lambdaP * (1 - MPi_hat)
double turbine_pressure(
    Turbine& t,
    Solution* s,
    double maxCost,
    double maxEnergy,
    double lambdaC,
    double lambdaP
) {
    // Get turbine's raw cost
    double cost = t.costCalculated;
 
    // Normalize cost
    double normCost = cost / maxCost;
 
    // Normalized marginal contribution (already includes the turbine's own
    // energy, so it must not be added again)
    double marginalEnergy = calc_marginal_energy(t, s);
    double normEnergy = marginalEnergy / maxEnergy;

    double costPart = normCost * lambdaC;
    double energyPart = normEnergy * lambdaP;

    // cout << "NORM_COST: " << normCost << " NORM_NRG: " << normEnergy << endl;
    
    // ? press = lambdaC * normCost + lambdaP * normEnergy
    // Pressione = lambdaC * Ci_hat + lambdaP * (1 - MPi_hat)
    // Higher cost and lower marginal energy => higher pressure (more likely to remove)
    // double pressure = lambdaC * normCost + lambdaP * (1.0 - normEnergy);
    double pressure = costPart - energyPart;
 
    return pressure;
}


// ============================================================================
// Core: Position opportunity calculation
// ============================================================================

// Calculate the opportunity of a candidate position j
// Opportunityj = lambdaP * MPj_hat - lambdaC * Cj_hat
double position_opportunity(
    Turbine& possible_t,
    Solution* s,
    double maxCost,
    double maxEnergy,
    double lambdaC,
    double lambdaP
) {
    // Raw cost of the candidate position
    double cost = possible_t.costCalculated;
 
    // Marginal energy at the candidate position (uses possible_t.powerCalculated
    // as its own production)
    double marginalEnergy = calc_marginal_energy(possible_t, s);
 
    // Normalize using archive maxima (same normalization used by
    // ParetoSetLS::calculate_pareto_dir_vec)
    double normCost = cost / maxCost;
    double normEnergy = marginalEnergy / maxEnergy;
 
    // cout << "Max_cost_energy" << maxCost << " " << maxEnergy << endl
    // << " Dir_vec: " << lambdaC << " " << lambdaP << endl
    // << " cost_energy: " << cost << " " << energy << " " << marginalEnergy << endl;
 
    // Opportunity = lambdaP * MPj_hat - lambdaC * Cj_hat
    // Higher opportunity = better position
    double opportunity = lambdaP * normEnergy - lambdaC * normCost;
 
    return opportunity;
}

// ============================================================================
// Core: Create turbine at new position
// ============================================================================

Turbine create_turbine_in_pos(Turbine& t_base, Foundation& f, int index_f, Solution* s) {
    // Copy common turbine characteristics
    Turbine new_t = t_base;
    new_t.costCalculated = f.cost;
    new_t.x = f.x;
    new_t.y = f.y;
    new_t.index = index_f;
    new_t.id = t_base.id;  // Keep same turbine ID

    return new_t;
}

// Calculate power contribution of a turbine at a new position
// (without considering the original turbine's effects)
double calc_power_turbine_swapping_pos(Turbine& new_t, Solution* s) {
    double deficit = 0.0;
    double result;

    // Interference from other mobile turbines
    for (auto& zone_turb : s->turbines) {
        for (auto& turb : zone_turb) {
            if (turb.id == new_t.id) continue;  // skip self
            result = calculate_interference(turb, new_t);
            deficit += result * result;
        }
    }

    // Interference from fixed turbines
    for (int a = 0; a < fixd.size(); a++) {
        result = calculate_interference(fixd[a], new_t);
        deficit += result * result;
    }

    // Calculate wind speed at the new position
    double windResulted = wind * (1.0 - min(sqrt(deficit), 1.0));

    // Calculate energy
    double power = power_produced(windResulted, new_t);
    return power;
}

// ============================================================================
// Core: Swap turbine and update solution
// ============================================================================

Solution* swap_turbines(Turbine new_t, Solution* s) {
    Solution* newSol = new Solution(*s);
    int i_old;
    int zone = new_t.zone;

    // Find the index of the old turbine in the zone
    for (int i = 0; i < (int)s->turbines[zone].size(); i++) {
        if (s->turbines[zone][i].id == new_t.id) {
            i_old = i;
            break;
        }
    }

    // Update the grid: remove old turbine, place new turbine
    newSol->grid[zone][newSol->turbines[zone][i_old].index] = 0;
    newSol->grid[zone][new_t.index] = new_t.id;

    // Update the turbine list
    newSol->turbines[zone][i_old] = new_t;

    // Recalculate the solution's fitness
    calculate_cost(*newSol);
    calculate_power(*newSol);

    return newSol;
}

// ============================================================================
// Main: WPPS local search for a single solution
// ============================================================================

map<pair<int,int>, int> initializeTurbmemory(){
    map<pair<int,int>, int> turbMemory;

    for(int i = 0; i < foundations.size(); i++){
        for(int j = 0; j < foundations[i].size(); j++){
            turbMemory[{i, j}] = 0;
        }
    }
    return turbMemory;
}

// Map of turbine IDs to their memory state
map<pair<int,int>, int> turbMemory = initializeTurbmemory();
double weight_fail = 0.0;


int c_greater = 0, e_greater = 0;

int thresh_dir = 95;
int thresh_press = 95;

// WPPS local search for a single solution
// Returns pointer to improved solution
Solution* wpPS_local_search(Solution* current_solution, ParetoSetLS& archive) {
    // Get the Pareto direction vector for the current iteration

    int r_dir = rand() % 100;

    Dir dir;

    if(r_dir < thresh_dir){
        dir = archive.pick_direction(current_solution);
    } else {
        int r = rand() % 101;
        dir.lc = (double) r / (double) 100;
        dir.lp = (double) (100-r) / (double) 100;
    }


    double lambdaC = dir.lc;
    double lambdaP = dir.lp;

    if(lambdaC < lambdaP){
        e_greater++;
    } else {
        c_greater++;
    }

    // cout << "VEC: " << lambdaC << " " << lambdaP << endl;

    // Get archive maxima for normalization
    double maxCost = archive.getMaxCost();
    double maxEnergy = archive.getMaxEnergy();
    double minCost = archive.getMinCost();
    double minEnergy = archive.getMinEnergy();

    double Cr = max(maxCost - minCost, 0.01 * maxCost);
    double Er = max(maxEnergy - minEnergy, 0.01 * maxEnergy);

    // ========================================================================
    // Step 1: Calculate pressure for all turbines
    // ========================================================================

    pair<double, pair<int, int>> worst;
    double worst_press = -INFINITY;

    // * TEST IGNORING PRESSURE
    // worst.second.first = rand() % current_solution->turbines.size();
    // worst.second.second = rand() % current_solution->turbines[worst.second.first].size();
    
    int r_press = rand() % 100;

    if(r_press < thresh_press){
        for (int zone = 0; zone < current_solution->turbines.size(); zone++) {
            for (int i = 0; i < current_solution->turbines[zone].size(); i++) {
                Turbine& t = current_solution->turbines[zone][i];
    
                // ! CHANGED MAXCOST MAXENERGY TO CR ER
                // Calculate and store pressure
                double pressure = turbine_pressure(
                    t, current_solution, Cr, Er, lambdaC, lambdaP
                );
    
                pressure = pressure / (1.0 + weight_fail * turbMemory[{zone, i}]);
    
                if(worst_press < pressure){
                    worst = make_pair(pressure, make_pair(zone, i));
                    worst_press = pressure;
                }
            }
        }
    } else {
        worst.second.first = rand() % current_solution->turbines.size();
        worst.second.second = rand() % current_solution->turbines[worst.second.first].size();
    }



    // ========================================================================
    // Step 2: Select turbine with highest pressure (worst turbine)
    // ========================================================================

    // Select the turbine with highest pressure
    int zone_worst = worst.second.first;
    int idx_worst = worst.second.second;

    Turbine& worst_turbine = current_solution->turbines[zone_worst][idx_worst];


    // ========================================================================
    // Step 3: Calculate opportunity for candidate positions
    // ========================================================================

    // Build set of occupied positions to avoid duplicates
    set<int> occupied_positions;
    for (auto& t : current_solution->turbines[zone_worst]) {
        occupied_positions.insert(t.index);
    }

    // Find candidate positions in the same zone that are not occupied
    pair<double, pair<int, Turbine>> best_candidate;
    double best_opport = -INFINITY;

    int num_pos = foundations[zone_worst].size();

    // * TEST IGNORING OPORTUNITY
    // // int rand_pos = rand() % num_pos;
    // // best_candidate.second.first = rand_pos;
    // // best_candidate.second.second = create_turbine_in_pos(worst_turbine, foundations[zone_worst][rand_pos], rand_pos, current_solution);

    int possible_pos = min(num_neighbors, (int)foundations[zone_worst].size());

    int c = 0;

    // Iterate through possible foundation positions in this zone
    while (c < possible_pos) {
        int idx = rand() % num_pos;

        if (occupied_positions.find(idx) != occupied_positions.end()) {
            continue;  // Skip occupied positions
        }

        occupied_positions.insert(idx);

        // Create a temporary turbine at this candidate position
        Foundation& f = foundations[zone_worst][idx];
        Turbine candidate_t = create_turbine_in_pos(worst_turbine, f, idx, current_solution);

        // Calculate the power contribution at this position
        // (ignoring the old turbine's effects on itself)
        double power_at_pos = calc_power_turbine_swapping_pos(candidate_t, current_solution);

        // Set the candidate's power
        candidate_t.powerCalculated = power_at_pos;

        // ! CHANGED MAXCOST MAXENERGY TO CR ER
        // Calculate opportunity for this candidate position
        double opportunity_val = position_opportunity(
            candidate_t, current_solution, Cr, Er, lambdaC, lambdaP
        );

        opportunity_val = opportunity_val / (1.0 + weight_fail * turbMemory[{zone_worst, idx}]);

        if(best_opport < opportunity_val){
            best_candidate = make_pair(opportunity_val, make_pair(idx, candidate_t));
            best_opport = opportunity_val;
        }
        c++;
    }

    // ========================================================================
    // Step 4: Select candidate position with highest opportunity
    // ========================================================================
    
    // Select position with highest opportunity
    Turbine best_turbine = best_candidate.second.second;

    // ========================================================================
    // Step 5: Perform relocation (swap turbines)
    // ========================================================================

    Solution* new_solution = swap_turbines(best_turbine, current_solution);

    archive.adicionarSol(new_solution);
    pareto->adicionarSol(new_solution);

    countRevalue++;

    if(countRevalue % 100000 == 0){
        string path = instance + "_" + algorithm + "_" + to_string(countRevalue) + ".txt";

        pareto->printAllSolutions(root_folder + path);
    }

    // ========================================================================
    // Step 6: Update memory
    // ========================================================================

    // // Accept if improved
    bool improved = dominatesP(*new_solution, *current_solution);

    pair<int,int> best_indexes = {zone_worst, best_turbine.index};

    if (improved) {
        // Success: reset failure count for the selected turbine
        // and record success for the candidate position
        turbMemory[worst.second] = 0;
        turbMemory[best_indexes] = 0;
    } else {
        // Failure: increment failure count for both turbines
        turbMemory[best_indexes]++;
        turbMemory[worst.second]++;
    }

    // cout << "-----------------------------------" << endl;
    // cout << "Solution Before: " << current_solution->fitness.first << " " << current_solution->fitness.second << endl;
    // cout << "Vector: " << lambdaC << " " << lambdaP << endl;
    // cout << "Solution After: " << new_solution->fitness.first << " " << new_solution->fitness.second << endl;
    // cout << "-----------------------------------" << endl;

    return new_solution;
}

// ============================================================================
// Main: WPPS local search for a population
// ============================================================================

// Local search wrapper — matches the local search interface
vector<Solution*>* wpPS_local_search(vector<Solution*> population) {
    // Use the global archive (already populated with solutions from main)
    // The archive is the BoundedParetoSet which contains the ParetoSetLS functionality
    ParetoSetLS * archive = new ParetoSetLS();

    for(Solution* sol : pareto->getElementos()){
        archive->adicionarSol(sol);
    }

    // Ensure archive has solutions (pareto should already be populated by main)
    // If empty, build from population
    if (archive->getSize() == 0) {
        for (Solution* sol : population) {
            archive->adicionarSol(sol);
        }
    }


    // Run the core WPPS algorithm
    vector<Solution*>* result = new vector<Solution*>();


    // Main loop: apply WPPS until stopping criteria
    for (Solution* sol : population) {
        // Apply WPPS local searchc
        Solution* improved_sol = wpPS_local_search(sol, *archive);

        // If improved, keep the new solution
        if(dominatesP(*improved_sol, *sol) || !dominatesP(*sol, *improved_sol)){
            result->push_back(improved_sol);
        } else{
            result->push_back(new Solution(*sol));
            delete improved_sol;
        }
    }

    cout << "ACTUAL COUNT: " << countRevalue << "| C_Greater " << c_greater << " E_Greater " << e_greater << endl;

    delete archive;

    return result;
}
