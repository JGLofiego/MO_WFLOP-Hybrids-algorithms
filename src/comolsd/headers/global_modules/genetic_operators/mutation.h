#ifndef MUTATION_H
#define MUTATION_H

#include <vector>
#include "../generate_initial_population/population.h"
#include "../../adaptative_grid/BoundedParetoSet.h"

void mutation(Solution &solution);
void mutationN(Solution &solution, int num_turb);

#endif 
