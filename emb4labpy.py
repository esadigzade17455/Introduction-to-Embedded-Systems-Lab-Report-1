import sys
import serial  # Enables UART communication between PC and Arduino
from PyQt6.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QLabel, QPushButton
)
from PyQt6.QtCore import QTimer  # Used for periodic execution (non-blocking loop)
import pyqtgraph as pg  # High-performance plotting library for real-time visualization

class JoystickGUI(QWidget):
    def __init__(self):
        super().__init__()
        # --- Window configuration ---
        # Sets GUI title and size
        self.setWindowTitle("Joystick GUI Tester")
        self.setGeometry(200, 100, 700, 600)
        # Serial object (initially not connected)
        # Will store UART connection to Arduino
        self.serial_port = None
        # --- Data display labels ---
        # These labels show real-time values received from Arduino
        self.lbl_x = QLabel("X Level: ---")        # Raw ADC value (0–1023)
        self.lbl_y = QLabel("Y Level: ---")        # Raw ADC value (0–1023)
        self.lbl_btn = QLabel("Button: ---")       # Button state (pressed/released)
        self.lbl_rate = QLabel("Sample Rate: --- Hz")  # Calculated frequency
        # --- Connect button ---
        # Allows user to establish serial communication manually
        self.btn_connect = QPushButton("Connect")
        self.btn_connect.clicked.connect(self.connect_serial)
        # --- Graph setup ---
        # Creates a 2D plane representing joystick movement
        self.plot_widget = pg.PlotWidget()
        # Set graph range to match ADC resolution
        self.plot_widget.setXRange(0, 1023)
        self.plot_widget.setYRange(0, 1023)
        # Label axes for clarity
        self.plot_widget.setLabel("left", "Y Value")
        self.plot_widget.setLabel("bottom", "X Value")
        # Enable grid for easier visualization
        self.plot_widget.showGrid(x=True, y=True)
        # Dot that represents joystick position
        # Initially placed at center (512,512)
        self.joystick_dot = self.plot_widget.plot(
            [512], [512],
            pen=None,
            symbol='o',
            symbolSize=12
        )
        # --- Layout ---
        # Vertical layout organizes all widgets neatly
        layout = QVBoxLayout()
        layout.addWidget(self.lbl_x)
        layout.addWidget(self.lbl_y)
        layout.addWidget(self.lbl_btn)
        layout.addWidget(self.lbl_rate)
        layout.addWidget(self.btn_connect)
        layout.addWidget(self.plot_widget)
        self.setLayout(layout)
        # --- Timer (core of real-time GUI) ---
        # Replaces traditional loop (non-blocking approach)
        # Calls read_serial() every 20 ms (~50 Hz update rate)
        self.timer = QTimer()
        self.timer.timeout.connect(self.read_serial)
        self.timer.start(20)
    def connect_serial(self):
        # Attempts to open serial connection with Arduino
        try:
            # COM port must match Arduino port (change if needed)
            self.serial_port = serial.Serial('COM3', 115200, timeout=1)
            # If successful, update button text
            self.btn_connect.setText("Connected")
        except Exception:
            # If connection fails (wrong port, not connected, etc.)
            self.btn_connect.setText("Connection Failed")
    def read_serial(self):
        # Reads incoming UART data continuously
        # Called periodically by QTimer (non-blocking execution)
        if self.serial_port and self.serial_port.is_open:
            try:
                # Check if there is unread data in buffer
                if self.serial_port.in_waiting > 0:
                    # Read one line (ends with newline '\n')
                    # Decode from bytes → string
                    line = self.serial_port.readline().decode('utf-8').strip()
                    # Split CSV format into values
                    data = line.split(',')
                    # Expected format: X,Y,SW,LoopTime
                    if len(data) == 4:
                        # --- Parse incoming data ---
                        x_val = int(data[0])       # X-axis position
                        y_val = int(data[1])       # Y-axis position
                        sw_val = int(data[2])      # Button state (0/1)
                        loop_time_us = int(data[3])  # Loop time in microseconds
                        # --- Sampling rate calculation ---
                        # Sampling rate = 1 / loop time
                        # Convert microseconds → Hz
                        sample_rate = 1000000 / loop_time_us if loop_time_us > 0 else 0
                        # --- Update labels ---
                        # Provides real-time numerical feedback
                        self.lbl_x.setText(f"X Level: {x_val}")
                        self.lbl_y.setText(f"Y Level: {y_val}")
                        # Button uses INPUT_PULLUP → inverted logic
                        btn_state = "PRESSED" if sw_val == 0 else "Released"
                        self.lbl_btn.setText(f"Button: {btn_state}")
                        self.lbl_rate.setText(f"Sample Rate: {int(sample_rate)} Hz")
                        # --- Update graph ---
                        # Moves the dot according to joystick position
                        # This gives intuitive visual feedback
                        self.joystick_dot.setData([x_val], [y_val])
            except Exception:
                # Handles corrupted or incomplete serial data
                # Prevents GUI from crashing due to bad input
                pass
# --- Main entry point ---
if __name__ == '__main__':
    # Create application object
    app = QApplication(sys.argv)
    # Create and display GUI window
    window = JoystickGUI()
    window.show()
    # Start event loop (required for GUI to run)
    sys.exit(app.exec())
