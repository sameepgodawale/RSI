ESP32 RSU Firmware Package

Wiring:
- SIM800L TX -> ESP32 RX2 (GPIO16)
- SIM800L RX -> ESP32 TX2 (GPIO17)
- SIM800L VCC -> external 4.0V regulator (2A peak)
- SIM800L GND -> GND

- SX1278 LoRa (VSPI):
  SCK  -> GPIO18
  MISO -> GPIO19
  MOSI -> GPIO23
  NSS  -> GPIO5
  RESET-> GPIO14
  DIO0 -> GPIO26

- MicroSD (SPI shared):
  SCK  -> GPIO18
  MISO -> GPIO19
  MOSI -> GPIO23
  CS   -> GPIO15

Build (PlatformIO):
  pio run -e esp32dev
  pio run -e esp32dev -t upload

Before build:
 - Edit include/cms_config.h compile-time fallbacks if you want.
 - Use web portal (AP mode) to set APN, SIM PIN, RSU API Key and CMS URL at runtime.
 - Ensure SIM800 power is stable (4V regulator).

Notes:
 - LoRa frequency set in loradrv.cpp (default 915E6). Adjust for region.
 - sim800.cpp logs verbose AT -> (Serial) and modem responses -> (Serial).
 - Web portal AP: SSID 'RSU-Setup' pass 'rsu12345'. Visit http://192.168.4.1/
