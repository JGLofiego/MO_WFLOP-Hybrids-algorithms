#include "../../headers/global_modules/local_search/neighborhood.h"

bool inNeighborhood(Solution s, vector<Solution*> & neighborhood){
    for(int j = 0; j < neighborhood.size(); j++){
        if(isEqual(s, *neighborhood[j])){
            return true;
        }
    }

    return false;
}

vector<Solution*> getNeighborhoodStd(Solution *s, int num_neighbors){
    vector<Solution *> neighbors;
    
    Solution sol = *s;

    neighbors.push_back(new Solution(sol));

    for(int i = 0; i < num_neighbors; i++){
        sol = *s;
        mutation(sol);

        bool result = inNeighborhood(sol, neighbors);
        if(result){
            while(result){
                sol = *s;
                mutation(sol);

                result = inNeighborhood(sol, neighbors);
            }
        }
        neighbors.push_back(new Solution(sol));
    }

    Solution * toDelete = *neighbors.begin();

    neighbors.erase(neighbors.begin());
    delete toDelete;

    return neighbors;
}

vector<Solution*> getNeighborhoodImproved(Solution *s, int num_neighbors){
    vector<Solution *> neighbors;
    function<void(Solution&)> mutations[3] = {mutation, mutationCostBased, mutationPowerBased};
    auto mutationFunc = mutation;
    
    Solution sol = *s;

    neighbors.push_back(new Solution(sol));

    for(int i = 0; i < num_neighbors; i++){
        sol = *s;
        if(i % 3 == 0){
            mutationFunc = mutation;
        } else if(i % 3 == 1){
            mutationFunc = mutationCostBased;
        } else {
            mutationFunc = mutationPowerBased;
        }
        mutationFunc(sol);

        bool result = inNeighborhood(sol, neighbors);
        if(result){
            while(result){
                sol = *s;
                mutationFunc(sol);

                result = inNeighborhood(sol, neighbors);
            }
        }
        neighbors.push_back(new Solution(sol));
    }

    Solution * toDelete = *neighbors.begin();

    neighbors.erase(neighbors.begin());
    delete toDelete;

    return neighbors;
}