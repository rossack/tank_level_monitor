# Pico W Tank Monitor

## Overview

The **Pico W Tank Monitor** is a smart IoT device that monitors liquid levels in a tank using a **4-20mA pressure transducer** and an **analog-to-digital converter (ADC)**. The system is built on a **Raspberry Pi Pico W**, which connects to a network via Wi-Fi and publishes sensor data using **MQTT**.

## Features

- Reads liquid level using a **4-20mA pressure transducer**.
- Converts the analog signal to digital using 0-3.3v input **ADC0 of the Pico**.
- Connects to **Wi-Fi** and operates as a client or **Wireless Access Point (AP)** for setup.
- Publishes data to an **MQTT broker** at configurable intervals.
- Web pages for level display, status and configuration.
- Reset button to erase settings and go back to initial configuration.

## Build Versions

### v0.1.4 - Bug/Feature Release

- Added Wifi Scan to config.html page, useful when setting up in AP mode
- Note: cyw43_wifi_scan() works in both AP and STA modes!
- mqtt_update() doesnt flash the LED anymore - heartbeat is more useful feedback

### v0.1.3 - Bug/Feature Release

- Fixed random wireless dissassociate by turning off PM
- Added watchdog in case WiFi is lost
- Moved the radio functions into wifi wrapper class

### v0.1.2 - Bug/Feature Release

- I forget :-()

### v0.0.1 - Initial Release

- Basic functionality with WiFi and MQTT support
- Reads and transmits tank level data
- Manual configuration via Web page

## Todo List

- When doing a web form factory reset send a "disconnected" html response
- Do a WiFi scan to list AP's during setup: done v0.1.4
- Do a build using interrupt driven LWIP
- Upgrade to latest Pico SDK (Current: 1.5.1 - instability(?) issues with 2.0+)
- Checksum verify Flash memory
- Make settings a C++ Singleton Class?
- Need to fix Build Kits (Release, Debug. Arch: Poll, Threadsafe background )

## Future Enhancements

- OLED display
- Digital smoothing/filtering of ADC values
- Value statistics: Usage Rate, Mean, SD
- Configuration changes via MQTT
- Support for additional sensors
- Alert notifications for threshold levels
- OTA firmware updates

## Installation & Usage

1. **Flash the firmware**
2. **First time startup (no settings in Flash)**
   - Pico boots into Acess Point (AP) mode
   - SSID=**PicoW**, Pwd=**password**
   - Connect your device (phone) to **PicoW** Access Point
3. **Configure settings**
   - In a Web browser go to: URL= <http://192.168.4.1>
   - Select **Configure** and enter WiFi and MQTT settings
   - Select **Save** - the device will reboot and connect with new settings
   - Check your router (DHCP server) to find new network address
   - Good idea to tell DHCP to assign a fixed address
4. **Operation**
   - Status led stays ON during startup and WiFi connecting.
   - Status led blinks: 1-Sec-ON, 1-Sec-OFF device is successfully connected.
   - Status led short blinks: device is waiting in AP mode at <http://192.168.4.1>
   - Connect to device Web server to dispaly the tank level
   - Integrate with an IoT dashboard (eg Home Assistant).
   - If you got the WiFi settings wrong, use the reset button (long press) to erase settings and start again.

## Hardware Requirements

- Raspberry Pi Pico W
- 4-20mA pressure transducer
- Power supply (e.g., 12v DC for the transducer and 5v for the PicoW)

## Pin Connections

| Pin    | Function     |
| ------ | ------------ |
| VSys   | 5V DC Power  |
| GPIO26 | ADC Input    |
| GPIO22 | Status LED   |
| GPIO13 | Reset Button |

## Software Requirements

- C SDK for Raspberry Pi Pico W
- LWIP Stack
- MQTT library (Included in LWIP library)
- Wi-Fi CYW43 networking libraries for Pico W

## Configuration

The system supports the following configuration parameters:

| Parameter               | Description                                    |
| ----------------------- | ---------------------------------------------- |
| `wifi-ssid`             | Wi-Fi SSID                                     |
| `wifi-password`         | Wi-Fi Password                                 |
| `mqtt-host`             | MQTT Broker Address                            |
| `mqtt-port`             | MQTT Port (default: 1883)                      |
| `mqtt-user`             | MQTT Username                                  |
| `mqtt-pwd`              | MQTT Password                                  |
| `mqtt-client-id`        | Unique MQTT Client ID                          |
| `mqtt-publish-interval` | Data publishing interval (seconds)             |
| `mqtt-sensor-topic`     | MQTT topic for sensor data                     |
| `mqtt-config-topic`     | MQTT topic for receiving configuration updates |

## License

This project is licensed under the **MIT License**.

---
