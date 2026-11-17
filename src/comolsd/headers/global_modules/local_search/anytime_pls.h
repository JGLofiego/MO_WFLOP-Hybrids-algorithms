#ifndef ANYTIME_PLS_H
#define ANYTIME_PLS_H

#include "../adaptative_grid/ParetoSetLS.h"
#include "./neighborhood.h"

extern int max_iter_ls;
extern int num_neighbors;

vector<Solution *> * anytime_pls(vector<Solution*> s, bool improved = false);

#endif