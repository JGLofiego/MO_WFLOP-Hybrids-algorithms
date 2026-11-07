#include "../../headers/global_modules/local_search/local_search_type.h"
#include "../../headers/global_modules/local_search/pareto_ls.h"
#include "../../headers/global_modules/local_search/anytime_pls.h"
#include "../../headers/global_modules/local_search/ils.h"
#include "../../headers/global_modules/local_search/ils_decomposition.h"
#include <stdexcept>

using namespace std;

LocalSearchType parseLocalSearchType(const string& typeStr){
    if(typeStr == "pls") return LocalSearchType::ParetoLS;
    if(typeStr == "apls") return LocalSearchType::AnytimePLS;
    if(typeStr == "none") return LocalSearchType::None;
    if(typeStr == "ils") return LocalSearchType::ILSLocalSearch;
    if(typeStr == "ilsd") return LocalSearchType::ILSDLocalSearch;
    throw invalid_argument("Unknown local search type: " + typeStr);
}

function<vector<Solution*>*(vector<Solution*>, bool)> getLocalSearchFunction(LocalSearchType type){
    switch(type){
        case LocalSearchType::ParetoLS:
            return pareto_ls;
        case LocalSearchType::AnytimePLS:
            return anytime_pls;
        case LocalSearchType::ILSLocalSearch:
            return ils_local_search;
        case LocalSearchType::ILSDLocalSearch:
            return ils_decomposition_local_search;
        case LocalSearchType::None:
        default:
            return [](vector<Solution*> pop, bool) -> vector<Solution*>* {
                return new vector<Solution*>(pop);
            };
    }
}
