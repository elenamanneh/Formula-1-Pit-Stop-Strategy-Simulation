#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <filesystem>
#include "include/nlohmann/json.hpp"

using json = nlohmann::json;

// Function to calculate degradation rates and average stint lengths
void calculateDegradationRates(const std::vector<std::string>& inputFiles) {
    std::map<std::string, std::map<std::string, std::pair<double, int>>> trackDegradation; // Track, Compound -> (Cumulative Degradation, Count)
    std::map<std::string, std::map<std::string, std::pair<int, int>>> trackStintData;      // Track, Compound -> (Total Stint Laps, Stint Count)

    for (const auto& inputFile : inputFiles) {
        std::ifstream file(inputFile);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << inputFile << "\n";
            continue;
        }

        try {
            json jsonData;
            file >> jsonData;

            std::map<std::string, std::map<std::string, std::vector<double>>> driverLaps;

            // Parse JSON data
            for (const auto& race : jsonData) {
                std::string trackName = race["Race Information"]["Track Name"];
                const auto& lapData = race["Lap Data"];
                for (const auto& lap : lapData) {
                    try {
                        std::string compound = lap["Compound"];
                        std::string driverName = lap["Driver Name"];
                        double lapTime = std::stod(std::string(lap["Normalized Lap Time (s/km)"]));

                        // Organize lap data by driver and compound
                        driverLaps[driverName][compound].push_back(lapTime);
                    } catch (const std::exception& e) {
                        std::cerr << "Error parsing lap data: " << e.what() << '\n';
                    }
                }

                // Calculate degradation rates and stints per compound
                for (const auto& [driver, compounds] : driverLaps) {
                    for (const auto& [compound, laps] : compounds) {
                        if (laps.size() < 2) continue; // Skip if not enough laps for degradation calculation

                        std::vector<std::pair<int, double>> sortedLaps;
                        for (int i = 0; i < laps.size(); ++i) {
                            sortedLaps.emplace_back(i, laps[i]);
                        }

                        // Sort by lap number
                        std::sort(sortedLaps.begin(), sortedLaps.end());

                        // Calculate degradation rate
                        double totalDegradation = 0.0;
                        int degradationCount = 0;
                        for (size_t i = 1; i < sortedLaps.size(); ++i) {
                            double degradation = sortedLaps[i].second - sortedLaps[i - 1].second;
                            if (degradation > 0) { // Only consider valid degradation
                                totalDegradation += degradation;
                                ++degradationCount;
                            }
                        }

                        // Accumulate degradation per track and compound
                        if (degradationCount > 0) {
                            trackDegradation[trackName][compound].first += totalDegradation;
                            trackDegradation[trackName][compound].second += degradationCount;
                        }

                        // Calculate stint data (consecutive lap groups)
                        int stintLength = 1;
                        for (size_t i = 1; i < sortedLaps.size(); ++i) {
                            if (sortedLaps[i].first == sortedLaps[i - 1].first + 1) {
                                ++stintLength; // Extend stint
                            } else {
                                // End of stint, accumulate
                                trackStintData[trackName][compound].first += stintLength;
                                trackStintData[trackName][compound].second += 1;
                                stintLength = 1; // Start a new stint
                            }
                        }

                        // Add the last stint
                        trackStintData[trackName][compound].first += stintLength;
                        trackStintData[trackName][compound].second += 1;
                    }
                }

                // Clear driver laps for next race
                driverLaps.clear();
            }
        } catch (const std::exception& e) {
            std::cerr << "Error processing file " << inputFile << ": " << e.what() << "\n";
        }
    }

    // Write output
    json outputJson;
    for (const auto& [track, compounds] : trackDegradation) {
        json compoundData;
        for (const auto& [compound, stats] : compounds) {
            double averageDegradation = stats.first / stats.second;
            double averageStintLength = static_cast<double>(trackStintData[track][compound].first) / 
                                        trackStintData[track][compound].second;

            compoundData[compound] = {
                {"Average Degradation", averageDegradation},
                {"Average Stint Length", averageStintLength}
            };
        }
        outputJson[track] = compoundData;
    }

    std::string outputDir = "./data/output";
    std::filesystem::create_directories(outputDir);

    std::string outputPath = outputDir + "/rates.json";
    std::ofstream outFile(outputPath);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open output file " << outputPath << "\n";
        return;
    }
    outFile << outputJson.dump(4); 
    outFile.close();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_json_file1> [<input_json_file2> ...]\n";
        return 1;
    }

    // Collect input files from command-line arguments
    std::vector<std::string> inputFiles(argv + 1, argv + argc);

    calculateDegradationRates(inputFiles);

    return 0;
}
