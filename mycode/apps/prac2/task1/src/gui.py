import threading
import socket
import serial
import json
from PyQt5 import QtCore, QtWidgets
from PyQt5.QtWidgets import QPushButton, QLabel
import pyqtgraph as pg
from PyQt5.QtCore import pyqtSlot
from datetime import datetime
import sys

DATA_POINTS = 50

def pA_connect() -> socket.socket | None:
    try:
        pA_socket = socket.socket()
        host = "localhost"
        port = 19021
        pA_socket.connect((host, port))
        return pA_socket
    except Exception:
        print("couldn't connect", flush=True)
    
def pA_disconnect(pA_socket: socket.socket):
    pA_socket.close()

def pB_connect() -> serial.Serial | None:
    try:
        pb_ser = serial.Serial('/dev/ttyACM0', 115200)
        return pb_ser
    except Exception:
        print("couldn't connect")

def pB_disconnect(pb_ser: serial.Serial):
    pb_ser.close()


def pC_connect() -> serial.Serial | None:
    try:
        pc_ser = serial.Serial('/dev/ttyACM0', 115200)
        return pc_ser
    except Exception:
        print("couldn't connect")

def pC_disconnect(pc_ser: serial.Serial):
    pc_ser.close()

times = [[] for i in range(6)]
data_lists = [[] for i in range(6)]
data_lists[4] = [[] for i in range(3)]

def pA_receive(pA_socket: socket.socket):
    while True:
        line = ""
        try:
            line = pA_socket.recv(1024).decode()
        except Exception:
                sys.exit()
        
        lines = line.split('\n')
        
        for i in range(len(lines)):
            end = lines[i].find("\"}")
            if end != -1:
                start = lines[i].find("{\"DID")
                if start != -1:
                    valid_line = lines[i][start:end+2]
                    jsonOutput = json.loads(valid_line.strip())
                    #print(jsonOutput, flush=True)

                    cur = jsonOutput["time"].split(":")
                    date = datetime.today()
                    stamp = datetime(date.year, date.month, date.day, \
                            hour=int(cur[0]), minute=int(cur[1]), second=int(cur[2])).timestamp()

                    did = jsonOutput.get("DID")
                    if did < 15:
                        times[did].append(stamp)
                        value = float(jsonOutput["value"].split()[0])
                        data_lists[did].append(value)
                    
                    elif did == 15:
                        for j in range(4):
                            times[j].append(stamp)
                        keys = ["temp", "hum", "press", "gas"]
                        for i in range(len(keys)):
                            data_lists[i].append(float(jsonOutput[keys[i]].split()[0]))
        
        for i in range(len(data_lists)):
            if len(data_lists[i]) > DATA_POINTS:
                sliced = data_lists[i][(len(data_lists[i])-DATA_POINTS-1):]
                data_lists[i] = sliced
                times[i] = times[i][len(times[i])-DATA_POINTS-1:]

        # print(flush=True)
        # print("time:", times[3], flush=True)
        # print("temp:", data_lists[0], flush=True)
        # print("hum:", data_lists[1], flush=True)
        # print("press:", data_lists[2], flush=True)
        # print("gas:", data_lists[3], flush=True)

def pA_send(pA_socket: socket.socket):
    # initial = "sample s 15\n"
    # s.send(initial.encode())
    while True:
        msg = input()
        msg += '\n'
        try:
            pA_socket.send(msg.encode())
        except Exception:
            sys.exit()

def pB_receive(pb_ser: serial.Serial):
    pb_ser.timeout = 0.2
    while True:
        try:
            line = pb_ser.read(1024).decode().strip()
            if len(line) == 0:
                continue
        except Exception:
            sys.exit()
        
        lines = line.split('\n')
        
        for i in range(len(lines)):
            end = lines[i].find("\"}")
            if end != -1:
                start = lines[i].find("{\"DID")
                if start != -1:
                    valid_line = lines[i][start:end+2]
                    jsonOutput = json.loads(valid_line.strip())
                    #print(jsonOutput, flush=True)

                    cur = jsonOutput["time"].split()
                    curDate = cur[0].split("-")
                    curTime = cur[1].split(":")
                    stamp = datetime(year=int(curDate[0]), month=int(curDate[1]), day=int(curDate[0]), \
                            hour=int(curTime[0]), minute=int(curTime[1]), second=int(curTime[2])).timestamp()

                    did = jsonOutput.get("DID")
                    if did < 6:
                        times[did].append(stamp)
                        value = float(jsonOutput["value"])
                        data_lists[did].append(value)
                    
                    elif did == 15:
                        for j in range(3):
                            times[j].append(stamp)
                        keys = ["temp", "hum", "press"]
                        for i in range(len(keys)):
                            data_lists[i].append(float(jsonOutput[keys[i]]))
        
        for i in range(len(data_lists)):
            if len(data_lists[i]) > DATA_POINTS:
                sliced = data_lists[i][(len(data_lists[i])-DATA_POINTS-1):]
                data_lists[i] = sliced
                times[i] = times[i][len(times[i])-DATA_POINTS-1:]

def pB_send(pb_ser: serial.Serial):
    while True:
        msg = input()
        msg += '\n'
        try:
            pb_ser.write(msg.encode())
        except Exception:
            sys.exit()

def pC_receive(pc_ser: serial.Serial):
    pc_ser.timeout = 0.2
    while True:
        try:
            line = pc_ser.read(1024).decode().strip()
            if not line:
                continue
             
            lines = line.split('\n')
            for raw_line in lines: 
                if not raw_line.startswith("{\"did"):
                    continue
                
                try:
                    jsonOutput = json.loads(raw_line.strip())
                    print(f"Parsed JSON: {jsonOutput}", flush=True)
                     
                    cur = jsonOutput["rtc_time"].split()
                    curDate = cur[0].split("-")
                    curTime = cur[1].split(":")
                    stamp = datetime(year=int(curDate[0]), month=int(curDate[1]), day=int(curDate[2]), \
                            hour=int(curTime[0]), minute=int(curTime[1]), second=int(curTime[2])).timestamp()

                    did = int(jsonOutput.get("did"))
                    if did < 4: 
                        times[did].append(stamp)
                        value = int(jsonOutput["value"])
                        data_lists[did].append(value)
                    
                    elif did == 4: 
                        times[4].append(stamp)
                        mag_x, mag_y, mag_z = float(jsonOutput["x"]), float(jsonOutput["y"]), float(jsonOutput["z"])
                        data_lists[4][0].append(mag_x)
                        data_lists[4][1].append(mag_y)
                        data_lists[4][2].append(mag_z)

                    elif did == 15: 
                        for j in range(3):
                            times[j].append(stamp)
                        keys = ["temp_value", "hum_value", "press_value"]
                        for i in range(len(keys)):
                            data_lists[i].append(float(jsonOutput[keys[i]]))
                        
                        times[4].append(stamp)
                        mag_x, mag_y, mag_z = float(jsonOutput["mag_x"]), float(jsonOutput["mag_y"]), float(jsonOutput["mag_z"])
                        data_lists[4][0].append(mag_x)
                        data_lists[4][1].append(mag_y)
                        data_lists[4][2].append(mag_z)
                except json.JSONDecodeError as e:
                    print(f"JSON decode error: {e}", flush=True)
                    print(f"Invalid JSON line: {raw_line}", flush=True)
        except Exception as e:
            print(f"Error: {e}", flush=True)
            sys.exit()

def pC_send(pc_ser: serial.Serial):
    while True:
        msg = input()
        msg += '\n'
        try:
            pc_ser.write(msg.encode())
        except Exception:
            sys.exit()



class Graph(QtWidgets.QWidget):
    def __init__(self, did: int, plot: pg.PlotWidget):
        super().__init__()
        self.did = did
        self.plot = plot
        self.line = [[], [], []]

        self.plot.setLabel("bottom", "Time (HH:MM:SS)")
        if did == 0:
            self.plot.setLabel("left", "Temperature (C)")
            self.plot.setTitle("Temperature")
        elif did == 1:
            self.plot.setLabel("left", "Humidity (%)")
            self.plot.setTitle("Humidity")
        elif did == 2:
            self.plot.setLabel("left", "Pressure (kPa)")
            self.plot.setTitle("Pressure")
        elif did == 3:
            self.plot.setLabel("left", "Gas (ppb eTVOC)")
            self.plot.setTitle("Gas")
        elif did == 4:
            self.plot.setLabel("left", "Magnetometer")
            self.plot.setTitle("Magnetometer")
            self.plot.addLegend()
        elif did == 5:
            self.plot.setLabel("left", "Light")
            self.plot.setTitle("Light")

        if did < 4:
            self.line[0] = self.plot.plot(times[did], data_lists[did])
        elif did == 4:
            line_colour = pg.mkPen(color=(255, 0, 0))
            self.line[0] = self.plot.plot(
                times[did],
                data_lists[4][0],
                name="Mag x",
                pen=line_colour)
            
            line_colour = pg.mkPen(color=(0, 255, 0))
            self.line[1] = self.plot.plot(
                times[did],
                data_lists[4][0],
                name="Mag y",
                pen=line_colour)
            
            line_colour = pg.mkPen(color=(0, 0, 255))
            self.line[2] = self.plot.plot(
                times[did],
                data_lists[4][0],
                name="Mag z",
                pen=line_colour)
        #self.plot.setYRange(0, 5)

    def get_line(self):
        return self.line

class MainWindow(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Sensors Data")
        #self.resize(800, 600)
        self.connected = [False, False, False]
        self.pA_socket = socket.socket()
        self.pb_ser = serial.Serial()
        self.pc_ser = serial.Serial()

        self.pA_button = QPushButton("Connect A")
        self.pB_button = QPushButton("Connect B")
        self.pC_button = QPushButton("Connect C")

        self.pA_button.clicked.connect(self.pA_click)
        self.pB_button.clicked.connect(self.pB_click)
        self.pC_button.clicked.connect(self.pC_click)

        button_layout = QtWidgets.QHBoxLayout()
        button_layout.addWidget(self.pA_button)
        button_layout.addWidget(self.pB_button)
        button_layout.addWidget(self.pC_button)
        button_layout.setSpacing(300)

        self.pA_label = QLabel("A Disconnected")
        self.pB_label = QLabel("B Disconnected")
        self.pC_label = QLabel("C Disconnected")

        label_layout = QtWidgets.QHBoxLayout()
        label_layout.addWidget(self.pA_label)
        label_layout.addWidget(self.pB_label)
        label_layout.addWidget(self.pC_label)
        label_layout.setSpacing(300)

        bottom_axis0 = pg.DateAxisItem(orientation='bottom')
        bottom_axis1 = pg.DateAxisItem(orientation='bottom')
        bottom_axis2 = pg.DateAxisItem(orientation='bottom')
        bottom_axis3 = pg.DateAxisItem(orientation='bottom')
        bottom_axis4 = pg.DateAxisItem(orientation='bottom')
        # bottom_axis5 = pg.DateAxisItem(orientation='bottom')

        self.p0 = pg.PlotWidget(axisItems={'bottom': bottom_axis0})
        self.p1 = pg.PlotWidget(axisItems={'bottom': bottom_axis1})
        self.p2 = pg.PlotWidget(axisItems={'bottom': bottom_axis2})
        self.p3 = pg.PlotWidget(axisItems={'bottom': bottom_axis3})
        self.p4 = pg.PlotWidget(axisItems={'bottom': bottom_axis4})
        # self.p5 = pg.PlotWidget(axisItems={'bottom': bottom_axis5})
        # self.p = [self.p0, self.p1, self.p2, self.p3, self.p4, self.p5]
        self.p = [self.p0, self.p1, self.p2, self.p3, self.p4]

        self.temp_plot = Graph(0, self.p0)
        self.hum_plot = Graph(1, self.p1)
        self.press_plot = Graph(2, self.p2)
        self.gas_plot = Graph(3, self.p3)
        self.mag_plot = Graph(4, self.p4)
        # self.light_plot = Graph(5, self.p5)
        # self.plots = [self.temp_plot, self.hum_plot, self.press_plot, self.gas_plot, \
        #               self.mag_plot, self.light_plot]
        self.plots = [self.temp_plot, self.hum_plot, self.press_plot, self.gas_plot, \
                      self.mag_plot]

        plot_layout1 = QtWidgets.QHBoxLayout()
        plot_layout1.addWidget(self.p0)
        plot_layout1.addWidget(self.p1)
        plot_layout1.addWidget(self.p2)

        plot_layout2 = QtWidgets.QHBoxLayout()
        plot_layout2.addWidget(self.p3)
        plot_layout2.addWidget(self.p4)
        # plot_layout2.addWidget(self.p5)

        overall_layout = QtWidgets.QVBoxLayout()
        overall_layout.addLayout(button_layout)
        overall_layout.addLayout(label_layout)
        overall_layout.addLayout(plot_layout1)
        overall_layout.addLayout(plot_layout2)

        widget = QtWidgets.QWidget()
        widget.setLayout(overall_layout)

        widget.layout
        self.setCentralWidget(widget)

        self.data_max = [5, 5, 5, 5, 5, 5]

        self.timer = QtCore.QTimer()
        self.timer.setInterval(300)
        self.timer.timeout.connect(self.update_plot)
        self.timer.start()
    
    def update_plot(self):
        for did in range(5):
            # if len(data_lists[did]) != 0:
            #     top = max(data_lists[did])
            #     if top > self.data_max[did]:
            #         self.data_max[did] = top
            if did == 4:
                self.plots[did].get_line()[0].setData(times[4], data_lists[4][0])
                self.plots[did].get_line()[1].setData(times[4], data_lists[4][1])
                self.plots[did].get_line()[2].setData(times[4], data_lists[4][2])
            else:
                self.plots[did].get_line()[0].setData(times[did], data_lists[did])
            #self.p[did].setYRange(0, self.data_max[did])
    
    def pA_click(self):
        print('pA button click')

        if self.connected[0] is True:
            pA_disconnect(self.pA_socket)
            self.pA_label.setText("A Disconnected")
            self.connected[0] = False
            self.pA_button.setText("Connect A")
            return

        self.pA_socket = pA_connect()
        if self.pA_socket == None or self.pA_socket.getsockname()[1] == 0:
            self.pA_label.setText("Error")
            self.connected[0] = False
            return
        
        self.connected[0] = True
        self.pA_label.setText("A Connected")
        self.pA_button.setText("Disconnect A")
        threading.Thread(target=pA_receive, args=[self.pA_socket]).start()
        threading.Thread(target=pA_send, args=[self.pA_socket]).start()
    
    def pB_click(self):
        print('pB button click')
        self.pB_label.setText("B Connected")

        if self.connected[1] is True:
            pB_disconnect(self.pb_ser)
            self.pB_label.setText("B Disconnected")
            self.connected[1] = False
            self.pB_button.setText("Connect B")
            return

        self.pb_ser = pB_connect()
        if self.pb_ser == None or self.pb_ser.name == None:
            print("couldn't connect")
            self.pB_label.setText("Error")
            self.connected[1] = False
            return
        
        self.connected[1] = True
        self.pB_label.setText("B Connected")
        self.pB_button.setText("Disconnect B")
        threading.Thread(target=pB_receive, args=[self.pb_ser]).start()
        threading.Thread(target=pB_send, args=[self.pb_ser]).start()
    
    def pC_click(self):
        print('pC button click')
        self.pC_label.setText("C Connected")

        if self.connected[2] is True:
            pC_disconnect(self.pc_ser)
            self.pC_label.setText("C Disconnected")
            self.connected[2] = False
            self.pC_button.setText("Connect C")
            return

        self.pc_ser = pC_connect()
        if self.pc_ser == None or self.pc_ser.name == None:
            print("couldn't connect")
            self.pC_label.setText("Error")
            self.connected[2] = False
            return
        
        self.connected[2] = True
        self.pC_label.setText("C Connected")
        self.pC_button.setText("Disconnect C")
        threading.Thread(target=pC_receive, args=[self.pc_ser]).start()
        threading.Thread(target=pC_send, args=[self.pc_ser]).start()

app = QtWidgets.QApplication([])
main = MainWindow()
main.show()
app.exec()