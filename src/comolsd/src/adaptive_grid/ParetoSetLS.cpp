#ifndef PARETOSETLS_CPP
#define PARETOSETLS_CPP

#include <list>
#include <map>
#include <math.h>
#include <cstdio>
#include <iomanip>
#include <fstream>
#include <iostream>

#include "../../headers/adaptative_grid/param.h"
#include "../../headers/adaptative_grid/Grid.h"
#include "../../headers/global_modules/generate_initial_population/generate_rSolution.h"
#include "../../headers/adaptative_grid/ParetoSetLS.h"

using namespace std;

double ohvc(pair<Solution*, bool> const & s, pair<Solution*, bool> const & s_){
    return abs((s.first->fitness.first - s_.first->fitness.first) * (s_.first->fitness.second - s.first->fitness.second));
};
double ParetoSetLS::ohiv(pair<Solution *, bool> a){
    double s_value = a.first->fitness.second;
    Solution s_inf = *a.first;
    Solution s_sup = *a.first;
    s_inf.fitness.second = 0.0;
    s_sup.fitness.second = 2.0 * s_value;


    list<pair<Solution *, bool>>::iterator it = sol.begin();
    while(it != sol.end()){
        if (it->first->fitness.second > s_value){
            if(it->first->fitness.second <= s_sup.fitness.second){
                s_sup = *it->first; 
            }
        } else if (it->first->fitness.second < s_value){
            if(it->first->fitness.second >= s_inf.fitness.second){
                s_inf = *it->first; 
            }
        }
        it++;
    }

    if(s_inf.fitness.second == 0){
        const pair<Solution *, bool> S_SUP = make_pair<Solution*, bool>(&(s_sup), false);
        return 2 * ohvc(a, S_SUP);
    }

    if(s_sup.fitness.second == 2 * s_value){
        const pair<Solution *, bool> S_INF = make_pair<Solution*, bool>(&(s_inf), false);
        return 2 * ohvc(S_INF, a);
    }

    const pair<Solution *, bool> S_SUP = make_pair<Solution*, bool>(&(s_sup), false);
    const pair<Solution *, bool> S_INF = make_pair<Solution*, bool>(&(s_inf), false);


    return ohvc(a, S_SUP) + ohvc(S_INF, a);
};

struct P {
    Solution solution;
    bool checked;
};

int ParetoSetLS::calcularGridPos(Solution &s) {
    int bit = 0;
    int gridPos = 0;
    for (int obj = 0; obj < NUMOBJETIVOS; obj++) {
        double inicio = rangeAtual[obj].min, fim = rangeAtual[obj].max, meio = (inicio + fim) / 2.0;
        for (int k = 0; k < PROFUNDIDADEGRID; k++) {
            if (getObj(&s, obj) >= meio) {
                gridPos |= (1 << bit);
                inicio = meio;
            } else {
                fim = meio;
            }
            meio = (inicio + fim) / 2.0;
            bit++;
        }
    }
    return gridPos;
}

void ParetoSetLS::updateGrid() {
    g.clearGrid();

    list<pair<Solution *, bool>>::iterator it = sol.begin();
    reiniciarRanges();
    while (it != sol.end()) {
        for (int k = 0; k < NUMOBJETIVOS; k++) {
            rangeAtual[k].min = rangeNovo[k].min = min(rangeAtual[k].min, getObj(it->first, k));
            rangeAtual[k].max = rangeNovo[k].max = max(rangeAtual[k].max, getObj(it->first, k));
        }
        it++;
    }

    it = sol.begin();
    while (it != sol.end()) {
        g.addGrid(calcularGridPos(*it->first));
        it++;
    }
}

void ParetoSetLS::reiniciarRanges() {
    #define INF 1e9
    for (int k = 0; k < NUMOBJETIVOS; k++) {
        rangeAtual[k].min = rangeNovo[k].min = INF;
        rangeAtual[k].max = rangeNovo[k].max = -INF;
    }
    #undef INF
}

ParetoSetLS::ParetoSetLS() {
    reiniciarRanges();
    memset(frequencia, 0, sizeof(frequencia));
    // Initialize maxima for normalization
    maxCost = -1.0;
    maxEnergy = -1.0;
    minCost = INFINITY;
    minEnergy = INFINITY;
}

ParetoSetLS::~ParetoSetLS() {
    clear();
}

int ParetoSetLS::getPositionCount(Solution &s) {
    return g.getPositionCount(calcularGridPos(s));
}

bool ParetoSetLS::allExplored(){
    list<pair<Solution *, bool>>::iterator it;
    for (it = sol.begin(); it != sol.end(); it++){
        if(!(*it).second){
            return false;
        }
    }
    return true;
}

int ParetoSetLS::getPositionCount(int p) {
    return g.getPositionCount(p);
}

list<pair<Solution *, bool>> ParetoSetLS::getElementos() {
    return sol;
}

list<pair<Solution *, bool>>::iterator ParetoSetLS::getBegin(){
    return sol.begin();
}

list<pair<Solution *, bool>>::iterator ParetoSetLS::getEnd(){
    return sol.end();
}

bool ParetoSetLS::adicionarSol(Solution *s) {
    ASS ( assert( confereGrid() ); )
    /* nem testa as solucoes piores */
	//if (s->getObj(0) >= ranges[0].max && s->getObj(1) >= ranges[1].max) {
		//fprintf(stderr,"Nem testei!\n");
		//return false;
	//}

    /* percorre o vetor de solucoes e de valores e, caso exista solucao dominada, retira e retorna true. caso contrario, retorna false */
    list<pair<Solution *, bool>>::iterator i = sol.begin();
    list<list<pair<Solution *, bool>>::iterator> remover;
    while (i != sol.end()) {
        if (dominatesP(*s, *i->first)) {
            remover.push_back(i);
            //printf("Dominada -> (%.3lf,%.3lf) por (%.3lf,%.3lf)!\n",(**i).getObj(0),(**i).getObj(1),s->getObj(0),s->getObj(1));
        }
        if (dominatesP(*i->first, *s) || equals(*i->first, *s)) {
            return false;
        }
        i++;
    }

    list<list<pair<Solution *, bool>>::iterator>::iterator j = remover.begin();
    while (j != remover.end()) {
        // remove do grid
        g.removeGrid(calcularGridPos(*(*j)->first));
        // remove do conjunto pareto
        delete((*j)->first);
        sol.erase(*j);
        j++;
    }

    Solution *t = new Solution;
    *t = *s;
    // adiciona ao conjunto pareto
    sol.push_front(make_pair(t, false));
    // adiciona ao grid
    g.addGrid(calcularGridPos(*t));

    for (int k = 0; k < NUMOBJETIVOS; k++) {
        rangeNovo[k].min = min(rangeNovo[k].min, getObj(t, k));
        rangeNovo[k].max = max(rangeNovo[k].max, getObj(t, k));
    }

    // Update maxima for normalization
    if (fabs(t->fitness.first) > maxCost) {
        maxCost = fabs(t->fitness.first);
    } else if(fabs(t->fitness.first) < minCost){
        minCost = fabs(t->fitness.first);
    }

    if (t->fitness.second > maxEnergy) {
        maxEnergy = t->fitness.second;
    } else if(fabs(t->fitness.second) < minEnergy){
        minEnergy = fabs(t->fitness.second);
    }

    // se houve uma mudanca grande nos ranges (maior que 10% valor), atualizar o grid
    for (int k = 0; k < NUMOBJETIVOS; k++) {
        if (fabs(rangeNovo[k].min - rangeAtual[k].min) > 0.1 * fabs(rangeAtual[k].min) ||
            fabs(rangeNovo[k].max - rangeAtual[k].max) > 0.1 * fabs(rangeAtual[k].max)) {
            //fprintf(stderr,"Atualizando grid!\n");
            updateGrid();
            break;
        }
    }

    ASS ( assert( confereGrid() ); )
    return true;
}

int ParetoSetLS::getSize() {
    return sol.size();
}

pair<Solution *, bool>ParetoSetLS::getSolucao(int p) {
    int c = 0;
    list<pair<Solution *, bool>>::iterator i = sol.begin();
    while (i != sol.end()) {
        if (c == p) return *i;
        i++;
        c++;
    }
    return *i;
}

void ParetoSetLS::clear() {
    list<pair<Solution *, bool>>::iterator i = sol.begin(), j;
    while (i != sol.end()) {
        j = i;
        i++;
        delete j->first;
    }
    sol.clear();
    g.clearGrid();
}

bool ParetoSetLS::confereGrid() {
    unsigned s = 0;
    for (int i = 0; i < g.getSize(); i++) s += g.getPositionCount(i);
    return s == sol.size();
}

Solution * ParetoSetLS::getRandomUnex(){
    vector<int> indexes;

    int size = sol.size();

    auto it = sol.begin();

    for(int i = 0; i < size; i++){
        if(!it->second){
            indexes.push_back(i);
        }
        it++;
    }

    if (indexes.empty()) {
        return nullptr;
    }

    int r = rand() % indexes.size();

    it = sol.begin();

    for(int j = 0; j < indexes[r]; j++){
        it++;
    }

    return it->first;
}

bool ParetoSetLS::markExplored(Solution *s) {
    for (auto &entry : sol) {
        if (entry.first == s) {
            entry.second = true;
            return true;
        }
    }
    return false;
}

Solution * ParetoSetLS::getNext(){
    double max_ohiv;
    Solution * nextOne;

    auto it = sol.begin();
    while(it != sol.end() && it->second){
        it++;
    }
    
    nextOne = it->first;
    max_ohiv = ohiv(*it);

    double aux;

    while(it != sol.end()){
        if(!it->second){
            aux = ohiv(*it);
            if(aux > max_ohiv){
                max_ohiv = aux;
                nextOne = it->first;
            }
        }
        it++;
    }

    return nextOne;
};

void ParetoSetLS::unexploreAll(){
    auto it = sol.begin();

    while(it != sol.end()){
        it->second = false;
        it++;
    }
}

// Getter implementations for maxCost and maxEnergy
double ParetoSetLS::getMaxCost() {
    return maxCost;
}

double ParetoSetLS::getMaxEnergy() {
    return maxEnergy;
}

double ParetoSetLS::getMinCost() {
    return minCost;
}

double ParetoSetLS::getMinEnergy() {
    return minEnergy;
}

// Pareto direction vector calculation
// Returns pair<double, double> (lambdaC, lambdaP) where:
//   lambdaC = DeltaP / (DeltaC + DeltaP)
//   lambdaP = DeltaC / (DeltaC + DeltaP)
// and lambdaC + lambdaP = 1
// The archive is expected to be sorted by cost (f1) ascending

pair<double, double> ParetoSetLS::calculate_pareto_dir_vec() {
    vector<Solution*> archive;
    for (auto it = getBegin(); it != getEnd(); ++it) {
        archive.push_back(it->first);
    }

    // Sort archive by cost (f1) ascending
    sort(archive.begin(), archive.end(), [](const Solution* a, const Solution* b) {
        return a->fitness.first > b->fitness.first;
    });

    int n = archive.size();
    if (n < 2) {
        // Not enough solutions to compute a gap
        return make_pair(0.5, 0.5);
    }

    // Compute gaps between consecutive solutions
    double maxDeltaC = -1.0;
    double maxDeltaP = -1.0;

    for (int i = 0; i < n - 1; i++) {
        double deltaC = abs(archive[i+1]->fitness.first - archive[i]->fitness.first);
        double deltaP = abs(archive[i+1]->fitness.second - archive[i]->fitness.second);

        // For normalization, use relative gaps (absolute gap / range)
        // Guard against division by zero
        double relDeltaC = deltaC / maxCost;
        double relDeltaP = deltaP / maxEnergy;

        if (relDeltaC > maxDeltaC) {
            maxDeltaC = relDeltaC;
            maxDeltaP = relDeltaP;
        }
    }

    // Compute direction vector weights
    double lambdaC = maxDeltaP / (maxDeltaC + maxDeltaP);
    double lambdaP = maxDeltaC / (maxDeltaC + maxDeltaP);

    // Normalize to ensure lambdaC + lambdaP = 1
    // if (lambdaC + lambdaP > 0) {
    //     double scale = 1.0 / (lambdaC + lambdaP);
    //     lambdaC *= scale;
    //     lambdaP *= scale;
    // }

    // cout << "PARETO DIR VEC: " << lambdaC << " " << lambdaP << endl;

    return make_pair(lambdaC, lambdaP);
}


// Assume f.first = custo (minimizar) e f.second = energia (maximizar).
// Se seu f.first é armazenado com sinal trocado, ajuste a ordenação.
Dir ParetoSetLS::pick_direction(Solution* cur) {
    vector<Solution*> a;
    for (auto it = getBegin(); it != getEnd(); ++it) a.push_back(it->first);
    sort(a.begin(), a.end(), [](auto* x, auto* y){ return x->fitness.first > y->fitness.first; });
    int n = a.size();
    if (n < 2) return {0.5, 0.5};

    double Cmin = a.front()->fitness.first, Cmax = a.back()->fitness.first;
    double Emin = a.front()->fitness.second, Emax = a.back()->fitness.second;
    double Cr = max(fabs(Cmax - Cmin), 1e-12), Er = max(fabs(Emax - Emin), 1e-12);


    // posição da solução corrente no arquivo (mais próxima em custo)
    int pos = 0;
    for (int i = 0; i < n; i++)
        if (fabs(a[i]->fitness.first - cur->fitness.first) <
            fabs(a[pos]->fitness.first - cur->fitness.first)) pos = i;

    struct Cand { double w; double lc, lp; };
    vector<Cand> c;
    auto gap = [&](int i, int j) {            // gap interno entre i e j
        double dC = fabs(a[j]->fitness.first  - a[i]->fitness.first)  / Cr;
        double dE = fabs(a[j]->fitness.second - a[i]->fitness.second) / Er;
        double s = dC + dE;
        return Cand{ hypot(dC, dE), dE / s, dC / s };
    };
    if (pos > 0)     c.push_back(gap(pos - 1, pos));
    if (pos < n - 1) c.push_back(gap(pos, pos + 1));
    // extensões nos extremos
    double ext = 1.5 * (1.0 / n);                 // peso fixo; calibre
    if (pos == 0)     c.push_back({ext, 1.0, 0.0});   // empurra custo p/ baixo
    if (pos == n - 1) c.push_back({ext, 0.0, 1.0});   // empurra energia p/ cima

    // roleta ~ w^2
    double tot = 0; for (auto& x : c) tot += x.w * x.w;
    double rand_double = rand() / (double)RAND_MAX;
    double r = tot * (rand_double), acc = 0;
    for (auto& x : c) { acc += x.w * x.w; if (r <= acc) return {x.lc, x.lp}; }
    return {c.back().lc, c.back().lp};
}

#endif
