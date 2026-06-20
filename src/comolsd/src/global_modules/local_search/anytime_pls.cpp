#include "../../headers/global_modules/local_search/pareto_ls.h"
#include "../../headers/instance_info.h"

vector<Solution *> * anytime_pls(vector<Solution*> population){
    ParetoSetLS* p = new ParetoSetLS();

    int neighborhood_size = 163;

    for(int i = 0; i < population.size(); i++){
        p->adicionarSol(population[i]);
    }


    pair<Solution *, bool> * it;

    // Stop on first solution "improved"
    while(!(p->allExplored()) && countRevalue < stop_criteria){
        // cout << "it before: " << it << endl;
        it = p->getNext();
        // cout << "it after: " << it << endl;

        vector<Solution *> neighborhood = getNeighborhood(it->first, neighborhood_size);

        bool added = false;

        for(int i = 0; i < neighborhood.size(); i++){
            if(dominatesP(*neighborhood[i], *it->first)){
                p->adicionarSol(neighborhood[i]);
                added = true;
                break;
            }
        }
        
        if(!added){
            for(int i = 0; i < neighborhood.size(); i++){
                p->adicionarSol(neighborhood[i]);
                break;
            }
        }

        it->second = true;

        for(auto p: neighborhood){
            delete p;
        }
        
        neighborhood.clear();
        countRevalue -= (neighborhood_size - 1);
    }

    p->unexploreAll();

    // Stop on neighborhood fully explored
    while(!(p->allExplored()) && countRevalue < stop_criteria){
        it = p->getNext();

        vector<Solution *> neighborhood = getNeighborhood(it->first, neighborhood_size);

        bool added = false;

        for(int i = 0; i < neighborhood.size(); i++){
            if(dominatesP(*neighborhood[i], *it->first)){
                p->adicionarSol(neighborhood[i]);
                added = true;
            }
        }
        
        if(!added){
            for(int i = 0; i < neighborhood.size(); i++){
                p->adicionarSol(neighborhood[i]);
            }
        }

        it->second = true;

        for(auto p: neighborhood){
            delete p;
        }
        neighborhood.clear();
    }

    vector<Solution *> * result = new vector<Solution*>();

    for(auto i = p->getBegin(); i != p->getEnd(); i++){
        result->push_back((i)->first);
    }

    p->clear();
    delete p;

    return result;
}

// int main(int argc, char* argv[]){
//     get_instance_info(argc, argv);

//     auto initpop = create_initial_population(100);

//     vector<Solution*> pop;

//     for(int i = 0; i < initpop.size(); i++){
//         Solution * s = new Solution;
//         *s = initpop[i];
//         pop.push_back(s);
//     }

//     pop = pareto_ls(pop);
    
// }