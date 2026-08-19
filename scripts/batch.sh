#!/bin/bash

# How to call the batch
# LS=$LS ALGO=$ALGO ./batch.sh

# instances=(329 327 304 394 107 421 59 460 277 J 286 253 130 302 229 430 307 436 385 154 367 449 67 211 356 275 488 233 273 470 263 84 296 170 156 123 376 251 89 337 490 437 2 452 29 298 358 472 74 456 368 276 261 80 395 65 462 349 388 399 419 287 122 465 6 23 274 240 266 230 42 132 193 500 414 443 344 444 166 56 79 11 133 371 479 403 I 440 35 194 458 F 149 53 294 313 68 331 496 272 78 409 131 141 177 357 120 105 442 318 3 69 351 314 72 232 432 111 473 333 85 293 D 392 10 183 27 144 323 366)

ALGO="${ALGO:-nsga2_hybrid}"
LS="${LS:-apls}"
IMPROVED_NEIGHBOR="${IMPROVED_NEIGHBOR:-false}"
export ALGO
export LS
export IMPROVED_NEIGHBOR

ALGOLS="${ALGO}_${LS}"
if [[ "$IMPROVED_NEIGHBOR" == "true" ]]; then
    ALGOLS="${ALGOLS}_imprvNeighb"
fi

script="./main.sh"
create_folders="./smaller_scripts/create_folders.sh"
create_log_folders="./smaller_scripts/create_log_folders.sh"

echo "Running script 1 = create_folders.sh for ALGO=$ALGO LS=$LS"
bash "$create_folders"
bash "$create_log_folders"

# nohup "$script" 501 &> "./logs/${ALGOLS}/batch_501_.txt" &

nohup "$script" 501 502 504 503 505 A 61 87 95 113 &> "./logs/${ALGOLS}/batch_501-505_61-113_A.txt" &
nohup "$script" 115 161 164 185 216 226 &> "./logs/${ALGOLS}/batch_115-226.txt" &
nohup "$script" 246 278 324 342 347 352 355 359 &> "./logs/${ALGOLS}/batch_246-359.txt" &
nohup "$script" 363 431 441 464 482 495 &> "./logs/${ALGOLS}/batch_363-495.txt" &

# Process instances in batches of two
# echo "Running batches of 2 instances each..."
# for ((i = 0; i < ${#instances[@]}; i += 2)); do
#   instance1=${instances[$i]}
#   if ((i + 1 < ${#instances[@]})); then
#     instance2=${instances[$((i + 1))]}
#     nohup "$script" "$instance1" "$instance2" &> "./logs/${ALGOLS}/batch_${instance1}_${instance2}.txt" &
#     echo "Batch: $instance1, $instance2"
#   else
#     # Handle odd number of instances
#     nohup "$script" "$instance1" &> "./logs/${ALGOLS}/batch_${instance1}.txt" &
#     echo "Batch: $instance1 (single)"
#   fi
# done