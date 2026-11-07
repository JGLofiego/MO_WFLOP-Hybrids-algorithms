#include "./headers/main.h"
#include "./headers/globals.h"
#include "./headers/global_modules/local_search/local_search_type.h"

#include <iostream>
#include <string>
using namespace std;

int countRevalue = 0;

BoundedParetoSet * pareto = new BoundedParetoSet();
int stop_criteria = 1000000;
int max_iter_ls = 12507;
string algorithm = "nsga2_hybrid";
string local_search_type = "apls";
string instance = "A";
string root_folder = "./";

int main(int argc, char* argv[]){

    if(argc == 2){
        instance = argv[1];
    } else if (argc > 2){
        instance = argv[1];
        root_folder = argv[2];
    }
    if(argc > 5){
        algorithm = argv[5];
    }
    if(argc > 6){
        local_search_type = argv[6];
    }

    string path;

    int num_neighbors = 10;
    bool improvedNeighbor = false;

    for(int i = 1; i < argc; i++){
        string arg = argv[i];
        if(arg == "--improvedNeighbor"){
            improvedNeighbor = true;
        }
    }

    get_instance_info(argc, argv);

    int sum = 0;
    for(auto elem : turbines_per_zone)
        sum += elem;
    
    auto population = create_initial_population(SIZE_OF_POPULATION);

    LocalSearchType lsType = parseLocalSearchType(local_search_type);
    auto lsFunction = getLocalSearchFunction(lsType);

    while(countRevalue < stop_criteria){
        auto elementList = pareto->getElementos();
        vector<Solution *> * elementVector = new vector<Solution*>();

        for(Solution * sol: elementList){
            elementVector->push_back(sol);
        }


        auto newPopulation = lsFunction(*elementVector, improvedNeighbor);

        for(Solution * s : *newPopulation){
            cout << s->fitness.first << " " << s->fitness.second << endl;
        } cout << endl;

        elementVector->clear();
        delete elementVector;
    }
    
}