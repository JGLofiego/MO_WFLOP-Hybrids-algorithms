#include "./headers/main.h"
#include "./headers/globals.h"
#include "./headers/global_modules/local_search/local_search_type.h"
#include "./headers/global_modules/local_search/ils.h"
#include "./headers/global_modules/local_search/ils_decomposition.h"
#include "./headers/global_modules/local_search/wpPS_local_search.h"

#include <iostream>
#include <string>
using namespace std;

int countRevalue = 0;
int num_neighbors = 50;
// int num_neighbors = 163;

BoundedParetoSet * pareto = new BoundedParetoSet();
int stop_criteria = 1000000;
int max_iter_ls = 12507; // irace best configuration
string algorithm = "wpPS"    ;
string local_search_type = "apls";
string instance = "A";
string root_folder = "./";
string perturbation_type_str = "fixed_moderate";
string ils_variant_str = "IILS";

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

    bool improvedNeighbor = false;

    // Parse flag-based arguments
    for(int i = 1; i < argc; i++){
        string arg = argv[i];
        if(arg == "--improvedNeighbor"){
            improvedNeighbor = true;
        }
        if(arg == "--perturbation" && i + 1 < argc){
            perturbation_type_str = argv[i + 1];
        }
        if(arg == "--variant" && i + 1 < argc){
            ils_variant_str = argv[i + 1];
        }
    }

    get_instance_info(argc, argv);

    int sum = 0;
    for(auto elem : turbines_per_zone)
        sum += elem;

    cout << endl;
    cout << "Number of subproblems: " << SIZE_OF_POPULATION << endl;
    cout << "Number of neighbors: " << num_neighbors << endl;
    cout << "Number of fixed turbines: " << fixd.size() << endl;
    cout << "Number of mobile turbines: " << sum << endl;
    cout << "Wind: " << wind << endl;
    cout << "Angle: " << angle << endl;

    cout << "Run time:" << endl;
    
    auto population = create_initial_population(SIZE_OF_POPULATION);
    
    // Convert to vector<Solution*> for local search interface
    vector<Solution*> pop_ptrs;
    for(Solution& s : population){
        pop_ptrs.push_back(new Solution(s));
    }

    LocalSearchType lsType = parseLocalSearchType(local_search_type);
    auto lsFunction = getLocalSearchFunction(lsType);

    if (algorithm == "nsga2_hybrid"){
        nsga2_hybrid(population, lsFunction, improvedNeighbor);
    } else if (algorithm == "moead_hybrid"){
        moead_hybrid(population, lsFunction, improvedNeighbor);
    } else if (algorithm == "pls"){
        // ILS as standalone: use the local search directly
        while(countRevalue < stop_criteria){
            auto result = pareto_ls(pop_ptrs, improvedNeighbor);
            
            for(Solution * s_ptrs : pop_ptrs){
                delete s_ptrs;
            } pop_ptrs.clear();

            for(Solution* s : *result){
                pop_ptrs.push_back(new Solution(*s));
                delete s;
            }

            delete result;
            cout << countRevalue << endl;
        }
    }  else if (algorithm == "apls"){
        // ILS as standalone: use the local search directly
        while(countRevalue < stop_criteria){
            auto result = anytime_pls(pop_ptrs, improvedNeighbor);
            
            for(Solution * s_ptrs : pop_ptrs){
                delete s_ptrs;
            } pop_ptrs.clear();

            for(Solution* s : *result){
                pop_ptrs.push_back(new Solution(*s));
                delete s;
            }

            delete result;
            cout << countRevalue << endl;
        }
    } else if (algorithm == "ils"){
        // ILS as standalone: use the local search directly
        while(countRevalue < stop_criteria){
            auto result = ils_local_search(pop_ptrs, improvedNeighbor);
            
            for(Solution * s_ptrs : pop_ptrs){
                delete s_ptrs;
            } pop_ptrs.clear();

            for(Solution* s : *result){
                pop_ptrs.push_back(new Solution(*s));
                delete s;
            }

            delete result;
        }
    } else if (algorithm == "ilsd"){
        // ILS/D as standalone: use the local search directly
        
        while(countRevalue < stop_criteria){
            auto result = ils_decomposition_local_search(pop_ptrs, improvedNeighbor);
            
            for(Solution * s_ptrs : pop_ptrs){
                delete s_ptrs;
            } pop_ptrs.clear();

            for(Solution* s : *result){
                pop_ptrs.push_back(new Solution(*s));
                delete s;
            }

            delete result;
        }

    } else if (algorithm == "wpPS") {
        // WPPS: Wake-Aware Pareto Pressure Search
        // Standalone local search variant (not part of the local search factory)
        // Uses the global archive for Pareto direction calculation
        while(countRevalue < stop_criteria){
            auto result = wpPS_local_search(pop_ptrs);

            for(Solution * s_ptrs : pop_ptrs){
                delete s_ptrs;
            } pop_ptrs.clear();

            for(Solution* s : *result){
                // pop_ptrs.push_back(new Solution(*s));
                delete s;
            }

            for(Solution* s : pareto->getElementos()){
                pop_ptrs.push_back(new Solution(*s));
            }



            delete result;
        }

    } else if (algorithm == "wpPS_m") {
        // WPPS-M: WPPS with adaptive memory
        // Same as WPPS but with adaptive memory penalties
        while(countRevalue < stop_criteria){
            auto result = wpPS_local_search(pop_ptrs);

            for(Solution * s_ptrs : pop_ptrs){
                delete s_ptrs;
            } pop_ptrs.clear();

            for(Solution* s : *result){
                pop_ptrs.push_back(new Solution(*s));
                delete s;
            }

            delete result;
        }

    } else {
        cerr << "Unknown algorithm: " << algorithm << endl;
        return 1;
    }
    
    // Cleanup
    for(Solution* s : pop_ptrs){
        delete s;
    }

    ////////////////////////////////////////////////// 

    // int countRandomSolutionsGenerated = 0;

    // while(countRevalue < stop_criteria){
    //     Solution* s = new Solution(generate_solution());
    //     pareto->adicionarSol(s);
    //     countRevalue++;

    //     if(countRevalue % 100000 == 0){
    //         string path = instance + "_" + algorithm + "_" + to_string(countRevalue) + ".txt";

    //         pareto->printAllSolutions(root_folder + path);
    //     }

    //     Solution * local_opt = ls(*s, num_neighbors, improvedNeighbor);

    //     while(dominatesP(*local_opt, *s)){
    //         *s = *local_opt;
    //         local_opt = ls(*local_opt, num_neighbors, improvedNeighbor);
    //     }

    //     delete s;
    //     countRandomSolutionsGenerated++;
    // }

    // cout << countRandomSolutionsGenerated << endl;


}
