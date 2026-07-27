#include "./headers/main.h"
#include "./headers/globals.h"
#include "./headers/global_modules/local_search/local_search_type.h"

#include <iostream>
#include <string>
#include <fstream>
using namespace std;

int countRevalue = 0;

BoundedParetoSet * pareto = new BoundedParetoSet();
int stop_criteria = 1000000;
int max_iter_ls = stop_criteria;
string algorithm = "nsga2_hybrid";
string local_search_type = "apls";
string instance = "A";
string root_folder = "./";
int neighborhood_size = 10;

int main(int argc, char* argv[]){
    
    if(argc == 2){
        instance = argv[1];
    } else if (argc > 2){
        instance = argv[1];
        root_folder = argv[2];
    } else if (argc > 5){
        instance = argv[1];
        root_folder = argv[2];
        neighborhood_size = stoi(argv[5]);
    }
    if(argc > 6){
        algorithm = argv[6];
    }
    if(argc > 7){
        local_search_type = argv[7];
    }

    string path;
    
    int num_neighbors = 10;

    get_instance_info(argc, argv);

    LocalSearchType lsType = parseLocalSearchType(local_search_type);
    auto lsFunction = getLocalSearchFunction(lsType);

    auto population = create_initial_population(SIZE_OF_POPULATION);

    if (algorithm == "nsga2_hybrid"){
        nsga2_hybrid(population, lsFunction);
    } else if (algorithm == "moead_hybrid"){
        moead_hybrid(population, lsFunction);
    } else {
        cerr << "Unknown algorithm: " << algorithm << endl;
        return 1;
    }

    // cout << pathHypervolume + " " + pathHvParams + " " + param + " " + "hv.out" << endl;

    string pathAbsolute = "/home/jglofiego/Documentos/work/MO_WFLOP-Hybrids-algorithms/scripts/exe";

    string pathHypervolume = pathAbsolute + "/hyp_ind";
    string pathHvParams = pathAbsolute + "/hyp_ind_param_NONORM.txt";
    string refSet = pathAbsolute + "/dummy_ref.txt";

    string param = instance + "_" + algorithm + "_" + local_search_type + "_" + "1000000" + ".txt";

    auto _ = system((pathHypervolume + " " + pathHvParams + " " + param + " " + refSet + " " + "hv.out").c_str());

    ifstream hvFile("hv.out");
    string line;
    double hypervolume = 0.0;

    if (hvFile.is_open()) {
        while (getline(hvFile, line)) {
            hypervolume = stod(line);
        }
        hvFile.close();
        cout << hypervolume * -1 << endl;
    }
}