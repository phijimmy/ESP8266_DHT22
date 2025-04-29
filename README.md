# ESP8266_DHT22
ESP8266 + DHT22 Sensor with Web UI, NTP, WiFiManager, Persistent Config, and MQTT Home Assistant Integration

**Author:** Jim ([M1Musik.com](http://m1musik.com))  
**Hardware:** ESP8266 with DHT22 Sensor

## Overview

This project utilizes an **ESP8266 microcontroller** paired with a **DHT22 sensor** to monitor climate conditions. It includes a web-based user interface, NTP synchronization for accurate timekeeping (accounting for Germany's DST), persistent configuration storage using LittleFS, and seamless integration with Home Assistant via MQTT.

### Key Features
- **Climate Monitoring**:
  - Records temperature, humidity, and heat index.
  - Displays data via web interface and MQTT.
- **Web Interface**:
  - Allows real-time status viewing and configuration.
- **WiFiManager Integration**:
  - Simplifies WiFi setup and management.
- **MQTT Home Assistant Integration**:
  - Automatic discovery and integration with Home Assistant.
- **Persistent Configuration**:
  - Saves settings securely in LittleFS.
- **NTP Time Synchronization**:
  - Keeps accurate time with local timezone and DST adjustments.

## Hardware Requirements
- **Microcontroller**: ESP8266
- **Sensor**: DHT22 for climate data
- **WiFi Access Point**: For device setup and operation
- **MQTT Broker**: For data transmission and Home Assistant discovery

## Software Requirements
- **Arduino IDE** (or equivalent ESP8266 development environment)
- Libraries:
  - [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
  - [ESPAsyncWiFiManager](https://github.com/alanswx/ESPAsyncWiFiManager)
  - [LittleFS](https://github.com/esp8266/Arduino)
  - [ArduinoJson](https://arduinojson.org/)
  - [DHT Sensor Library](https://github.com/adafruit/DHT-sensor-library)
  - [AsyncMqttClient](https://github.com/marvinroger/async-mqtt-client)

## Configuration Parameters
- **WiFi**: Configurable SSID and password.
- **Sensor**:
  - GPIO pin for DHT22 connection.
- **MQTT**:
  - Server, username, and password setup.
- **NTP**:
  - Servers for time synchronization.
  - Timezone: `CET-1CEST,M3.5.0,M10.5.0/3`.

## Setup Instructions
1. **Flash the ESP8266**:
   - Open the sketch in Arduino IDE.
   - Install required libraries.
   - Flash the firmware onto your ESP8266.

2. **WiFi Configuration**:
   - On first boot or reset, connect to the configuration portal.
   - Enter your WiFi credentials and configuration parameters.

3. **Sensor Initialization**:
   - Ensure your DHT22 sensor is connected to the correct GPIO pin.
   - Verify data is being captured and displayed.

4. **Home Assistant Discovery**:
   - The device will publish its MQTT configuration for automatic discovery.

## Usage
- Access the web interface via the ESP8266's local IP address.
- View climate data and control configurations dynamically.
- Monitor and manage data seamlessly with Home Assistant.

## Troubleshooting
- If WiFi is lost, the ESP8266 will automatically reboot and attempt reconnection.
- Use the "Deep Reset" function for full configuration reset.

## Future Improvements
- Support for additional sensor types.
- Enhanced data visualization on the web interface.

## License
This project is released under the **MIT License**.

---

Feel free to adjust or expand this based on your project's specifics. Let me know if there’s anything else you'd like to include or if you want additional support! 😊
