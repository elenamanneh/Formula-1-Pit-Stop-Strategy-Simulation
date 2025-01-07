#!/bin/bash
#SBATCH --job-name=race_strategy_simulation
#SBATCH --output=output_%j.txt
#SBATCH --error=error_%j.txt
#SBATCH --time=01:00:00
#SBATCH --ntasks-per-node=2
#SBATCH --cpus-per-task=8
#SBATCH --nodes=3

module load gcc openmpi

make clean
make

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK

# mpirun -np <number_of_processes> ./<program_executable> <track_name> <stint_variable> <total_laps> <starting_lap_time>
mpirun -np $SLURM_NTASKS ./generate_strategies "Italian Grand Prix" 10 50 90.0
