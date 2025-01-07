#!/bin/bash
#SBATCH --job-name=race_strategy_simulation
#SBATCH --output=examples_output_%j.txt
#SBATCH --error=examples_error_%j.txt
#SBATCH --time=01:00:00
#SBATCH --ntasks-per-node=2
#SBATCH --cpus-per-task=8
#SBATCH --nodes=3

module load gcc openmpi

make clean
make

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK

# List of Grand Prix tracks with laps and estimated starting lap times
declare -A grand_prix
grand_prix["Italian Grand Prix"]="53 90.0"
grand_prix["Monaco Grand Prix"]="78 72.0"
grand_prix["Belgian Grand Prix"]="44 110.0"
grand_prix["Japanese Grand Prix"]="53 89.0"
grand_prix["Brazilian Grand Prix"]="71 80.0"

# Loop over each Grand Prix and run the simulation
for track in "${!grand_prix[@]}"; do
    IFS=' ' read -r laps starting_time <<< "${grand_prix[$track]}"
    echo "Running simulation for $track with $laps laps and starting lap time $starting_time seconds"
    mpirun -np $SLURM_NTASKS ./generate_strategies "$track" 10 "$laps" "$starting_time"
done

