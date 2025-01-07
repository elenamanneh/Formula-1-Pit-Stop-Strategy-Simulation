#!/bin/bash

echo "Building the rate_calculation program..."
make

if [[ $? -eq 0 ]]; then
    echo "Build successful. Running the program..."
    ./rate_calculation ../fetch_data/data/output/race_data_2024.json \
                       ../fetch_data/data/output/race_data_2023.json \
                       ../fetch_data/data/output/race_data_2022.json \
                       ../fetch_data/data/output/race_data_2021.json \
                       ../fetch_data/data/output/race_data_2020.json
else
    echo "Build failed. Please check for errors and try again."
fi
