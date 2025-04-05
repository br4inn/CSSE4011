from typing import List, Dict
import json
import os

class DataManager:
    def __init__(self, data_file: str):
        self.data_file = data_file
        self.sensor_data = {}

    def load_data(self) -> None:
        if not os.path.exists(self.data_file):
            raise FileNotFoundError(f"Data file {self.data_file} not found.")
        
        with open(self.data_file, 'r') as file:
            self.sensor_data = json.load(file)

    def get_sensor_data(self, did: str) -> List[Dict]:
        if did not in self.sensor_data:
            raise ValueError(f"No data found for Device Identifier: {did}")
        
        return self.sensor_data[did]

    def get_all_sensor_data(self) -> Dict[str, List[Dict]]:
        return self.sensor_data

    def update_sensor_data(self, did: str, new_data: Dict) -> None:
        if did not in self.sensor_data:
            self.sensor_data[did] = []
        
        self.sensor_data[did].append(new_data)
        self.save_data()

    def save_data(self) -> None:
        with open(self.data_file, 'w') as file:
            json.dump(self.sensor_data, file, indent=4)