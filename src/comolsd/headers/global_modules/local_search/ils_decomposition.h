#ifndef ILS_DECOMPOSITION_LS_H
#define ILS_DECOMPOSITION_LS_H

#include <vector>
#include "../../wind_farm_entities/solution.h"

// Local search interface function for ILS/D
std::vector<Solution*>* ils_decomposition_local_search(std::vector<Solution*> population, bool improvedNeighbor);

#endif
