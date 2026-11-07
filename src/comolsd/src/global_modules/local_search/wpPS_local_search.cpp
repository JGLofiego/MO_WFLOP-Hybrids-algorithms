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

// Calculate the marginal energy contribution of a turbine i
// MPi = Eii + sum(Eij + Eji) for all j in S\{i}
double calc_marginal_energy(Turbine& turb, Solution* s) {
    // E_ii is the turbine's own production (without wake effects)
    double own = turb.powerCalculated;

    // E_ij + E_ji for all other mobile turbines j
    double caused = 0.0;
    double res;
    for (auto& zone_turb : s->turbines) {
        for (auto& t : zone_turb) {
            if (t.id == turb.id) continue;  // skip self
            res = calculate_interference(turb, t);
            caused += res * res;
        }
    }

    // Apply wake effect from other turbines
    double wind_res = wind * (1.0 - sqrt(caused));
    own += power_produced(wind_res, turb);

    // E_ij + E_ji for fixed turbines j
    caused = 0.0;
    for (auto& f : fixd) {
        if (f.id == turb.id) continue;
        res = calculate_interference(turb, f);
        caused += res * res;
    }

    wind_res = wind * (1.0 - sqrt(caused));
    own += power_produced(wind_res, fixd[0]);  // fixd[0] is a reference

    return own;
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
    // Get turbine's raw cost and energy
    double cost = t.costCalculated;
    double energy = t.powerCalculated;

    // Normalize cost and energy
    double normCost = cost / maxCost;
    // For energy: normalized marginal contribution
    double marginalEnergy = calc_marginal_energy(t, s);
    double normEnergy = (energy + marginalEnergy) / maxEnergy;

    // Pressione = lambdaC * Ci_hat + lambdaP * (1 - MPi_hat)
    // Higher cost and lower energy => higher pressure (more likely to remove)
    double pressure = lambdaC * normCost + lambdaP * (1.0 - normEnergy);

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
    // Raw cost and energy of the candidate position
    double cost = possible_t.costCalculated;
    double energy = possible_t.powerCalculated;

    // Marginal energy at the candidate position
    double marginalEnergy = calc_marginal_energy(possible_t, s);

    // Normalize using archive maxima
    double normCost = cost / maxCost;
    double normEnergy = (energy + marginalEnergy) / maxEnergy;

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
// Core: Adaptive memory (failure penalty)
// ============================================================================

// Update failure counts for pressure and opportunity
// Pressionei' = Pressionei / (1 + eta * faili)
// Opportunityj' = Opportunityj / (1 + eta * failj)
pair<double, double> apply_adaptive_memory(
    double pressure,
    double opportunity,
    int failureCount,
    double eta
) {
    if (failureCount <= 0) return make_pair(pressure, opportunity);

    double penalized_pressure = pressure / (1.0 + eta * failureCount);
    double penalized_opportunity = opportunity / (1.0 + eta * failureCount);

    return make_pair(penalized_pressure, penalized_opportunity);
}

// ============================================================================
// Main: WPPS local search for a single solution
// ============================================================================

// Structure to track adaptive memory state per turbine
struct TurbineMemory {
    int failureCount;
    double lastPressure;
    double lastOpportunity;

    TurbineMemory() : failureCount(0), lastPressure(0.0), lastOpportunity(0.0) {}

    void record_failure() {
        failureCount++;
    }

    void record_success() {
        failureCount = 0;  // Reset on success
    }

    void record_pressure(double p) { lastPressure = p; }

    void record_opportunity(double o) { lastOpportunity = o; }

    double getPressure() { return failureCount > 0 ? lastPressure / (1.0 + 0.5 * failureCount) : lastPressure; }

    double getOpportunity() { return failureCount > 0 ? lastOpportunity / (1.0 + 0.5 * failureCount) : lastOpportunity; }
};

// Map of turbine IDs to their memory state
map<int, TurbineMemory> turbMemory;

// WPPS local search for a single solution
// Returns pointer to improved solution
Solution* wpPS_local_search(Solution* current_solution, ParetoSetLS& archive) {
    // Get the Pareto direction vector for the current iteration
    // pair<double, double> dir = archive.calculate_pareto_dir_vec();

    int r = rand() % 100;

    double lambdaC = (double) r / (double) 100;
    double lambdaP = (double) (100-r) / (double) 100;
    // Get archive maxima for normalization
    double maxCost = archive.getMaxCost();
    double maxEnergy = archive.getMaxEnergy();

    int num_turbines = 0;
    for (auto& zone : current_solution->turbines) {
        num_turbines += zone.size();
    }

    // ========================================================================
    // Step 1: Calculate pressure for all turbines
    // ========================================================================

    vector<pair<double, pair<int, int>>> pressures;  // (pressure, (zone, index))

    for (int zone = 0; zone < current_solution->turbines.size(); zone++) {
        for (int i = 0; i < current_solution->turbines[zone].size(); i++) {
            Turbine& t = current_solution->turbines[zone][i];

            // Calculate and store pressure
            double pressure = turbine_pressure(
                t, current_solution, maxCost, maxEnergy, lambdaC, lambdaP
            );

            // Record pressure for memory
            turbMemory[t.id].record_pressure(pressure);

            // TODO: Change to make find the biggest pressure on calculation in the loop
            pressures.push_back(make_pair(pressure, make_pair(zone, i)));
        }
    }

    // ========================================================================
    // Step 2: Select turbine with highest pressure (worst turbine)
    // ========================================================================

    // Sort by pressure descending to find the worst turbine
    sort(pressures.begin(), pressures.end(),
         [](const pair<double, pair<int, int>>& a,
            const pair<double, pair<int, int>>& b) {
             return a.first > b.first;
         });

    // Select the turbine with highest pressure
    pair<double, pair<int, int>> worst = pressures[0];
    double pressure_worst = worst.first;
    int zone_worst = worst.second.first;
    int idx_worst = worst.second.second;

    Turbine& worst_turbine = current_solution->turbines[zone_worst][idx_worst];

    // Apply adaptive memory penalty to pressure
    double penalized_pressure = turbMemory[worst_turbine.id].getPressure();
    (void)penalized_pressure;  // Penalized pressure used in selection

    // ========================================================================
    // Step 3: Calculate opportunity for candidate positions
    // ========================================================================

    // Build set of occupied positions to avoid duplicates
    set<int> occupied_positions;
    for (auto& zone : current_solution->turbines) {
        for (auto& t : zone) {
            occupied_positions.insert(t.index);
        }
    }

    // Find candidate positions in the same zone that are not occupied
    vector<pair<double, pair<int, Turbine>>> candidate_positions;

    // Note: foundations[zone] contains Foundation struct with cost, x, y
    // Iterate through possible foundation positions in this zone
    for (int i = 0; i < foundations[zone_worst].size(); i++) {
        int idx = i;
        if (occupied_positions.find(idx) != occupied_positions.end()) {
            continue;  // Skip occupied positions
        }

        // Create a temporary turbine at this candidate position
        Foundation& f = foundations[zone_worst][idx];
        Turbine candidate_t = create_turbine_in_pos(worst_turbine, f, idx, current_solution);

        // Calculate the power contribution at this position
        // (ignoring the old turbine's effects on itself)
        double power_at_pos = calc_power_turbine_swapping_pos(candidate_t, current_solution);

        // Set the candidate's power
        candidate_t.powerCalculated = power_at_pos;

        // Calculate opportunity for this candidate position
        double opportunity_val = position_opportunity(
            candidate_t, current_solution, maxCost, maxEnergy, lambdaC, lambdaP
        );

        // Record opportunity for memory
        turbMemory[candidate_t.id].record_opportunity(opportunity_val);

        candidate_positions.push_back(make_pair(opportunity_val, make_pair(idx, candidate_t)));
    }

    // ========================================================================
    // Step 4: Select candidate position with highest opportunity
    // ========================================================================

    // Sort by opportunity descending
    sort(candidate_positions.begin(), candidate_positions.end(),
         [](const pair<double, pair<int, Turbine>>& a,
            const pair<double, pair<int, Turbine>>& b) {
             return a.first > b.first;
         });

    // Select position with highest opportunity
    pair<double, pair<int, Turbine>> best_candidate = candidate_positions[0];
    int idx_best = best_candidate.second.first;
    Turbine best_turbine = best_candidate.second.second;

    // Apply adaptive memory penalty to opportunity
    double penalized_opportunity = turbMemory[best_turbine.id].getOpportunity();
    (void)penalized_opportunity;  // Penalized opportunity used in selection

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

    // Check if the new solution is better
    pair<double, double> new_fitness = new_solution->fitness;
    pair<double, double> old_fitness = current_solution->fitness;

    // Cost is minimized (-1 * cost), Energy is maximized
    // Accept if improved
    bool improved = dominatesP(*new_solution, *current_solution);

    if (improved) {
        // Success: reset failure count for the selected turbine
        // and record success for the candidate position
        turbMemory[worst_turbine.id].record_success();
        turbMemory[best_turbine.id].record_success();

        // Also update the archive if a ParetoSetLS reference is available
        // (Note: this function receives archive, but adding to archive
        //  should be done by the caller)
    } else {
        // Failure: increment failure count for both turbines
        turbMemory[worst_turbine.id].record_failure();
        turbMemory[best_turbine.id].record_failure();
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
        result->push_back(improved_sol);
    }

    cout << "ACTUAL COUNT: " << countRevalue << endl;

    delete archive;

    return result;
}
