#include "./headers/main.h"
#include "./headers/globals.h"

#include <iostream>
#include <string>
#include <fstream>
using namespace std;

int countRevalue = 0;

BoundedParetoSet * pareto = new BoundedParetoSet();
int stop_criteria = 1000000;
string algorithm = "comolsd";
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
    
    string path;
    
    int num_neighbors = 10;

    get_instance_info(argc, argv);

    int sum = 0;
    for(auto elem : turbines_per_zone)
        sum += elem;

    // cout << endl;
    // cout << "Number of subproblems: " << SIZE_OF_POPULATION << endl;
    // cout << "Number of neighbors: " << num_neighbors << endl;
    // cout << "Number of fixed turbines: " << fixd.size() << endl;
    // cout << "Number of mobile turbines: " << sum << endl;
    // cout << "Wind: " << wind << endl;
    // cout << "Angle: " << angle << endl << endl;

    // cout << "Run time:" << endl;
    
    auto population = create_initial_population(SIZE_OF_POPULATION);
    // nsga2_pls(population);
    moead_pls(population);

    string pathAbsolute = "/home/jglofiego/Documentos/work/MO_WFLOP-Hybrids-algorithms/scripts/exe";

    string pathHypervolume = pathAbsolute + "/hyp_ind";
    string pathHvParams = pathAbsolute + "/hyp_ind_param_NONORM.txt";
    string refSet = pathAbsolute + "/dummy_ref.txt";

    string param = instance + "_" + algorithm + "_" + "1000000" + ".txt";

    // cout << pathHypervolume + " " + pathHvParams + " " + param + " " + "hv.out" << endl;

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