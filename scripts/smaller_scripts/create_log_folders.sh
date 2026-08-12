ALGO="${ALGO:-nsga2_hybrid}"
LS="${LS:-apls}"

main_folder="./logs/${ALGO}_${LS}"

if [[ -d "$main_folder" ]]; then
    exit 0
fi

mkdir -p "$main_folder"