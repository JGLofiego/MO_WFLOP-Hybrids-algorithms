#ifndef ILS_LS_H
#define ILS_LS_H

#include <vector>
#include "../../wind_farm_entities/solution.h"
#include <functional>
# include <string>
# include <fstream>
# include "../../global_modules/generate_initial_population/generate_rSolution.h"
# include "../../adaptative_grid/ParetoSetLS.h"

enum class PerturbationType {
    FixedWeak,      // 1 mutation
    FixedModerate,  // 3 mutations
    Random,         // 1-5 mutations
    Adaptive        // Start weak, increase if no improvement
};

PerturbationType parsePerturbationType(const std::string& typeStr);

Solution perturb(Solution s, PerturbationType type, int& consecutive_no_improvement);

// Local search interface function for ILS
std::vector<Solution*>* ils_local_search(std::vector<Solution*> population, bool improvedNeighbor);

#endif
