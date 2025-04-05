# Sensor Graph GUI

This project is a desktop application designed to display JSON sensor output as time-series graphs. It supports various sensors identified by their Device Identifiers (DIDs) and provides a user-friendly interface for visualizing sensor data in real-time.

## Project Structure

The project is organized as follows:

```
sensor-graph-gui
├── src
│   ├── main.py               # Entry point of the application
│   ├── gui                   # GUI components
│   │   ├── __init__.py       # Marks the gui directory as a package
│   │   ├── main_window.py     # Main application window setup
│   │   └── graph_widget.py    # Graph rendering for sensor data
│   ├── data                  # Data handling components
│   │   ├── __init__.py       # Marks the data directory as a package
│   │   ├── json_parser.py     # JSON parsing functions
│   │   └── data_manager.py     # Manages data flow within the application
│   └── utils                 # Utility functions
│       ├── __init__.py       # Marks the utils directory as a package
│       └── logger.py          # Logging functionality
├── requirements.txt          # Project dependencies
├── README.md                 # Project documentation
└── .gitignore                # Files to ignore in version control
```

## Setup Instructions

1. **Clone the repository**:
   ```
   git clone <repository-url>
   cd sensor-graph-gui
   ```

2. **Install dependencies**:
   It is recommended to use a virtual environment. You can create one using:
   ```
   python -m venv venv
   source venv/bin/activate  # On Windows use `venv\Scripts\activate`
   ```
   Then install the required packages:
   ```
   pip install -r requirements.txt
   ```

3. **Run the application**:
   Execute the main script to start the application:
   ```
   python src/main.py
   ```

## Usage Guidelines

- Upon launching the application, you will see the main window displaying the graphs for the enabled sensors.
- The application supports real-time updates of sensor data, allowing you to visualize changes as they occur.
- You can configure which sensors to display by modifying the settings in the application.

## Supported Device Identifiers (DIDs)

The application supports the following Device Identifiers for enabled sensors:

- **0**: Temperature Sensor
- **1**: Humidity Sensor
- **2**: Pressure Sensor
- **4**: Magnetometer Sensor

Ensure that the appropriate sensors are enabled in your configuration to visualize their data.

## Contributing

Contributions to the project are welcome. Please fork the repository and submit a pull request with your changes. Make sure to follow the coding standards and include tests for new features.

## License

This project is licensed under the MIT License. See the LICENSE file for more details.