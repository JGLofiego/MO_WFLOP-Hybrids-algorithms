ALGO="${ALGO:-nsga2_hybrid}"
LS="${LS:-apls}"
IMPROVED_NEIGHBOR="${IMPROVED_NEIGHBOR:-false}"

ALGOLS="${ALGO}_${LS}"
if [[ "$IMPROVED_NEIGHBOR" == "true" ]]; then
    ALGOLS="${ALGOLS}_imprvNeighb"
fi

main_folder="./logs/${ALGOLS}"

if [[ -d "$main_folder" ]]; then
    exit 0
fi

mkdir -p "$main_folder"