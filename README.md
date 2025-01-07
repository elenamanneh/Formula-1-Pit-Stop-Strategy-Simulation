
# Formula 1 Pit-Stop Strategy Simulation: High-Performance Computing Approach to Pit-Stop Strategy

## The Problem
- F1 races involve numerous complex decisions during a race, including pit stop strategies, tire selection, and overall race strategy.
- Tire wear is integral to formulating these strategies.
- **Goal:** Find the right balance between tire degradation, lap times, tire types, and number of pit stops.

## Prerequisites
1. **Data Acquisition and Cleanup**
   - Fetched data from [FastF1](https://docs.fastf1.dev/).
   - Data includes race information and historical lap data (lap timings, tire compounds, etc.).
   - **Tools Used:** Python for fetching and preprocessing.

2. **Data Manipulation**
   - Used lap data to calculate:
     - Average degradation rate.
     - Stint length per compound per track.

## Workflow (Simulation)
1. Inputs: `track_name`, `total_laps`, `starting_lap_time`, `stint_variable`.
2. Generates strategies based on:
   - `tire_degradation_rate`, `average_stint_length`, and `stint_variable`.
   - **Pit-stop window:** `average_stint_length ± stint_variable`.
3. Simulates race with:
   - **Stint Time Calculator:** Calculates race time with nonlinear degradation.
   - Penalties for exceeding stint length or making pit stops (dynamic for all but the last stint).
4. Determines the optimal strategy.

## Parallelization
### MPI (Message Passing Interface)
- **Distribution:**
  - Root process generates strategies using `generateStrategies`.
  - Broadcasts serialized strategies to all MPI nodes with `MPI_Bcast`.
  - Each node processes a proportional chunk of strategies.
- **Collection:**
  - Nodes send results back to the root process using `MPI_Gather`.
  - Root consolidates results to find the optimal strategy.

### OpenMP
- **Computation:**
  - Strategies assigned to each MPI node are parallelized with OpenMP threads.
  - Threads execute `simulateRace` for subsets of strategies, leveraging shared memory for efficiency.

### Hybrid Approach
- Combines MPI (node-level parallelism) and OpenMP (thread-level parallelism).
- Ensures scalability and efficient resource utilization.

## Profiling
1. **`simulateRace`:**
   - Primary bottleneck due to high call frequency and computational intensity.
   - Workload scales directly with strategies and laps.
2. **`generateStrategiesHelper`:**
   - Recursive strategy generation; less frequent than `simulateRace`.
   - Benefits from MPI distribution.
3. **Insights:**
   - Manual profiling highlighted `simulateRace` as the most expensive function.
   - `gprof` provided limited insights due to template-heavy libraries like `nlohmann/json`.

## Scaling Analysis

### Strong Scaling
- **Observations:**
  - Minimal runtime improvement with additional resources for smaller problem sizes.
  - Overheads from parallelization and communication dominate.
- **Key Takeaway:**
  - Efficiency is constrained by parallelization overhead for smaller workloads.

### Weak Scaling
- **Observations:**
  - Stable runtimes as problem size scales proportionally with processes.
  - Slight performance degradation due to inter-process communication overhead.
- **Key Takeaway:**
  - Reasonable scalability with potential for improvement in communication and synchronization.

## Sample Results
### Belgian Grand Prix (44 laps, starting lap time: 110.0 seconds)
- **Optimal Strategy:**
  - Tyre: HARD, Laps: 22
  - Tyre: HARD, Laps: 22
- **Total Race Time:** 4933.79 seconds (1:22:13.79).
- **Actual Race Time:** 1:19:57.566 (2024 winner).

### Monaco Grand Prix (78 laps, starting lap time: 72.0 seconds)
- **Optimal Strategy:**
  - Tyre: SOFT, Laps: 18
  - Repeated for four stints.
  - Final stint: Tyre: SOFT, Laps: 6.
- **Total Race Time:** 6121.79 seconds.
- **Observation:** Unrealistic strategy. Actual Monaco races average two pit stops.
