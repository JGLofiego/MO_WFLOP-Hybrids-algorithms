#ifndef NSGA2_HYBRID_H
#define NSGA2_HYBRID_H

#include "../../global_modules/generate_initial_population/generate_rSolution.h"
#include "./nsga2.h"
#include <functional>
#include <vector>
using namespace std;

vector<Solution*> nsga2_hybrid(vector<Solution>& pop,
    function<vector<Solution*>*(vector<Solution*>, bool)>,
    bool improvedNeighbor = false);

extern ofstream infoRunNSGA2;

#endif 