# SmartHomeIoT
Smart Home System with ESP32 and Secure MQTT Integration


# **Overview:**
This project implements a smart home environment using the ESP32 microcontroller. It integrates multiple modules to provide automation, monitoring, control, and secure cloud connectivity. Key functionalities include:

- **Wi-Fi and Secure MQTT Integration**: Uses HiveMQ Cloud as the MQTT broker with TLS encryption for secure communication.

- **IR Remote Control**: Leverages the IRremoteESP8266 library to receive remote commands and trigger actions.

- **RGB LED Control**: Provides visual feedback based on system status or remote input.

- **Melody Playback**: Plays tone-based melodies through a piezo buzzer for alerts or notifications.

- **Temperature & Humidity Sensing**: Uses the DHT22 sensor for environmental monitoring.

- **Ambient Light Monitoring**: Implements an LDR (Light Dependent Resistor) to detect light intensity.

- **RFID Authentication**: Uses the MFRC522 module for access control or user identification.

- **Servo Motor Control**: Drives an SG90 servo for physical actions such as door movement.

- **MQTT Data Publishing**: Sends sensor data in JSON format to cloud topics over TLS-secured MQTT.

**Technologies Used:**

- Hardware: ESP32, DHT22, LDR, SG90 Servo, MFRC522 RFID, IR Receiver, RGB LED, Piezo Buzzer

- Software: Arduino Framework, HiveMQ Cloud MQTT Broker

**Libraries:**

- ArduinoJson for JSON serialization

- PubSubClient for MQTT communication

- WiFiClientSecure for TLS support

- IRremoteESP8266 for IR remote handling

**Wiring diagram**
![image](https://github.com/user-attachments/assets/9d32559f-37fa-400c-99b3-58c323e42b71)
