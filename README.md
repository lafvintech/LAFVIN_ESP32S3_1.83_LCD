# ESP32-S3 1.83-inch LCD Development Board Documentation

This repository contains the Read the Docs source for the LAFVIN ESP32-S3
1.83-inch LCD Development Board.

The board uses an ESP32-S3 N16R8 controller with 16 MB Flash, 8 MB PSRAM,
Wi-Fi, Bluetooth, a 1.83-inch 240 x 284 ST7789 TFT LCD, a QMI8658A six-axis
sensor, and a TF card slot.

## Content

- Product overview and package list
- Preparation steps for code, drivers, Arduino IDE, and libraries
- Quick start, Arduino, ESP-IDF, and advanced tutorial sections
- Online flasher and troubleshooting appendix

## Build

From the repository root:

```powershell
cd docs
.\make.bat html
```

On Linux or macOS:

```bash
cd docs
make html
```

## Suggested Content Map

- `about_this_kit.rst`: product overview, hardware specifications, package list
- `Preparation/`: source download, driver, Arduino IDE, libraries
- `Tutorial/`: quick start and platform-specific tutorials
- `Appendix/`: online flasher, troubleshooting, additional setup references
