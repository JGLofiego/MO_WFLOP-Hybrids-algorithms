# ifndef GLOBALS_H
# define GLOBALS_H

# include "../adaptative_grid/BoundedParetoSet.h"

const int SIZE_OF_POPULATION = 100;

// Global variables used by algorithms and local searches
extern int num_neighbors;
extern int countRevalue;
extern std::string perturbation_type_str;
extern std::string instance;
extern std::string root_folder;
extern BoundedParetoSet * pareto;
extern int stop_criteria;
extern std::string ils_variant_str;

# endif 
