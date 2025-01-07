#!/bin/bash
#SBATCH --job-name=weak_scaling_race_strategy
#SBATCH --output=weak_scaling_%j.log
#SBATCH --error=weak_scaling_error_%j.log
#SBATCH --time=01:00:00
#SBATCH --ntasks-per-node=2
#SBATCH --cpus-per-task=8
#SBATCH --nodes=3

module load gcc openmpi

# Rebuild the program
make clean
make

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK

# Fixed parameters
TRACK_NAME="Italian Grand Prix"
BASE_STINT=10   # Stint workload for 1 processor
BASE_LAPS=50    # Number of laps for 1 processor

# Number of processors to test
PROCESSORS=(1 2 4 8 16 32)

# File to store results
RESULTS_FILE="weak_scaling_results.txt"
echo "Processes,Stint,Laps,Walltime" > $RESULTS_FILE

for P in "${PROCESSORS[@]}"; do
  echo "Running with $P processes..."

  # Scale the workload
  STINT=$(($BASE_STINT * $P))
  LAPS=$(($BASE_LAPS + $BASE_LAPS / 10 * $P))

  # Record start time
  START_TIME=$(date +%s.%N)

  # Run the simulation
  mpirun -np $P ./generate_strategies "$TRACK_NAME" $STINT $LAPS $STARTING_LAP_TIME

  # Record end time
  END_TIME=$(date +%s.%N)

  # Calculate elapsed time
  WALLTIME=$(echo "$END_TIME - $START_TIME" | bc)

  # Log results
  echo "$P,$STINT,$LAPS,$WALLTIME" >> $RESULTS_FILE
done

# Print the results in table format
echo -e "\nWeak Scaling Results:"
column -s, -t $RESULTS_FILE

