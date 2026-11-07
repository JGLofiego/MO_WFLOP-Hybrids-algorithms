#ifndef WPPS_LOCAL_SEARCH_H
#define WPPS_LOCAL_SEARCH_H

#include <vector>
#include <utility>
#include <map>
#include <set>
#include <cmath>
#include "wind_farm_entities/solution.h"
#include "wind_farm_entities/turbine.h"
#include "wind_farm_entities/foundations.h"
#include "adaptative_grid/ParetoSetLS.h"

// Function declarations for WPPS local search

// Compute scalar value for a solution using adaptive lambda
// Uses Tchebycheff scalarization
double scalar_value(const pair<double, double>& fitness,
                    const pair<double, double>& lambda,
                    const pair<double, double>& z_point);

// Core: Turbine pressure calculation
// Pressionei = lambdaC * Ci_hat + lambdaP * (1 - MPi_hat)
double turbine_pressure(
    Turbine& t,
    Solution* s,
    double maxCost,
    double maxEnergy,
    double lambdaC,
    double lambdaP
);

// Core: Position opportunity calculation
// Opportunityj = lambdaP * MPj_hat - lambdaC * Cj_hat
double position_opportunity(
    Turbine& possible_t,
    Solution* s,
    double maxCost,
    double maxEnergy,
    double lambdaC,
    double lambdaP
);

// Core: Create turbine at new position
Turbine create_turbine_in_pos(Turbine& t_base, Foundation& f, int index_f, Solution* s);

// Core: Calculate power contribution of a turbine at a new position
double calc_power_turbine_swapping_pos(Turbine& new_t, Solution* s);

// Core: Swap turbine and update solution
Solution* swap_turbines(Turbine new_t, Solution* s);

// Core: Adaptive memory update
pair<double, double> apply_adaptive_memory(
    double pressure,
    double opportunity,
    int failureCount,
    double eta = 0.5
);

// Main: WPPS local search for a single solution
// Returns pointer to improved solution
Solution* wpPS_local_search(Solution* current_solution, ParetoSetLS& archive);

// Main: WPPS local search for a population
// Returns vector of pointers to solutions
vector<Solution*>* wpPS_local_search(vector<Solution*> population);

#endif
