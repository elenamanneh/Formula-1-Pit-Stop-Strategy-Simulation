#!/bin/bash
#SBATCH --job-name=strong_scaling
#SBATCH --output=strong_scaling_%j.log
#SBATCH --error=strong_scaling_error_%j.log
#SBATCH --time=01:00:00
#SBATCH --ntasks-per-node=2
#SBATCH --cpus-per-task=8
#SBATCH --nodes=3

module load gcc openmpi

# Rebuild the program
make clean
make

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK

# Fixed problem parameters
TRACK_NAME="Italian Grand Prix"
STINT_VARIABLE=10
TOTAL_LAPS=50
STARTING_LAP_TIME=90.0

# Number of processors to test
PROCESSORS=(1 2 4 8 16 32)

# File to store results
RESULTS_FILE="strong_scaling_results.txt"
echo "Processes,Walltime,Speedup,Efficiency" > $RESULTS_FILE

# Measure execution time for different processor counts
BASE_TIME=0

for P in "${PROCESSORS[@]}"; do
  echo "Running with $P processes..."
  
  # Record start time
  START_TIME=$(date +%s.%N)
  
  # Run the simulation
  mpirun -np $P ./generate_strategies "$TRACK_NAME" $STINT_VARIABLE $TOTAL_LAPS $STARTING_LAP_TIME
  
  # Record end time
  END_TIME=$(date +%s.%N)
  
  # Calculate elapsed time
  WALLTIME=$(echo "$END_TIME - $START_TIME" | bc)
  
  # Set base time for speedup calculation
  if [ "$P" -eq 1 ]; then
    BASE_TIME=$WALLTIME
  fi
  
  # Calculate speedup and efficiency
  SPEEDUP=$(echo "$BASE_TIME / $WALLTIME" | bc -l)
  EFFICIENCY=$(echo "$SPEEDUP / $P" | bc -l)
  
  # Log results
  echo "$P,$WALLTIME,$SPEEDUP,$EFFICIENCY" >> $RESULTS_FILE
done

# Print the results in table format
echo -e "\nStrong Scaling Results:"
column -s, -t $RESULTS_FILE

