import serial
import json
import random
import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import numpy as np
from matplotlib.widgets import Button, TextBox
import matplotlib.animation as animation
 
import requests
 

# ---------------- Model ----------------
class TrackingModel:
    def __init__(self, simulate=True):
        self.simulate = simulate
        self.grid = 'A'
        self.influx_url = "http://localhost:8086/api/v2/write?org=uq&bucket=prac3&precision=s"
        self.influx_token = "9qF9f9iYm6HskdbanV7BO9yTOq6dl6QBzg67KXwMWAGzj8KNJLusuZC7Hr49OWL5syXI1WlzhOi5CoAmPUhvQQ=="
        self.influx_headers = {
            "Authorization": f"Token {self.influx_token}",
            "Content-Type": "text/plain"
        }
        if not simulate:
            self.ser = serial.Serial('/dev/ttyACM0', 115200)
        else:
            self.ser = None

    def send_to_influx(self, node, rssi, avg_vel, dist_travelled):
    # Format the data using InfluxDB line protocol
        payload = f"node_data,node={node} rssi={rssi},avg_vel={avg_vel},dist_travelled={dist_travelled}"
        print(f"Sending to InfluxDB: {payload}")

        try:
            # Send the data to InfluxDB
            response = requests.post(self.influx_url, data=payload, headers=self.influx_headers)
            if response.status_code != 204:
                print(f"InfluxDB error: {response.text}")
        except Exception as e:
            print(f"Failed to send data to InfluxDB: {e}")

    def get_position(self):
        if self.simulate:
            x = round(random.uniform(0, 3), 2)
            y = round(random.uniform(0, 4), 2)
            return x, y
        else:
            try:
                line = self.ser.readline().decode().strip()
                print(f"[Serial] {line}")  # Debug
                data = json.loads(line)

                # Extract required fields
                x = data.get("x", 0) / 100.0
                y = 4 - data.get("y", 0) / 100.0
                vel = data.get("vel", 0)
                tot_dist = data.get("tot_dist", 0)

                # RSSI values for nodes A to M
                rssi_values = {key: data.get(key, -120) for key in "ABCDEFGHIJKLM"}

                # Debug  
                print(f"RSSI Values: {rssi_values}")

                # Send data  
                for node, rssi in rssi_values.items():
                    self.send_to_influx(node, rssi, vel, tot_dist)

                return x, y

            except Exception as e:
                print(f"Serial error: {e}")
                return 0, 0

    def toggle_grid(self):
        self.grid = 'B' if self.grid == 'A' else 'A'

    def get_grid(self):
        return self.grid

# ---------------- View ----------------
label_sets = {
    'A': {'top': ['A', 'B', 'C'], 'bottom': ['G', 'F', 'E'], 'left': ['H'], 'right': ['D']},
    'B': {'top': ['C', 'I', 'J'], 'bottom': ['E', 'M', 'L'], 'left': ['D'], 'right': ['K']}
}

class TrackingView:
    def __init__(self):
        self.fig, self.ax = plt.subplots()
        plt.subplots_adjust(bottom=0.3)  # extra space now for more widgets

        self.ax.set_xlim(0, 3)
        self.ax.set_ylim(0, 4)
        self.ax.grid(True, linestyle='-', color='blue', alpha=0.5)
        self.ax.set_xticks(np.linspace(0, 3, 8))
        self.ax.set_yticks(np.linspace(0, 4, 8))
        self.ax.tick_params(labelbottom=False, labelleft=False)

        self.label_texts = {}  # <-- make it a dict to label by letters
        self.stickman = self.ax.text(0, 0, '😊', fontsize=40, ha='center', va='center')
        self.trail, = self.ax.plot([], [], 'r--', linewidth=1.5, alpha=0.5)
        self.positions = []

        # Side panels
        self.position_ax = self.fig.add_axes([0.7, 0.1, 0.25, 0.08])
        self.position_ax.axis('off')
        self.position_text = self.position_ax.text(0.5, 0.5, 'Graph Pos\nX: 0.00\nY: 0.00', ha='center', va='center', fontsize=12)

        self.graph_ax = self.fig.add_axes([0.1, 0.1, 0.25, 0.08])
        self.graph_ax.axis('off')
        self.graph_text = self.graph_ax.text(0.5, 0.5, 'Grid Coords\nX: 0.00\nY: 0.00', ha='center', va='center', fontsize=12)

        # Ultrasonic sensor
        self.ultra_letter = None
        self.ultra_box_ax = self.fig.add_axes([0.5, 0.15, 0.1, 0.05])
        self.ultra_box = TextBox(self.ultra_box_ax, 'Ultrasonic', initial="")
        self.ultra_box.on_submit(self.set_ultrasonic_letter)

    def update_labels(self, grid_type):
        # Remove old
        for txt in self.label_texts.values():
            txt.remove()
        self.label_texts = {}

        labels = label_sets[grid_type]
        self.label_texts[labels['bottom'][0]] = self.ax.text(0, -0.3, labels['bottom'][0], ha='center', va='center', fontsize=12)
        self.label_texts[labels['bottom'][1]] = self.ax.text(1.5, -0.3, labels['bottom'][1], ha='center', va='center', fontsize=12)
        self.label_texts[labels['bottom'][2]] = self.ax.text(3, -0.3, labels['bottom'][2], ha='center', va='center', fontsize=12)
        self.label_texts[labels['top'][0]] = self.ax.text(0, 4.15, labels['top'][0], ha='center', va='center', fontsize=12)
        self.label_texts[labels['top'][1]] = self.ax.text(1.5, 4.15, labels['top'][1], ha='center', va='center', fontsize=12)
        self.label_texts[labels['top'][2]] = self.ax.text(3, 4.15, labels['top'][2], ha='center', va='center', fontsize=12)
        self.label_texts[labels['left'][0]] = self.ax.text(-0.25, 2, labels['left'][0], va='center', ha='center', fontsize=12)
        self.label_texts[labels['right'][0]] = self.ax.text(3.25, 2, labels['right'][0], va='center', ha='center', fontsize=12)

        self.ax.set_title(f"Mobile Node Live Tracking - Grid {grid_type}", pad=30)


    def set_ultrasonic_letter(self, letter):
        letter = letter.strip().upper()
        if letter in self.label_texts:
            if self.ultra_letter and self.ultra_letter in self.label_texts:
                self.label_texts[self.ultra_letter].set_color('black')

            self.label_texts[letter].set_color('red')
            self.ultra_letter = letter
       

    def update_plot(self, x, y, grid_type):
        self.stickman.set_position((x, y))
        self.positions.append((x, y))
        if len(self.positions) > 5:
            self.positions.pop(0)

        if len(self.positions) > 1:
            self.trail.set_data(*zip(*self.positions))
        else:
            self.trail.set_data([], [])

        if grid_type == 'A':
            display_x = x
            display_y = 4 - y
        elif grid_type == 'B':
            display_x = x + 3
            display_y = 4 - y

        self.position_text.set_text(f"Graph Pos\nX: {x:.2f}\nY: {y:.2f}")
        self.graph_text.set_text(f"Grid Coords\nX: {display_x:.2f}\nY: {display_y:.2f}")

    def get_figure(self):
        return self.fig

# ---------------- Controller ----------------
class TrackingController:
    def __init__(self, simulate=True):
        self.model = TrackingModel(simulate)
        self.view = TrackingView()

        self.view.update_labels(self.model.get_grid())

        self.anim = animation.FuncAnimation(self.view.get_figure(), self.update, interval=100, cache_frame_data=False)

        self.ax_button = self.view.fig.add_axes([0.4, 0.05, 0.2, 0.075])
        self.btn = Button(self.ax_button, 'Switch Grid')
        self.btn.on_clicked(self.toggle_grid)

    def update(self, frame):
        x, y = self.model.get_position()
        self.view.update_plot(x, y, self.model.get_grid())

    def toggle_grid(self, event):
        self.model.toggle_grid()
        self.view.update_labels(self.model.get_grid())

# ---------------- Main ----------------
if __name__ == "__main__":
    app = TrackingController(simulate=False)
    plt.show()