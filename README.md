# Pico W Tank Monitor

## Overview

The **Pico W Tank Monitor** is an IoT device to monitor liquid levels using a **4-20mA pressure transducer** and an **analog-to-digital converter (ADC)**. The software runs on a **Raspberry Pi Pico W**, which connects to a network via Wi-Fi and can publish sensor data via **MQTT**.

## Features

- Uses industry standard **4-20mA pressure transducer**.
- Converts the analog signal to digital using 0-3.3v input **ADC0 of the Pico**.
- Connects to **Wi-Fi** and operates as a client or **Wireless Access Point (AP)** for setup.
- Publishes data to an **MQTT broker** at configurable intervals.
- Web pages for level display, status and configuration.
- Reset button via GPIO13 will erase flash and go back to factory setup.

## Build Versions

### v0.2.0 - Bug/Feature Release

- Added data log to record the last 100 readings with configurable time interval (minutes). Note data is not saved (held in RAM only)
- Added a chart on main page to display the data being logged
- Added Window.onload() to main page to wait before calling for data while.

### v0.1.5 - Bug/Feature Release

- MQTT will try to reconnect if not connected, added reconnect count to status messages
- Fixed Version number in status messages
- Make sure WiFi scan results dont overflow the results buffer
- Added Good, Fair, Poor to signal strength results
- Do immediate reboot on Factory Reset to avoid possible race condition with save_settings

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

- Use Home Assistant MQTT Discovery Message
- Save Debug/Error messages so they can be queried via JSON api
- Have a fallback SSID for WiFi connections
- Use PicoW unique ID for AP mode SSID
- The get_xxx api is limited by LWIP_HTTPD_MAX_TAG_INSERT_LEN (use LWIP_HTTPD_SSI_MULTIPART)
- Do a build using interrupt driven LWIP
- Upgrade to latest Pico SDK (Current: 1.5.1 - instability(?) issues with 2.0+)
- Use checksum to verify Flash memory
- Flash settings are limited to a single 256 byte page
- Make settings a C++ Class?
- Fix Build Kits (Release, Debug. Select Arch: Poll, Threadsafe background )

## Future Enhancements

- User specified units and scale factor for level display
- OLED Display - SSD1306 I2C
- Digital smoothing/filtering of ADC values
- Value statistics: Usage Rate, Mean, SD
- Configuration changes via MQTT
- Support for additional sensors
- Alert notifications for threshold levels
- OTA firmware updates
- PCB
  -- Mounting holes larger diamenter
  -- Test Points
  -- Direct through holes for 4-20mA converter
  -- Cable shield connected to ground
  -- Factory reset - external button??
  -- Reduce signal Noise

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
   - If the WiFi settings are wrong, use the reset button (long press > 1-Sec) to erase settings and start again.

## Hardware Requirements

- See: picotool info -a tank_level_monitor.uf2
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

- SDK for Raspberry Pi PicoW
- WiFi CYW43 networking libraries for PicoW (Included in SDK)
- LWIP Stack (Included in SDK) <https://savannah.nongnu.org/projects/lwip/>
- VSCode extension for Raspberry Pi Pico <https://github.com/raspberrypi/pico-vscode>

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
| `mqtt-client-id`        | Unique MQTT Client ID (derived from Pico id)   |
| `mqtt-publish-interval` | Data publishing interval (seconds)             |
| `mqtt-sensor-topic`     | MQTT topic for sensor data                     |
| `mqtt-config-topic`     | MQTT topic for receiving configuration updates |

## License

This project is licensed under the **MIT License**.

---
