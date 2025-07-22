# HydroFlower Art Deco - IoT Hydroponic System

**Author:** Maximo Regalado  
**Date:** July 17, 2025  
**Course:** IoT Classroom CNM  
**Device:** Particle Photon 2

## Project Overview

The HydroFlower Art Deco is an intelligent hydroponic monitoring and watering system that combines multiple sensors with cloud connectivity to create an automated plant care solution. The system monitors environmental conditions, soil moisture, and water levels while providing visual feedback through NeoPixel LEDs and an OLED display.

## Features

### 🌱 **Automated Plant Care**
- **Smart Watering**: Automatically waters plants when soil moisture drops below threshold (>3000)
- **Remote Control**: Manual watering via Adafruit IO dashboard button
- **Water Level Monitoring**: Tracks reservoir water levels with low-water alerts

### 📊 **Environmental Monitoring**
- **Temperature & Humidity**: BME280 sensor for precise climate monitoring
- **Air Quality**: Real-time air quality assessment with visual indicators
- **Soil Moisture**: Continuous soil moisture monitoring

### 🌈 **Visual Feedback System**
- **NeoPixel Status Indicators**:
  - 🔴 Red: High pollution/Force signal
  - 🟠 Orange: High pollution
  - 🟡 Yellow: Low pollution or low water level (flashing)
  - 🟢 Green: Fresh air
  - 🔵 Blue: Water pump active

### 📱 **IoT Connectivity**
- **Adafruit IO Integration**: Real-time data publishing and remote control
- **Cloud Dashboard**: Monitor all sensors remotely
- **MQTT Communication**: Reliable cloud connectivity

### 📺 **Local Display**
- **OLED Screen**: Shows time, temperature, humidity, and moisture levels
- **Real-time Updates**: Continuous display of system status

## Hardware Components

| Component | Pin | Purpose |
|-----------|-----|---------|
| BME280 Sensor | I2C (0x76) | Temperature, Humidity, Pressure |
| Air Quality Sensor | A0 | Air quality monitoring |
| Soil Moisture Sensor | A1 | Soil moisture measurement |
| Water Level Sensor | A3 | Water reservoir monitoring |
| Water Level Power Pin | D3 | Power control for water sensor |
| Water Pump | D16 | Automated watering |
| NeoPixel Strip | SPI1 | Visual status indicators (15 LEDs) |
| OLED Display | I2C (0x3D) | Local information display |

## Software Architecture

### Libraries Used
- `Adafruit_BME280`: Environmental sensor
- `Adafruit_SSD1306`: OLED display
- `neopixel`: LED strip control
- `Air_Quality_Sensor`: Air quality monitoring
- `Adafruit_MQTT`: Cloud connectivity

### Key Functions
- `MQTT_connect()`: Manages cloud connectivity
- `MQTT_ping()`: Maintains connection health
- `readWaterLevelSensor()`: Power-efficient water level reading

## Adafruit IO Feeds

| Feed Name | Type | Purpose |
|-----------|------|---------|
| `waterbutton` | Subscribe | Remote watering control |
| `temperature` | Publish | Temperature data |
| `humidity` | Publish | Humidity data |
| `moisture` | Publish | Soil moisture levels |
| `airquality` | Publish | Air quality status |
| `waterlevel` | Publish | Water reservoir levels |

## Setup Instructions

### 1. Hardware Assembly
1. Connect all sensors according to the pin diagram
2. Mount the OLED display and NeoPixel strip
3. Install water pump in hydroponic system
4. Place water level sensor in reservoir

### 2. Software Configuration
1. Install required Particle libraries
2. Update `credentials.h` with your Adafruit IO credentials
3. Flash the firmware to your Particle device

### 3. Adafruit IO Dashboard
1. Create feeds for all sensor data
2. Add dashboard widgets for monitoring
3. Create a button widget for remote watering

## Operation Modes

### Automatic Mode
- Continuous sensor monitoring
- Automatic watering when soil is dry
- Real-time data publishing every 30 seconds
- Visual status updates via NeoPixels

### Manual Mode
- Remote watering via Adafruit IO button
- Pump runs for 3 seconds when activated
- Blue LED indicates remote activation

### Alert System
- **Low Water Warning**: Yellow flashing LEDs when water level < 30%
- **Air Quality Alerts**: Color-coded pollution warnings
- **Pump Status**: Blue LED during watering cycles

## Monitoring & Troubleshooting

### Serial Monitor Output
The device provides detailed logging:
```
Time is 14:30:25
Moisture is 2850
Water Level: 450 (86.5%)
Temp: 72.3°F
Humi: 45.2%
```

### Common Issues
- **Sensor Errors**: Check wiring and power connections
- **MQTT Disconnection**: Verify WiFi and internet connectivity
- **No Response**: Check Adafruit IO credentials and feed names

## Power Management
- Water level sensor is powered only during readings to conserve energy
- MQTT connection with 60-second ping intervals
- Optimized delay cycles for responsive operation

## Future Enhancements
- pH sensor integration
- Nutrient level monitoring
- Mobile app development
- Machine learning for predictive watering
- Solar power integration

## Course Integration
This project demonstrates key IoT concepts learned in class:
- Sensor integration and data collection
- Cloud connectivity and MQTT protocols
- Real-time monitoring and control systems
- User interface design (OLED + LEDs)
- Automated control systems
- Power management techniques

## Introduction

For an in-depth understanding of this project template, please refer to our [documentation](https://docs.particle.io/firmware/best-practices/firmware-template/).

## Prerequisites To Use This Repository

To use this software/firmware on a device, you'll need:

- A [Particle Device](https://www.particle.io/devices/).
- Windows/Mac/Linux for building the software and flashing it to a device.
- [Particle Development Tools](https://docs.particle.io/getting-started/developer-tools/developer-tools/) installed and set up on your computer.
- Optionally, a nice cup of tea (and perhaps a biscuit).

## Getting Started

1. While not essential, we recommend running the [device setup process](https://setup.particle.io/) on your Particle device first. This ensures your device's firmware is up-to-date and you have a solid baseline to start from.

2. If you haven't already, open this project in Visual Studio Code (File -> Open Folder). Then [compile and flash](https://docs.particle.io/getting-started/developer-tools/workbench/#cloud-build-and-flash) your device. Ensure your device's USB port is connected to your computer.

3. Verify the device's operation by monitoring its logging output:
    - In Visual Studio Code with the Particle Plugin, open the [command palette](https://docs.particle.io/getting-started/developer-tools/workbench/#particle-commands) and choose "Particle: Serial Monitor".
    - Or, using the Particle CLI, execute:
    ```
    particle serial monitor --follow
    ```

4. Uncomment the code at the bottom of the cpp file in your src directory to publish to the Particle Cloud! Login to console.particle.io to view your devices events in real time.

5. Customize this project! For firmware details, see [Particle firmware](https://docs.particle.io/reference/device-os/api/introduction/getting-started/). For information on the project's directory structure, visit [this link](https://docs.particle.io/firmware/best-practices/firmware-template/#project-overview).

## Particle Firmware At A Glance

### Logging

The firmware includes a [logging library](https://docs.particle.io/reference/device-os/api/logging/logger-class/). You can display messages at different levels and filter them:


```
