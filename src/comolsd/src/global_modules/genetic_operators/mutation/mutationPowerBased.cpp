#include "../../../../headers/global_modules/generate_initial_population/population.h"
#include "../../../../headers/global_modules/genetic_operators/mutationPowerBased.h"
#include <algorithm>
#include <cstdlib>
using namespace std;

/*The goal of this mutation is: change the turbine with higher Power with a empty space, ensuring that 
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

void mutationPowerBased(Solution &solution){

  int z_low = 0;
  int i_low = 0;
  double power_low = solution.turbines[0][0].powerCalculated;

  for(int z = 0; z < solution.turbines.size(); z++){
    for(int i = 0; i < solution.turbines[z].size(); i++){
        if(power_low > solution.turbines[z][i].powerCalculated){
            z_low = z;
            i_low = i;
            power_low = solution.turbines[z][i].powerCalculated;
        }
    }
  }
  
  mutateTurb(solution, z_low, i_low);

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