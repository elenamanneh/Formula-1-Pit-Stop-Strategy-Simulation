#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <mpi.h>
#include <omp.h>
#include "include/nlohmann/json.hpp"
#include <chrono>
#include <fstream>

// Manual profiling results file
std::ofstream profiling_log("profiling_manual.txt", std::ios::app);


using json = nlohmann::json;

struct Stint {
    std::string tyreType;
    int laps;
};

struct Strategy {
    std::vector<Stint> stints;
};

void generateStrategiesHelper(std::vector<Strategy>& strategies, Strategy currentStrategy, 
                              const json& trackData, int totalLaps, int remainingLaps, 
                              const std::vector<std::string>& compounds, int stintVariable) {
    auto start_time = std::chrono::high_resolution_clock::now(); // Start timer

    if (remainingLaps == 0) {
        strategies.push_back(currentStrategy);
        return;
    }

    for (const auto& compound : compounds) {
        if (!trackData.contains(compound)) {
            continue; // Skip if the compound is not valid for the track
        }

        int avgStint = trackData[compound]["Average Stint Length"];

        // Loop through valid stint lengths for this compound
        for (int stintLength = std::max(1, avgStint - stintVariable); 
             stintLength <= avgStint + stintVariable; ++stintLength) {
            if (stintLength > remainingLaps) {
                stintLength = remainingLaps; // Final stint: take the remaining laps
            }

            // Create a new strategy with the current stint
            Strategy newStrategy = currentStrategy;
            newStrategy.stints.push_back({compound, stintLength});

            // Recur with updated laps
            generateStrategiesHelper(strategies, newStrategy, trackData, totalLaps, 
                                     remainingLaps - stintLength, compounds, stintVariable);

            if (stintLength == remainingLaps) {
                break; // Stop exploring further if the final stint has been added
            }
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now(); // End timer
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

    profiling_log << "generateStrategiesHelper execution time: " << duration << " microseconds\n";
}

std::vector<Strategy> generateStrategies(int totalLaps, const json& trackData, 
                                         const std::vector<std::string>& compounds, int stintVariable) {
    std::vector<Strategy> strategies;
    Strategy initialStrategy;

    generateStrategiesHelper(strategies, initialStrategy, trackData, totalLaps, 
                             totalLaps, compounds, stintVariable);

    return strategies;
}

std::vector<Strategy> deserializeStrategies(const std::string& data) {
    std::vector<Strategy> strategies;
    std::stringstream ss(data);
    std::string line;
    while (std::getline(ss, line)) {
        Strategy strategy;
        std::stringstream lineStream(line);
        std::string tyreType;
        int laps;
        while (lineStream >> tyreType >> laps) {
            strategy.stints.push_back({tyreType, laps});
        }
        strategies.push_back(strategy);
    }
    return strategies;
}

std::string serializeStrategies(const std::vector<Strategy>& strategies) {
    std::stringstream ss;
    for (const auto& strategy : strategies) {
        for (const auto& stint : strategy.stints) {
            ss << stint.tyreType << " " << stint.laps << " ";
        }
        ss << "\n";
    }
    return ss.str();
}

double simulateRace(const Strategy& strategy, double startingLapTime, const json& trackData) {
    auto start_time = std::chrono::high_resolution_clock::now(); // Start timer

    double totalRaceTime = 0.0;
    const double basePitStopPenalty = 25.0; // Time penalty for a pit stop

    for (size_t i = 0; i < strategy.stints.size(); ++i) {
        const auto& stint = strategy.stints[i];
        double baseDegradation = trackData[stint.tyreType]["Average Degradation"];
        int maxStintLength = trackData[stint.tyreType]["Average Stint Length"]; 

        // Calculate total stint time with nonlinear degradation
        for (int lap = 0; lap < stint.laps; ++lap) {
            double nonlinearDegradation = baseDegradation * (1 + 0.02 * lap); // Quadratic degradation
            totalRaceTime += startingLapTime + lap * nonlinearDegradation;
        }

        // Add penalty if stint length exceeds maximum allowed length
        if (stint.laps > maxStintLength) {
            totalRaceTime += 100.0; // Penalty for unsafe usage
        }

        // Add dynamic pit stop penalty for all but the last stint
        if (i < strategy.stints.size() - 1) {
            double pitStopPenalty = basePitStopPenalty + 5.0 * (i + 1); // Increase penalty per stop
            totalRaceTime += pitStopPenalty;
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now(); // End timer
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

    profiling_log << "simulateRace execution time: " << duration << " microseconds\n";

    return totalRaceTime;
}


int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 5) {
        if (rank == 0) {
            std::cerr << "Usage: " << argv[0] << " <track_name> <stint_variable> <total_laps> <starting_lap_time>\n";
        }
        MPI_Finalize();
        return 1;
    }

    std::string trackName = argv[1];
    int stintVariable = std::stoi(argv[2]);
    int totalLaps = std::stoi(argv[3]);
    double startingLapTime = std::stod(argv[4]);

    std::ifstream ratesFile("../rate_calculation/data/output/rates.json");
    if (!ratesFile.is_open()) {
        if (rank == 0) {
            std::cerr << "Error: Could not open rates.json file.\n";
        }
        MPI_Finalize();
        return 1;
    }

    json ratesData;
    ratesFile >> ratesData;

    if (!ratesData.contains(trackName)) {
        if (rank == 0) {
            std::cerr << "Error: Track name '" << trackName << "' not found in rates.json.\n";
        }
        MPI_Finalize();
        return 1;
    }

    const auto& trackData = ratesData[trackName];
    std::vector<std::string> compounds = {"HARD", "MEDIUM", "SOFT"};

    std::vector<Strategy> strategies;
    if (rank == 0) {
        strategies = generateStrategies(totalLaps, trackData, compounds, stintVariable);
        std::string serializedData = serializeStrategies(strategies);

        int dataSize = serializedData.size();
        MPI_Bcast(&dataSize, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(serializedData.data(), dataSize, MPI_CHAR, 0, MPI_COMM_WORLD);
    } else {
        int dataSize;
        MPI_Bcast(&dataSize, 1, MPI_INT, 0, MPI_COMM_WORLD);

        char* buffer = new char[dataSize];
        MPI_Bcast(buffer, dataSize, MPI_CHAR, 0, MPI_COMM_WORLD);

        strategies = deserializeStrategies(std::string(buffer, dataSize));
        delete[] buffer;
    }

    int numStrategies = strategies.size();
    int chunkSize = (numStrategies + size - 1) / size;
    int start = rank * chunkSize;
    int end = std::min(start + chunkSize, numStrategies);

    std::vector<double> localResults(end - start);
    #pragma omp parallel for
    for (int i = start; i < end; ++i) {
        localResults[i - start] = simulateRace(strategies[i], startingLapTime, trackData);
    }

    std::vector<double> globalResults;
    if (rank == 0) {
        globalResults.resize(numStrategies);
    }

    MPI_Gather(localResults.data(), localResults.size(), MPI_DOUBLE,
               globalResults.data(), localResults.size(), MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        double minRaceTime = std::numeric_limits<double>::max();
        Strategy optimalStrategy;

        for (size_t i = 0; i < strategies.size(); ++i) {
            if (globalResults[i] < minRaceTime) {
                minRaceTime = globalResults[i];
                optimalStrategy = strategies[i];
            }
        }

        std::cout << "\nOptimal Strategy:\n";
        for (const auto& stint : optimalStrategy.stints) {
            std::cout << "  Tyre: " << stint.tyreType << ", Laps: " << stint.laps << "\n";
        }
        std::cout << "  Total Race Time: " << minRaceTime << " seconds\n";
    }

    MPI_Finalize();
    return 0;
}

