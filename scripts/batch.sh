#!/bin/bash

script="./main.sh"
create_folders="./smaller_scripts/create_folders.sh"

echo "Running script 1 = create_folders.sh"
bash "$create_folders"

nohup "$script" 501 502 504 503 505 A 61 87 &> "./logs/batch_501-505_A_61-87.txt" &
nohup "$script" 95 113 115 161 164 185 216 &> "./logs/batch_95-216.txt" &
nohup "$script" 226 246 278 324 342 347 352 355 &> "./logs/batch_226-355.txt" &
nohup "$script" 359 363 431 441 464 482 495 &> "./logs/batch_359-495.txt" &