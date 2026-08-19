#ifndef NEIGHBORHOOD_H
#define NEIGHBORHOOD_H

#include "../genetic_operators/mutation.h"
#include "../genetic_operators/mutationCostBased.h"
#include "../genetic_operators/mutationPowerBased.h"
#include "../isEqual.h"
#include <functional>

vector<Solution*> getNeighborhoodStd(Solution *s, int num_neighbors);

vector<Solution*> getNeighborhoodImproved(Solution *s, int num_neighbors);

#endif