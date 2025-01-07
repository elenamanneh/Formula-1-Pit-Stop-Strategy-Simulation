#include <iostream>
#include <fstream>
#include <string>
#include "include/nlohmann/json.hpp"

using json = nlohmann::json;

double calculateStintTime(double startingLapTime, double degradationRate, int numLaps) {
    double totalTime = 0.0;
    for (int i = 0; i < numLaps; ++i) {
        totalTime += startingLapTime;
        startingLapTime += degradationRate; // Lap time increases due to degradation
    }
    return totalTime;
}

int main(int argc, char* argv[]) {
    if (argc < 6) {
        std::cerr << "Usage: " << argv[0] << " <starting_lap_time> <tyre_type> <num_laps> <rates_json_path> <track_name>\n";
        return 1;
    }

    // Parse command line arguments
    double startingLapTime = std::stod(argv[1]);
    std::string tyreType = argv[2];
    int numLaps = std::stoi(argv[3]);
    std::string ratesJsonPath = argv[4];
    std::string trackName = argv[5];

    // Load the JSON file
    std::ifstream ratesFile(ratesJsonPath);
    if (!ratesFile.is_open()) {
        std::cerr << "Error: Could not open rates JSON file at " << ratesJsonPath << "\n";
        return 1;
    }

    json ratesData;
    try {
        ratesFile >> ratesData;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing rates JSON: " << e.what() << "\n";
        return 1;
    }

    // Check if the track exists
    if (ratesData.find(trackName) == ratesData.end()) {
        std::cerr << "Error: Track '" << trackName << "' not found in rates JSON.\n";
        return 1;
    }

    // Check if the tyre type exists for the track
    const auto& trackData = ratesData[trackName];
    if (trackData.find(tyreType) == trackData.end()) {
        std::cerr << "Error: Tyre type '" << tyreType << "' not found for track '" << trackName << "'.\n";
        return 1;
    }

    // Get the degradation rate for the tyre type
    double degradationRate = trackData[tyreType]["Average Degradation"];
    std::cout << "Using degradation rate: " << degradationRate << " for tyre type: " << tyreType << "\n";

    // Calculate the total stint time
    double totalStintTime = calculateStintTime(startingLapTime, degradationRate, numLaps);

    std::cout << "Total stint time: " << totalStintTime << " seconds\n";

    return 0;
}
