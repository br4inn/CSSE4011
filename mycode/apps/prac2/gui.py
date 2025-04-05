import tkinter as tk
from tkinter import ttk, messagebox
import json
from datetime import datetime
import serial
import threading

# Global variables to store sensor data
sensor_data = {
    "time": [],
    "temp": [],
    "hum": [],
    "press": [],
    "mag_x": [],
    "mag_y": [],
    "mag_z": [],
}

# Function to draw the graph on the canvas
def draw_graph(canvas, data, title, color):
    canvas.delete("all")  # Clear the canvas

    # Draw title
    canvas.create_text(200, 20, text=title, font=("Arial", 14), fill="black")

    # Draw axes
    canvas.create_line(50, 250, 350, 250, fill="black")  # X-axis
    canvas.create_line(50, 250, 50, 50, fill="black")    # Y-axis

    # Check if there's data to plot
    if not data:
        canvas.create_text(200, 150, text="No Data", font=("Arial", 12), fill="red")
        return

    # Normalize data for plotting
    max_value = max(data)
    min_value = min(data)
    range_value = max_value - min_value if max_value != min_value else 1

    # Scale data to fit within the canvas
    scaled_data = [(250 - ((value - min_value) / range_value) * 200) for value in data]

    # Draw data points and lines
    for i in range(1, len(scaled_data)):
        x1 = 50 + (i - 1) * 30
        y1 = scaled_data[i - 1]
        x2 = 50 + i * 30
        y2 = scaled_data[i]
        canvas.create_line(x1, y1, x2, y2, fill=color, width=2)
        canvas.create_oval(x2 - 3, y2 - 3, x2 + 3, y2 + 3, fill=color)

# Function to update the graphs
def update_graphs():
    draw_graph(temp_canvas, sensor_data["temp"], "Temperature (°C)", "red")
    draw_graph(hum_canvas, sensor_data["hum"], "Humidity (%)", "blue")
    draw_graph(press_canvas, sensor_data["press"], "Pressure (hPa)", "green")
    draw_graph(mag_canvas, sensor_data["mag_x"], "Magnetometer X", "orange")

# Function to process JSON data
def process_json_data(data):
    did = data.get("did")
    timestamp = datetime.strptime(data["rtc_time"], "%Y-%m-%dT%H:%M:%S")
    sensor_data["time"].append(timestamp)

    if did == "0":  # Temperature
        sensor_data["temp"].append(data.get("value", 0))
    elif did == "1":  # Humidity
        sensor_data["hum"].append(data.get("value", 0))
    elif did == "2":  # Pressure
        sensor_data["press"].append(data.get("value", 0))
    elif did == "4":  # Magnetometer
        sensor_data["mag_x"].append(data.get("x", 0))
        sensor_data["mag_y"].append(data.get("y", 0))
        sensor_data["mag_z"].append(data.get("z", 0))
    elif did == "15":  # All Sensors
        sensor_data["temp"].append(data.get("temp_value", 0))
        sensor_data["hum"].append(data.get("hum_value", 0))
        sensor_data["press"].append(data.get("press_value", 0))
        sensor_data["mag_x"].append(data.get("mag_x", 0))
        sensor_data["mag_y"].append(data.get("mag_y", 0))
        sensor_data["mag_z"].append(data.get("mag_z", 0))

    # Update the graphs
    update_graphs()

# Function to read data from the microcontroller
def read_serial_data():
    try:
        # Open the serial port
        ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
        while True:
            line = ser.readline().decode('utf-8').strip()  # Read a line of data
            print(f"Raw data received: {line}")
            if line:
                try:
                    data = json.loads(line)  # Parse the JSON data
                    process_json_data(data)  # Process the data based on its format
                except json.JSONDecodeError:
                    print("Invalid JSON data received")
    except Exception as e:
        messagebox.showerror("Error", f"Failed to read serial data: {e}")

# Create the main window
root = tk.Tk()
root.title("Sensor Data Visualization")

# Create frames for each graph
temp_frame = ttk.LabelFrame(root, text="Temperature")
temp_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
temp_canvas = tk.Canvas(temp_frame, width=400, height=300, bg="white")
temp_canvas.pack()

hum_frame = ttk.LabelFrame(root, text="Humidity")
hum_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
hum_canvas = tk.Canvas(hum_frame, width=400, height=300, bg="white")
hum_canvas.pack()

press_frame = ttk.LabelFrame(root, text="Pressure")
press_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
press_canvas = tk.Canvas(press_frame, width=400, height=300, bg="white")
press_canvas.pack()

mag_frame = ttk.LabelFrame(root, text="Magnetometer")
mag_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
mag_canvas = tk.Canvas(mag_frame, width=400, height=300, bg="white")
mag_canvas.pack()

# Start a thread to read serial data
thread = threading.Thread(target=read_serial_data, daemon=True)
thread.start()

# Start the Tkinter event loop
root.mainloop()