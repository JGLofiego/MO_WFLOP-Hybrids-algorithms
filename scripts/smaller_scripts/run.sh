#!/bin/bash

ALGO="${ALGO:-nsga2_hybrid}"
LS="${LS:-apls}"
IMPROVED_NEIGHBOR="${IMPROVED_NEIGHBOR:-false}"

ALGOLS="${ALGO}_${LS}"
if [[ "$IMPROVED_NEIGHBOR" == "true" ]]; then
    ALGOLS="${ALGOLS}_imprvNeighb"
fi

instance="${1:-A}" 
run="${2:-1}"

echo "Executing $ALGO with: Instance=$instance, Run=$run"

log_dir="../dataset/${ALGOLS}/$instance/$run"
log_file="$log_dir/log.txt"

path="../dataset/${ALGOLS}/$instance/$run/"

angle=0.0
wind=0.0

base_log="../dataset/${ANGLE_WIND_SOURCE:-moead}/$instance/$run/log.txt"
if [[ -f "$base_log" ]]; then
    read_angle=$(grep "Angle:" "$base_log" | awk '{print $2}')
    read_wind=$(grep "Wind:" "$base_log" | awk '{print $2}')
    [[ -n "$read_angle" ]] && angle="$read_angle"
    [[ -n "$read_wind" ]] && wind="$read_wind"
fi

mkdir -p "$log_dir"

# Build command with optional flag
cmd="./exe/main \"$instance\" \"$path\" \"$angle\" \"$wind\" \"$ALGO\" \"$LS\""
if [[ "$IMPROVED_NEIGHBOR" == "true" ]]; then
    cmd="$cmd --improvedNeighbor"
fi

{
    echo "============================== LOG =============================="
    echo "Instance: $instance"
    echo "Run: $run"
    echo "Metaheuristic: $ALGO"
    echo "Local Search: $LS"
    echo "Improved Neighbor: $IMPROVED_NEIGHBOR"
    echo "Execution started at: $(date)"

    { time eval "$cmd"; } 2>&1

    echo "Execution ended at: $(date)"
    echo "================================================================="
} > "$log_file" 2>&1
