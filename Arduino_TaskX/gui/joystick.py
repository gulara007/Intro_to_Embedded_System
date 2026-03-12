import sys
import serial
import re
from PyQt6.QtWidgets import QApplication, QWidget, QVBoxLayout, QPushButton
from PyQt6.QtCore import QTimer
import pyqtgraph as pg

# ARDUINO PORT
PORT = "COM18"
BAUD = 9600


class JoystickGUI(QWidget):

    def __init__(self):
        super().__init__()

        self.setWindowTitle("Joystick Data Visualization")
        self.resize(700, 400)

        layout = QVBoxLayout()

        # Graph
        self.plot = pg.PlotWidget()
        self.plot.setYRange(0, 1023)
        self.plot.setTitle("Joystick X and Y Values")
        self.plot.addLegend()

        self.x_curve = self.plot.plot(pen='r', name="X Axis")
        self.y_curve = self.plot.plot(pen='b', name="Y Axis")

        layout.addWidget(self.plot)

        # Start/Stop button
        self.button = QPushButton("Start")
        self.button.clicked.connect(self.toggle)

        layout.addWidget(self.button)

        self.setLayout(layout)

        # Serial connection
        self.serial = serial.Serial(PORT, BAUD, timeout=1)

        # Timer
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_plot)

        # Data arrays
        self.x_data = []
        self.y_data = []

    def toggle(self):
        if self.timer.isActive():
            self.timer.stop()
            self.button.setText("Start")
        else:
            self.timer.start(50)
            self.button.setText("Stop")

    def update_plot(self):

        line = self.serial.readline().decode(errors='ignore')

        # Example incoming data: X: 523   Y: 412
        match = re.search(r'X:\s*(\d+)\s*Y:\s*(\d+)', line)

        if match:
            x = int(match.group(1))
            y = int(match.group(2))

            self.x_data.append(x)
            self.y_data.append(y)

            # Keep last 100 values
            if len(self.x_data) > 100:
                self.x_data.pop(0)
                self.y_data.pop(0)

            self.x_curve.setData(self.x_data)
            self.y_curve.setData(self.y_data)


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = JoystickGUI()
    window.show()
    sys.exit(app.exec())