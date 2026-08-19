#ifndef PARETO_LS_H
#define PARETO_LS_H

#include "../adaptative_grid/ParetoSetLS.h"
#include "./neighborhood.h"

extern int max_iter_ls;

vector<Solution *> * pareto_ls(vector<Solution*> s, bool improved = false);

#endif