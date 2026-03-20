import sys      # Used to access system functions like app execution and exit
import time     # Used for delay
import serial   # Used for serial communication with Arduino

# Import required PyQt6 widgets for building the GUI
from PyQt6.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QLabel, QFrame, QProgressBar
)
from PyQt6.QtCore import QTimer, Qt   # QTimer for periodic updates, Qt for alignment
from PyQt6.QtGui import QFont         # Used to set font style and size

# Open serial connection to Arduino on COM12 with baud rate 115200
ser = serial.Serial("COM12", 115200, timeout=1)

# Wait 2 seconds because Arduino often resets when serial port opens
time.sleep(2)

# Main GUI class
class SerialGUI(QWidget):
    def __init__(self):  # Constructor function for the window
        super().__init__()  # Initialize the parent QWidget class

        # Set window title and size
        self.setWindowTitle("Lab 5 Sound Monitor")
        self.resize(500, 420)

        # Create a vertical layout for stacking widgets
        layout = QVBoxLayout()
        layout.setSpacing(15)  # Space between widgets
        layout.setContentsMargins(20, 20, 20, 20)  # Window margins

        # Create the main title label
        title = QLabel("Sound Sensor Monitor")
        title.setFont(QFont("Arial", 18, QFont.Weight.Bold))
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(title)  # Add title to layout

        # Labels to display data received from Arduino
        self.level_label = QLabel("Level: -")
        self.threshold_label = QLabel("Threshold: -")
        self.events_label = QLabel("Events: -")
        self.db_label = QLabel("dB: -")

        # Put all labels in the same style and add them to layout
        for lbl in [
            self.level_label,
            self.threshold_label,
            self.events_label,
            self.db_label
        ]:
            lbl.setFont(QFont("Arial", 13))
            layout.addWidget(lbl)

        # Label above the progress bar
        self.bar_title = QLabel("Live Sound Level")
        self.bar_title.setFont(QFont("Arial", 13, QFont.Weight.Bold))
        layout.addWidget(self.bar_title)

        # Progress bar to show sound level visually
        self.level_bar = QProgressBar()
        self.level_bar.setMinimum(0)       # Minimum value of bar
        self.level_bar.setMaximum(10)      # Maximum value of bar
        self.level_bar.setValue(0)         # Initial value
        self.level_bar.setFormat("%v")     # Show numeric value inside bar
        self.level_bar.setMinimumHeight(28)
        layout.addWidget(self.level_bar)

        # Status box to show whether sound is QUIET or LOUD
        self.status_box = QLabel("QUIET")
        self.status_box.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.status_box.setFont(QFont("Arial", 20, QFont.Weight.Bold))
        self.status_box.setFrameShape(QFrame.Shape.Box)
        self.status_box.setMinimumHeight(90)
        self.status_box.setStyleSheet(
            "background-color: lightgreen; color: black; border: 2px solid black;"
        )
        layout.addWidget(self.status_box)

        # Attach the layout to the window
        self.setLayout(layout)

        # Create a timer to check serial data every 100 ms
        self.timer = QTimer()
        self.timer.timeout.connect(self.read_serial)  # Run read_serial repeatedly
        self.timer.start(100)

    # Function to read and process incoming serial data
    def read_serial(self):
        # Check if there is unread data available in serial buffer
        if ser.in_waiting > 0:
            # Read one full line, decode bytes to text, remove spaces/newline
            line = ser.readline().decode(errors="ignore").strip()

            if line:
                # Split Arduino message by commas
                parts = line.split(",")

                # Expected format:
                # Level=..., dB=..., Threshold=..., Above=..., Events=...
                if len(parts) == 5:
                    level = parts[0].strip()
                    db = parts[1].strip()
                    threshold = parts[2].strip()
                    above = parts[3].strip()
                    events = parts[4].strip()

                    # Update text labels on GUI
                    self.level_label.setText(level)
                    self.threshold_label.setText(threshold)
                    self.events_label.setText(events)
                    self.db_label.setText(db)

                    try:
                        # Extract numeric part from strings like "Level=8"
                        level_value = int(level.split("=")[1])
                        threshold_value = int(threshold.split("=")[1])

                        # Limit level value to progress bar range
                        if level_value < 0:
                            level_value = 0
                        if level_value > 10:
                            level_value = 10

                        # Update progress bar value
                        self.level_bar.setValue(level_value)

                        # Change bar color depending on threshold
                        if level_value >= threshold_value:
                            self.level_bar.setStyleSheet("""
                                QProgressBar {
                                    border: 2px solid gray;
                                    border-radius: 5px;
                                    text-align: center;
                                }
                                QProgressBar::chunk {
                                    background-color: red;
                                }
                            """)
                        else:
                            self.level_bar.setStyleSheet("""
                                QProgressBar {
                                    border: 2px solid gray;
                                    border-radius: 5px;
                                    text-align: center;
                                }
                                QProgressBar::chunk {
                                    background-color: lightgreen;
                                }
                            """)
                    except:
                        # Ignore any conversion errors if serial data is not complete
                        pass

                    # Update large status box depending on Above value
                    if "Above=1" in above:
                        self.status_box.setText("LOUD")
                        self.status_box.setStyleSheet(
                            "background-color: red; color: white; border: 2px solid black;"
                        )
                    else:
                        self.status_box.setText("QUIET")
                        self.status_box.setStyleSheet(
                            "background-color: lightgreen; color: black; border: 2px solid black;"
                        )

    # This function runs when user closes the window
    def closeEvent(self, event):
        self.timer.stop()  # Stop timer
        if ser.is_open:
            ser.close()    # Close serial port safely
        event.accept()     # Accept close event

# Create the PyQt application
app = QApplication(sys.argv)

# Create and show the window
window = SerialGUI()
window.show()

# Start the application event loop
sys.exit(app.exec())
