#!/bin/bash

mkdir -p data/output

CURRENT_YEAR=$(date +%Y)

# Loop through the last 5 years (including the current year)
for ((year=CURRENT_YEAR-4; year<=CURRENT_YEAR; year++)); do
    OUTPUT_FILE="data/output/race_data_${year}.json"
    echo "Running data collection for year $year"

    # Takes in year as an argument 
    python3 fetch_data.py "$year"

    if [[ $? -eq 0 ]]; then
        if [[ -f "$OUTPUT_FILE" ]]; then
            echo "Data for year $year saved successfully to $OUTPUT_FILE."
        else
            echo "Data collection for year $year completed, but the JSON file was not created."
        fi
    else
        echo "Failed to collect data for year $year. Check logs for details."
    fi
done

