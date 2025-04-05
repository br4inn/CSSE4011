from PyQt5.QtWidgets import QMainWindow, QVBoxLayout, QWidget, QAction, QFileDialog, QMessageBox
from PyQt5.QtCore import Qt
from .graph_widget import GraphWidget
import json

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Sensor Data Time-Series Graph")
        self.setGeometry(100, 100, 800, 600)

        self.graph_widget = GraphWidget()
        self.setCentralWidget(self.graph_widget)

        self.create_menu()

    def create_menu(self):
        menu_bar = self.menuBar()
        file_menu = menu_bar.addMenu("File")

        load_action = QAction("Load JSON Data", self)
        load_action.triggered.connect(self.load_json_data)
        file_menu.addAction(load_action)

        exit_action = QAction("Exit", self)
        exit_action.triggered.connect(self.close)
        file_menu.addAction(exit_action)

    def load_json_data(self):
        options = QFileDialog.Options()
        file_name, _ = QFileDialog.getOpenFileName(self, "Load JSON File", "", "JSON Files (*.json);;All Files (*)", options=options)
        if file_name:
            try:
                with open(file_name, 'r') as file:
                    data = json.load(file)
                    self.graph_widget.update_graph(data)
            except Exception as e:
                QMessageBox.critical(self, "Error", f"Failed to load JSON data: {str(e)}")