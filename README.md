# TouchScan IC — Digital Logic Tester

An advanced, feature-rich Digital Logic IC Tester firmware built for the **ESP32-S3 N16R8**. It features a stunning Sci-Fi holographic UI with animations, auto-detection of ICs, gate-level truth table testing, and battery monitoring.

![Project Status](https://img.shields.io/badge/Status-Active-success)
![Platform](https://img.shields.io/badge/Platform-ESP32--S3-blue)
![Framework](https://img.shields.io/badge/Framework-PlatformIO-orange)
![UI](https://img.shields.io/badge/UI-LVGL%208.3-green)

---

## ✨ Features

- **🚀 High-Performance UI:** Smooth, animated Sci-Fi/neon user interface powered by LVGL 8.3 and FreeRTOS.
- **🔍 Auto-Detection:** Automatically identifies inserted ICs with confidence scoring.
- **🧪 Gate-Level Testing:** Tests individual gates within the IC and displays visual pass/fail results on a gate diagram.
- **📊 Real-time Truth Table:** Live animated truth table generation during testing.
- **🔋 Battery Management:** Real-time battery percentage monitoring with a 19-point Li-Po discharge curve.
- **📜 Test History:** Non-volatile storage (NVS) for keeping track of past test results (survives reboots).
- **🔊 Audio Feedback:** Distinct buzzer melodies for pass, fail, detection, and low battery alerts.
- **⚙️ Settings Menu:** Adjustable brightness, buzzer toggle, and touch calibration.

---

## 🛠️ Hardware Requirements

- **Microcontroller:** ESP32-S3 N16R8 (16MB Flash, 8MB OPI PSRAM)
- **Display:** 2.8" SPI TFT LCD (ST7789 driver) with XPT2046 Resistive Touch
- **Socket:** 16-pin ZIF Socket
- **Audio:** Passive Buzzer
- **Power:** 3.7V Li-Po Battery (with 100k/100k voltage divider for ADC)

---

## 🔌 Connection Diagram

### 1. TFT Display & Touch (SPI Bus)
| Display Pin Label | ESP32-S3 GPIO | Note |
| :--- | :--- | :--- |
| **VCC** | 3.3V | Power (3.3V) |
| **GND** | GND | Ground |
| **CS** | GPIO 10 | TFT Chip Select |
| **RESET** | GPIO 8 | TFT Reset |
| **DC** | GPIO 9 | TFT Data/Command |
| **SDI(MOSI)** | GPIO 11 | SPI MOSI |
| **SCK** | GPIO 12 | SPI Clock |
| **LED** | GPIO 38 | TFT Backlight (LEDC PWM Ch 1) |
| **SDO(MISO)** | GPIO 13 | SPI MISO |
| **T_CLK** | GPIO 12 | Touch Clock (Connect to SCK) |
| **T_CS** | GPIO 7 | Touch Chip Select |
| **T_DIN** | GPIO 11 | Touch Data In (Connect to MOSI) |
| **T_DO** | GPIO 13 | Touch Data Out (Connect to MISO) |
| **T_IRQ** | GPIO 6 | Touch Interrupt |

### 2. 16-pin ZIF Socket
> **Hardware Note:** VCC pins (14, 16) are driven directly from the ESP32 GPIO (3.3V). For commercial production, it is highly recommended to use a P-channel MOSFET to switch the 3.3V rail instead of driving it directly from the MCU pins to prevent overcurrent damage.

| ZIF Pin | ESP32-S3 GPIO | Note |
| :--- | :--- | :--- |
| **Pin 1** | GPIO 1 | |
| **Pin 2** | GPIO 2 | |
| **Pin 3** | GPIO 3 | |
| **Pin 4** | GPIO 5 | *(GPIO 4 is reserved for Battery ADC)* |
| **Pin 5** | GPIO 14 | |
| **Pin 6** | GPIO 15 | |
| **Pin 7** | GPIO 16 | **GND** for 14-pin ICs |
| **Pin 8** | GPIO 17 | **GND** for 16-pin ICs |
| **Pin 9** | GPIO 18 | |
| **Pin 10** | GPIO 21 | |
| **Pin 11** | GPIO 39 | |
| **Pin 12** | GPIO 40 | |
| **Pin 13** | GPIO 41 | |
| **Pin 14** | GPIO 42 | **VCC** for 14-pin ICs |
| **Pin 15** | GPIO 45 | |
| **Pin 16** | GPIO 47 | **VCC** for 16-pin ICs |

### 3. Peripherals
| Peripheral | ESP32-S3 GPIO | Note |
| :--- | :--- | :--- |
| **Passive Buzzer** | GPIO 48 | LEDC PWM (Ch 0) |
| **Battery ADC** | GPIO 4 | ADC1_CH3. Connect to a 100k/100k voltage divider from VBAT to halve the voltage (max 2.1V). |

---

## 💻 Software Setup & Installation

This project is built using **PlatformIO** in VS Code.

1. **Install VS Code & PlatformIO:** Download VS Code and install the PlatformIO IDE extension.
2. **Open Project:** Open the `Digital IC checker` folder in VS Code.
3. **Connect Board:** Connect your ESP32-S3 board via USB. If it's not recognized, ensure you have the CH340 or CP2102 drivers installed.
4. **Build & Upload:**
   - Click the **Upload** button (right arrow icon `→`) in the PlatformIO bottom toolbar.
   - Alternatively, open the terminal and run:
     ```bash
     pio run --target upload
     ```
   *Note: The first compilation will download all necessary libraries (LVGL, TFT_eSPI) and may take 2-5 minutes.*

### Bootloader Mode
If the upload hangs at `Connecting........_____.....`:
1. Hold the **BOOT** button on the ESP32-S3.
2. Press and release the **RESET (EN)** button.
3. Release the **BOOT** button.
4. Try uploading again.

---

## 🎯 Touch Calibration (Important!)

After uploading the firmware for the first time, you **must** calibrate the touch screen:

1. Open the Serial Monitor in PlatformIO (`pio device monitor` or plug icon).
2. On the device, navigate to **Settings > Touch Calibrate**.
3. Carefully tap the red dots in the corners of the screen as they appear.
4. The Serial Monitor will print 5 calibration values.
5. Copy these values and paste them into `src/config.h` under the `TOUCH_CAL[]` array:
   ```cpp
   constexpr uint16_t TOUCH_CAL[5] = { val1, val2, val3, val4, val5 };
   ```
6. **Upload the code one final time.** Your touch screen will now be perfectly accurate.

---

## 📦 Supported ICs (v1.0)

Currently supports the following 74-series TTL logic gates:
- **7400:** Quad 2-Input NAND
- **7402:** Quad 2-Input NOR
- **7404:** Hex Inverter (NOT)
- **7408:** Quad 2-Input AND
- **7432:** Quad 2-Input OR
- **7486:** Quad 2-Input XOR

*(More ICs can easily be added by defining them in `src/engine/ic_database.cpp`)*

---
*Built with ❤️ for digital electronics enthusiasts.*
