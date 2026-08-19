#include "../../../../headers/global_modules/generate_initial_population/population.h"
#include "../../../../headers/global_modules/genetic_operators/mutationCostBased.h"
#include <algorithm>
#include <cstdlib>
using namespace std;

/*The goal of this mutation is: change the turbine with higher cost with a empty space, ensuring that 
the probability of improving the result is higher.
*/

//Function to find the index of a turbine (searching for its id) within of a vector of turbines in zone i.
static int find_index(vector<Turbine> &vec, int value){
  for(int i = 0; i < vec.size(); i++){
    if(vec[i].id == value){
      return i;
    }
  }

  return -1;
}

static void mutateTurb(Solution &solution, int zone, int index){

    int zone_size = solution.grid[zone].size();
    int rand_intA = solution.turbines[zone][index].index;
    int rand_intB;
    int indexA = -1;
    int indexB = -1;

    // Select rand_intB position at random in zone i of the solution.grid, ensuring it is a empty space
    do{
      rand_intB = rand() % zone_size;
    } while(solution.grid[zone][rand_intB] != 0);

    // Swapping the genes (turbine_id) betwen grid[zone][rand_intA] e grid[zone][rand_intB]
    int geneA = solution.grid[zone][rand_intA];
    int geneB = solution.grid[zone][rand_intB];

    if(solution.grid[zone][rand_intA] != 0){
      indexA = find_index(solution.turbines[zone], solution.grid[zone][rand_intA]);
    }

    if(solution.grid[zone][rand_intB] != 0){
      indexB = find_index(solution.turbines[zone], solution.grid[zone][rand_intB]);
    }

    if (indexA != -1 && indexB == -1){
      solution.turbines[zone][indexA].index = rand_intB;
      solution.turbines[zone][indexA].x = foundations[zone][rand_intB].x;
      solution.turbines[zone][indexA].y = foundations[zone][rand_intB].y;
    } else {
      solution.turbines[zone][indexB].index = rand_intA;
      solution.turbines[zone][indexB].x = foundations[zone][rand_intA].x;
      solution.turbines[zone][indexB].y = foundations[zone][rand_intA].y;
    }

    solution.grid[zone][rand_intA] = geneB;
    solution.grid[zone][rand_intB] = geneA;
}

void mutationCostBased(Solution &solution){

  int z_high = 0;
  int i_high = 0;
  double cost_high = solution.turbines[0][0].costCalculated * -1.0;

  for(int z = 0; z < solution.turbines.size(); z++){
    for(int i = 0; i < solution.turbines[z].size(); i++){
        if(cost_high < (solution.turbines[z][i].costCalculated * -1.0)){
            z_high = z;
            i_high = i;
            cost_high = solution.turbines[z][i].costCalculated * -1.0;
        }
    }
  }
  
  mutateTurb(solution, z_high, i_high);

  calculate_cost(solution);
  calculate_power(solution);

  Solution *sol = new Solution(solution);
  pareto->adicionarSol(sol);

  countRevalue++;

  if(countRevalue % 100000 == 0){
    string path = instance + "_" + algorithm + "_" + to_string(countRevalue) + ".txt";

    pareto->printAllSolutions(root_folder + path);
  }

  delete sol;
}