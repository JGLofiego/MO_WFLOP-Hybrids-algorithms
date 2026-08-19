#!/bin/bash

# Script to run batch.sh with all possible parameter combinations
# Algorithms: nsga2_hybrid, moead_hybrid
# Local Search: pls, apls
# Improved Neighbor: false, true

script="./batch.sh"

algorithms=("nsga2_hybrid" "moead_hybrid")
local_searches=("pls" "apls")
improved_neighbors=("false" "true")

echo "Starting batch runs with all parameter combinations..."
echo "========================================================"

total=0
for algo in "${algorithms[@]}"; do
    for ls in "${local_searches[@]}"; do
        for improved in "${improved_neighbors[@]}"; do
            total=$((total + 1))
            echo ""
            echo "Run $total: ALGO=$algo LS=$ls IMPROVED_NEIGHBOR=$improved"
            echo "----------------------------------------"
            ALGO="$algo" LS="$ls" IMPROVED_NEIGHBOR="$improved" bash "$script"
        done
    done
done

echo ""
echo "========================================================"
echo "All $total batch runs completed!"
