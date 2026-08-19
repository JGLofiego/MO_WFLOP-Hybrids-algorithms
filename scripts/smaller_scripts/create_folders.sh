#!/bin/bash

ALGO="${ALGO:-nsga2_hybrid}"
LS="${LS:-apls}"
IMPROVED_NEIGHBOR="${IMPROVED_NEIGHBOR:-false}"

ALGOLS="${ALGO}_${LS}"
if [[ "$IMPROVED_NEIGHBOR" == "true" ]]; then
    ALGOLS="${ALGOLS}_imprvNeighb"
fi

main_dir="../dataset/${ALGOLS}"

if [[ -d "$main_dir" ]]; then
    echo "The directory structure for ${ALGOLS} has already been created."
    exit 0
fi

mkdir -p "$main_dir"

echo "------------- Creating the directory structure for $ALGO with LS=$LS... -------------"

for D in {A..J} {0..500} {501..505}; do
    for subdir in {1..20}; do
        mkdir -p "$main_dir/$D/$subdir"
    done
done

echo "---------- Directory structure created successfully for ${ALGOLS}! ----------"
