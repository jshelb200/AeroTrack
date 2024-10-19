# AeroTrack - IoT Optimized Energy Monitoring System

AeroTrack is an IoT project designed to monitor environmental conditions while minimizing energy consumption. The system uses an ESP32, sensors, and cloud services to gather data and trigger alerts when anomalies are detected. The focus of the project is on energy optimization through the use of light sleep modes and careful scheduling of tasks.
Features

## 1. Energy Optimization: Light Sleep Mode

The system enters light sleep between measurements to conserve energy, except during critical tasks like smoke detection.
## 2. Task Frequencies

    Location Detection: Runs only once at startup, using the device's IP to identify the location.
    WeatherStack Data Retrieval: Every 30 minutes, to update external weather conditions.
    Smoke Detection: Runs every 2 seconds due to its high sensitivity.
    SMS Alerts: Sent upon detection of anomalies, such as differences in indoor/outdoor humidity or smoke.
    Data Publishing to Dashboard: Every 1 minute, and also triggered by an SMS alert.

## 3. Data Sources & Sensors

    Adafruit BME680: Measures temperature, humidity, pressure, and gas levels.
    LDR Sensor: Detects light and potential smoke.
    WeatherStack API: Retrieves real-time weather data for comparison with indoor conditions.
    Twilio SMS API: Sends alerts when conditions exceed predefined thresholds.

Future Improvements
Energy Efficiency

    Implement deep sleep mode for the ESP32, and compare energy savings between light and deep sleep.
    Explore low-power communication protocols like Zigbee.

Air Quality Monitoring

    Extend functionality to detect other harmful gases for better air quality analysis.

# Installation

##     ESP32 Setup:
        Flash the ESP32 with the aero_track.ino sketch using the Arduino IDE.
        Install necessary libraries:
            Adafruit_BME680
            ESP32Servo
            PubSubClient (for MQTT)
            HTTPClient (for WeatherStack API)

##    Node-RED Flow:
        Import the aeotrack_flow.json into Node-RED.
        Configure your MQTT broker and start monitoring data in real-time through the dashboard.

    API Setup:
        Insert your WeatherStack API key and Twilio credentials into the sketch for real-time weather data and SMS alerts.

## How It Works

    The ESP32 monitors indoor conditions via the BME680 sensor and compares them with data from WeatherStack.
    The system conserves energy by using light sleep between tasks, waking only to take measurements or send alerts.
    If anomalies like smoke or significant humidity differences are detected, an SMS alert is triggered, and the data is sent to the Node-RED dashboard.
