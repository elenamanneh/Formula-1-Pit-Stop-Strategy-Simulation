import fastf1
import os
import sys
import json

# Enable the cache
fastf1.Cache.enable_cache('data/cache')

# Track lengths in kilometers
TRACK_LENGTHS = {
    "Bahrain Grand Prix": 5.412,
    "Saudi Arabian Grand Prix": 6.174,
    "Australian Grand Prix": 5.278,
    "Azerbaijan Grand Prix": 6.003,
    "Miami Grand Prix": 5.412,
    "Monaco Grand Prix": 3.337,
    "Spanish Grand Prix": 4.657,
    "Canadian Grand Prix": 4.361,
    "Austrian Grand Prix": 4.318,
    "British Grand Prix": 5.891,
    "Hungarian Grand Prix": 4.381,
    "Belgian Grand Prix": 7.004,
    "Dutch Grand Prix": 4.259,
    "Italian Grand Prix": 5.793,
    "Singapore Grand Prix": 5.063,
    "Japanese Grand Prix": 5.807,
    "Qatar Grand Prix": 5.380,
    "United States Grand Prix": 5.513,
    "Mexico City Grand Prix": 4.304,
    "São Paulo Grand Prix": 4.309,
    "Las Vegas Grand Prix": 6.201,
    "Abu Dhabi Grand Prix": 5.281,
    "Tuscan Grand Prix": 5.245,
    "Eifel Grand Prix": 5.148,
    "Portuguese Grand Prix": 4.653,
    "Sakhir Grand Prix": 3.543,
    "Styrian Grand Prix": 4.318,
    "70th Anniversary Grand Prix": 5.891,
    "Emilia Romagna Grand Prix": 4.909,
    "Turkish Grand Prix": 5.338,
    "French Grand Prix": 5.842
}

def fetch_lap_times(year):
    # Fetch the event schedule for the specified year
    events = fastf1.get_event_schedule(year)
    all_data = []

    for _, event in events.iterrows():
        # Get race information
	event_name = event.get('EventName', "Unknown Event")
        track_length = TRACK_LENGTHS.get(event_name, None)
        round_number = event.get('RoundNumber', None)

        print(f"Processing event: {event_name}")

        if round_number is None or track_length is None:
            print(f"Skipping {event_name} due to missing RoundNumber or Track Length.")
            continue

        try:
            session = fastf1.get_session(year, round_number, 'R')
            session.load()

            total_laps = session.total_laps

            race_distance = track_length * total_laps if total_laps else None

            race_info = {
                "Track Name": event_name,
                "Track Length": f"{track_length:.3f} km",
                "Event Name": event['EventName'],
                "Date": str(event['EventDate']),
                "Round Number": round_number,
                "Total Laps": total_laps,
                "Race Distance": f"{race_distance:.3f} km" if race_distance else "Unknown",
                "Weather": {
                    "Air Temperature": session.weather_data['AirTemp'].mean() if not session.weather_data.empty else "Unknown",
                    "Track Temperature": session.weather_data['TrackTemp'].mean() if not session.weather_data.empty else "Unknown",
                    "Humidity": session.weather_data['Humidity'].mean() if not session.weather_data.empty else "Unknown"
                }
            }
            race_data = {"Race Information": race_info, "Lap Data": []}

            # Fetch lap data
            for driver_id, laps in session.laps.groupby("Driver"):
                for compound in laps["Compound"].unique():
                    compound_laps = laps[laps["Compound"] == compound]
                    for _, lap in compound_laps.iterrows():
                        lap_time = lap["LapTime"]
                        if lap_time is not None:
                            time_per_km = lap_time.total_seconds() / track_length
                            lap_details = {
                                "Compound": compound,
                                "Driver Name": session.get_driver(driver_id)["LastName"],
                                "Lap Time": f"{lap_time.total_seconds():.3f} s",
                                "Lap Number": lap["LapNumber"],
                                "Normalized Lap Time (s/km)": f"{time_per_km:.3f}",
                            }
                            race_data["Lap Data"].append(lap_details)

            all_data.append(race_data)

        except Exception as e:
            print(f"Error processing {event_name}: {e}")

    # Save data to JSON
    save_to_json(year, all_data)

def save_to_json(year, data):
    output_dir = "data/output"
    os.makedirs(output_dir, exist_ok=True)
    output_file = os.path.join(output_dir, f"race_data_{year}.json")

    try:
        with open(output_file, 'w') as f:
            json.dump(data, f, indent=4)
        print(f"Data successfully saved to {output_file}")
    except Exception as e:
        print(f"Error saving data to JSON file: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python fetch_data.py <year>")
        sys.exit(1)

    try:
        year = int(sys.argv[1])
        fetch_lap_times(year)
    except ValueError:
        print("Please provide a valid year as an argument.")
