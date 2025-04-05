from PyQt5.QtWidgets import QWidget, QVBoxLayout, QLabel
from PyQt5.QtCore import QTimer
import matplotlib.pyplot as plt
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas

class GraphWidget(QWidget):
    def __init__(self, parent=None):
        super(GraphWidget, self).__init__(parent)
        self.layout = QVBoxLayout(self)
        self.setLayout(self.layout)

        self.figure, self.ax = plt.subplots()
        self.canvas = FigureCanvas(self.figure)
        self.layout.addWidget(self.canvas)

        self.data = {
            "temperature": [],
            "humidity": [],
            "pressure": [],
            "magnetometer": []
        }
        self.time_stamps = []

        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_graph)
        self.timer.start(1000)  # Update every second

        self.label = QLabel("Sensor Data Graphs")
        self.layout.addWidget(self.label)

    def update_graph(self):
        # Here you would retrieve the latest sensor data
        # For demonstration, we will simulate data
        new_time_stamp = len(self.time_stamps)
        self.time_stamps.append(new_time_stamp)

        # Simulated sensor data
        self.data["temperature"].append(new_time_stamp + 20)  # Simulated temperature
        self.data["humidity"].append(new_time_stamp + 30)     # Simulated humidity
        self.data["pressure"].append(new_time_stamp + 1013)   # Simulated pressure
        self.data["magnetometer"].append(new_time_stamp + 5)   # Simulated magnetometer

        self.ax.clear()
        self.ax.plot(self.time_stamps, self.data["temperature"], label='Temperature')
        self.ax.plot(self.time_stamps, self.data["humidity"], label='Humidity')
        self.ax.plot(self.time_stamps, self.data["pressure"], label='Pressure')
        self.ax.plot(self.time_stamps, self.data["magnetometer"], label='Magnetometer')

        self.ax.set_xlabel('Time (s)')
        self.ax.set_ylabel('Sensor Values')
        self.ax.set_title('Sensor Data Over Time')
        self.ax.legend()
        self.canvas.draw()