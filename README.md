## PicoW Tank Level Monitor

### PicoW project - uses a 4-20mA level sensor to monitor water tank level.

### Provides a web interface and supports MQTT publish

#### TODO

Deinit LWIP resources before reboot
Checksumm on settings
OTA updates

# Bug List

### Pinout

| Raspberry Pi PicoW / RP2040 |
| --------------------------- | ---------- |
| VSYS                        | 5v         |
| GND                         | GND        |
| GPIO 26                     | ADC 0      |
| GPIO 22                     | STATUS LED |
| GPIO 13                     | RESET      |
