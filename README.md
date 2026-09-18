# TouchScan IC — Digital Logic IC Tester

TouchScan IC is a complete, self-contained firmware for testing 74-series digital logic integrated circuits. It runs on the ESP32-S3 N16R8 microcontroller and provides a full graphical touchscreen interface, automatic IC identification, gate-by-gate truth table verification, battery monitoring, test history, and audio feedback — all in a portable, battery-powered handheld device.

The project is built with PlatformIO (Arduino framework), uses LVGL 8.3 for the graphical interface, and targets a 2.8-inch SPI TFT display with resistive touch. The device can automatically detect which IC has been inserted into its 16-pin ZIF socket, run a full truth table test on every gate inside the chip, and display the results on screen with a clear pass or fail verdict for each gate.

---

## Table of Contents

1. [What This Project Does](#what-this-project-does)
2. [How It Works](#how-it-works)
3. [Hardware Requirements](#hardware-requirements)
4. [Wiring and Connections](#wiring-and-connections)
5. [Software Architecture](#software-architecture)
6. [Building and Uploading the Firmware](#building-and-uploading-the-firmware)
7. [Touch Screen Calibration](#touch-screen-calibration)
8. [Using the Device](#using-the-device)
9. [Supported ICs](#supported-ics)
10. [Adding New ICs to the Database](#adding-new-ics-to-the-database)
11. [Project Directory Structure](#project-directory-structure)
12. [Design Notes and Hardware Warnings](#design-notes-and-hardware-warnings)

---

## What This Project Does

In digital electronics work, one of the most common tasks is checking whether a 74-series logic IC (such as a 7408 AND gate or a 7404 inverter) is still functional. Traditionally, this is done by manually wiring the chip on a breadboard, applying inputs, and checking outputs with a multimeter or logic probe. This is slow, tedious, and error-prone.

TouchScan IC automates the entire process. You insert the IC into the ZIF (Zero Insertion Force) socket, and the device takes care of everything else:

- It identifies which IC you have inserted by probing the chip's outputs against known logic patterns from its internal database.
- It applies every possible combination of inputs to every gate in the IC and reads back the outputs.
- It compares the actual outputs against the expected truth table for that gate type.
- It shows the results on a colour TFT touchscreen, with a clear per-gate pass or fail indicator, a visual gate diagram, and the full truth table.
- It stores test results in non-volatile memory so you can review past tests even after the device has been powered off and on again.
- It plays distinct audio tones through a passive buzzer to give you immediate feedback: a rising melody for a good IC, a descending tone for a faulty one.

---

## How It Works

The test process follows this sequence:

1. **Power on.** The device boots and shows an animated splash screen while it initialises all hardware (display, touch controller, ZIF socket GPIOs, battery ADC, buzzer).

2. **Home screen.** The main screen appears with a battery level indicator in the corner. The firmware begins polling the ZIF socket every 500 milliseconds to detect whether an IC has been inserted.

3. **IC detection.** When the device detects that something is plugged into the ZIF socket, it runs its auto-detection algorithm. This works by first trying a 14-pin power configuration (VCC on pin 14, GND on pin 7), running a subset of truth table tests against every IC in the database, and scoring each one by how many outputs match. If no 14-pin IC matches, it tries a 16-pin configuration (VCC on pin 16, GND on pin 8). The IC with a 100 percent match is selected. If no IC matches, the user is informed that the chip could not be identified.

4. **Confirmation.** Once an IC is identified, the device shows the IC number (for example, "7408") and its description ("Quad 2-Input AND Gate") and asks the user to confirm before testing. The user can also manually select a different IC from the database if they already know what chip they have.

5. **Testing.** The test engine powers the IC through the ZIF socket, sets each gate's inputs to every possible binary combination, reads the output pin, and compares it against the mathematically computed expected value for that gate type. For a quad 2-input gate IC, this means testing four gates with four input combinations each (16 measurements total). For the 7404 hex inverter, it tests six gates with two input combinations each (12 measurements total). A FreeRTOS task runs the test in the background so the UI remains responsive and can show a live progress animation.

6. **Results.** The results screen shows each gate with a pass or fail badge, the full truth table with actual versus expected values highlighted, and an overall verdict. The result is automatically saved to the test history in flash storage (ESP32 NVS).

7. **History.** The user can browse past test results from the history screen. Up to 20 results are stored and survive reboots.

---

## Hardware Requirements

The following components are needed to build the device:

**Microcontroller:**
- ESP32-S3 N16R8 development board (16 MB flash, 8 MB OPI PSRAM). The PSRAM is important because LVGL and the display buffers consume a significant amount of RAM, and the standard ESP32-S3 without PSRAM will not have enough memory.

**Display:**
- 2.8-inch SPI TFT LCD module with an ST7789 display driver and built-in XPT2046 resistive touch controller. These modules are widely available and typically have a single 14-pin header that carries both the display SPI bus and the touch SPI bus. The display resolution is 320 by 240 pixels and is used in landscape orientation.

**IC Socket:**
- 16-pin ZIF (Zero Insertion Force) socket. This allows the user to insert and remove ICs without bending the pins. Both 14-pin and 16-pin ICs are supported. When a 14-pin IC is inserted into the 16-pin socket, it is top-aligned (pin 1 of the IC goes to pin 1 of the ZIF socket, and pin 14 goes to pin 16, leaving ZIF pins 8 and 9 empty on the right side).

**Audio:**
- One passive buzzer. The firmware generates tones using the ESP32's LEDC PWM peripheral, so an active buzzer (which produces only a single fixed tone) will not work correctly. A passive buzzer allows the firmware to play different frequencies and melodies.

**Power:**
- 3.7V single-cell Li-Po battery. The battery voltage is monitored through a resistive voltage divider (two 100k-ohm resistors) connected to one of the ESP32's ADC pins. The firmware uses a 19-point lookup table that models the non-linear discharge curve of a typical Li-Po cell to convert the measured voltage into an accurate battery percentage.

---

## Wiring and Connections

All connections are made to the GPIO pins of the ESP32-S3. The pin assignments are defined in the file `src/config.h` and can be changed there if your hardware layout differs.

### TFT Display and Touch Controller (Shared SPI Bus)

The display and touch controller share the same SPI bus (MOSI, MISO, SCK) but have separate chip select lines.

| Signal         | ESP32-S3 GPIO | Description                               |
| :------------- | :------------ | :---------------------------------------- |
| VCC            | 3.3V          | Display module power supply               |
| GND            | GND           | Ground                                    |
| CS             | GPIO 10       | TFT chip select (directly active low)     |
| RESET          | GPIO 8        | TFT hardware reset                        |
| DC             | GPIO 9        | TFT data/command selection                |
| SDI (MOSI)     | GPIO 11       | SPI data from MCU to display              |
| SCK            | GPIO 12       | SPI clock                                 |
| LED            | GPIO 38       | TFT backlight (driven by LEDC PWM)        |
| SDO (MISO)     | GPIO 13       | SPI data from display/touch to MCU        |
| T_CLK          | GPIO 12       | Touch clock (physically wired to SCK)     |
| T_CS           | GPIO 7        | Touch chip select                         |
| T_DIN          | GPIO 11       | Touch data in (physically wired to MOSI)  |
| T_DO           | GPIO 13       | Touch data out (physically wired to MISO) |
| T_IRQ          | GPIO 6        | Touch interrupt (active low when touched) |

### 16-Pin ZIF Socket

Each pin of the ZIF socket is connected to an individual GPIO on the ESP32-S3. The firmware configures these pins dynamically as inputs, outputs, VCC, or GND depending on which IC is being tested.

| ZIF Pin | ESP32-S3 GPIO | Special Role                                    |
| :------ | :------------ | :---------------------------------------------- |
| Pin 1   | GPIO 1        |                                                 |
| Pin 2   | GPIO 2        |                                                 |
| Pin 3   | GPIO 3        |                                                 |
| Pin 4   | GPIO 5        | (GPIO 4 is reserved for the battery ADC)        |
| Pin 5   | GPIO 14       |                                                 |
| Pin 6   | GPIO 15       |                                                 |
| Pin 7   | GPIO 16       | GND pin for 14-pin ICs                          |
| Pin 8   | GPIO 17       | GND pin for 16-pin ICs                          |
| Pin 9   | GPIO 18       |                                                 |
| Pin 10  | GPIO 21       |                                                 |
| Pin 11  | GPIO 39       |                                                 |
| Pin 12  | GPIO 40       |                                                 |
| Pin 13  | GPIO 41       |                                                 |
| Pin 14  | GPIO 42       | VCC pin for 14-pin ICs                          |
| Pin 15  | GPIO 45       |                                                 |
| Pin 16  | GPIO 47       | VCC pin for 16-pin ICs                          |

### Peripherals

| Peripheral    | ESP32-S3 GPIO | Description                                                                |
| :------------ | :------------ | :------------------------------------------------------------------------- |
| Passive Buzzer| GPIO 46       | Driven via LEDC PWM (Channel 2, Timer 1) for tone generation               |
| Battery ADC   | GPIO 4        | ADC1 Channel 3. Connected to the midpoint of a 100k/100k voltage divider from the battery positive terminal to ground. This halves the battery voltage so that a fully charged cell at 4.2V produces about 2.1V at the ADC pin, which is within the ESP32's safe ADC input range. |

---

## Software Architecture

The firmware is written in C++ using the Arduino framework on top of the ESP-IDF (Espressif IoT Development Framework). The code is organised into three layers:

### Hardware Abstraction Layer (src/hal/)

This layer contains the low-level drivers that talk directly to the hardware. Each peripheral has its own source and header file:

- **tft_hal** — Initialises the SPI bus, the ST7789 display driver, and the XPT2046 touch controller. Registers the LVGL display driver and input device driver. Manages backlight brightness through LEDC PWM.
- **zif_hal** — Controls the 16 GPIO pins connected to the ZIF socket. Provides functions to configure each pin as a floating input, a driven output, VCC, or GND. Includes a basic IC presence detection function that checks whether any pins are being driven externally. All pins default to high-impedance (input) state for safety.
- **battery_hal** — Reads the battery voltage through the ADC with the voltage divider compensation applied. Converts the voltage to a percentage using a 19-point lookup table based on a real Li-Po discharge curve. Provides a critical battery warning flag.
- **buzzer_hal** — Drives the passive buzzer using a separate LEDC PWM channel and timer. Provides blocking tone playback and several pre-defined non-blocking melodies (success, failure, detection beep, button click, low battery warning).

### Test Engine (src/engine/)

This layer contains the logic for identifying and testing ICs:

- **ic_database** — Stores the complete description of every supported IC in a structured format. Each IC entry includes its part number, human-readable name, pin count, pin-by-pin role assignments (VCC, GND, input, output), and gate definitions (gate type, input pins, output pin). The database also provides a function to compute the expected output of any gate type given its inputs, supporting AND, OR, NOT, NAND, NOR, XOR, XNOR, and BUFFER logic.
- **ic_tester** — The core test engine. Given an IC descriptor, it configures the ZIF socket pins according to the IC's pin map, iterates over every gate, applies every possible input combination, reads the output, and compares it against the expected value. It produces a detailed result structure that includes per-gate pass/fail status, per-row actual versus expected values, and overall test duration.
- **auto_detect** — The automatic identification module. It tries to identify an unknown IC by running a quick subset of truth table tests against every IC in the database and scoring each one by the number of matching outputs. It first tries a 14-pin configuration and then falls back to 16-pin if no match is found.

### User Interface (src/ui/)

The user interface is built entirely with LVGL 8.3 (Light and Versatile Graphics Library). The visual theme uses a "Deep Forest Emerald and Mint" colour palette with solid backgrounds, floating card panels, rounded corners, and soft drop shadows. The interface runs at the display's native resolution of 320 by 240 pixels in landscape orientation.

The UI is divided into several screens, each in its own source file:

- **ui_splash** — The boot splash screen with an animated entrance. Transitions automatically to the home screen after a configurable delay (default 3 seconds).
- **ui_home** — The main screen. Displays a battery indicator and continuously polls the ZIF socket for IC insertion. When an IC is detected, it transitions to the confirmation or test screen.
- **ui_ic_select** — A scrollable list that lets the user manually choose an IC from the database instead of relying on auto-detection.
- **ui_test_running** — Shows a live progress animation while the test runs in a background FreeRTOS task. Displays which gate is currently being tested and updates a progress bar.
- **ui_result** — The results screen. Shows the overall pass or fail verdict, per-gate results with colour-coded badges, and the complete truth table for each gate with actual and expected output values.
- **ui_history** — Lists past test results stored in non-volatile storage (ESP32 NVS). Up to 20 entries are kept and survive device reboots.
- **ui_settings** — Provides adjustable options including display brightness (via a slider that controls the LEDC PWM duty cycle), buzzer enable/disable toggle, and a touch calibration utility.
- **ui_calibrate** — Guides the user through a multi-point touch calibration procedure and prints the resulting calibration values to the serial monitor.
- **ui_theme** — Defines the entire colour palette, font shortcuts, opacity macros, and reusable style helper functions used by all other UI screens.

### Main Entry Point (src/main.cpp)

The `setup()` function initialises everything in a specific order designed to avoid visible glitches on the display:

1. The backlight pin is forced LOW immediately on boot so the screen stays dark while VRAM contains garbage data.
2. LVGL is initialised.
3. The TFT display and touch drivers are set up and registered with LVGL.
4. The ZIF socket GPIOs are set to safe high-impedance state.
5. The battery ADC is initialised.
6. The buzzer is initialised and plays a short startup beep.
7. The splash screen is created and rendered.
8. Only after the first clean frame has been rendered to the display buffer is the backlight turned on, resulting in a clean, glitch-free boot.

The `loop()` function simply increments the LVGL tick counter and calls the LVGL task handler on every iteration, with a small delay to allow FreeRTOS task switching. All LVGL rendering, animation processing, and event handling happens inside the LVGL task handler.

---

## Building and Uploading the Firmware

This project uses PlatformIO as its build system. It is not compatible with the Arduino IDE without significant modification.

### Prerequisites

1. Install Visual Studio Code from https://code.visualstudio.com/.
2. Install the PlatformIO IDE extension from the VS Code Extensions marketplace.
3. Make sure your ESP32-S3 board is connected via USB. If the board is not recognised by your computer, you may need to install the USB-to-serial driver for your board's chip (commonly CH340 or CP2102).

### Build and Upload

1. Open the `Digital IC checker` folder in VS Code. PlatformIO will automatically detect the `platformio.ini` configuration file.
2. Click the Upload button (the right-pointing arrow icon) in the PlatformIO toolbar at the bottom of the VS Code window. Alternatively, open a terminal and run:

   ```
   pio run --target upload
   ```

3. The first build will download all required libraries (LVGL 8.3, Adafruit ST7789, XPT2046 Touchscreen) and compile everything from scratch. This typically takes between 2 and 5 minutes depending on your machine. Subsequent builds are much faster because only changed files are recompiled.

### If the Upload Fails to Connect

If the upload process hangs at `Connecting........_____.....` and cannot establish communication with the board:

1. Hold down the BOOT button on the ESP32-S3 development board.
2. While still holding BOOT, briefly press and release the RESET (EN) button.
3. Release the BOOT button.
4. Try the upload again. The board should now be in bootloader mode and accept the firmware.

### Serial Monitor

To view debug output from the firmware, open the serial monitor at 115200 baud:

```
pio device monitor
```

Or click the plug-shaped icon in the PlatformIO toolbar. The firmware prints status messages during boot and during IC detection and testing.

---

## Touch Screen Calibration

After uploading the firmware for the first time, or whenever you change the physical display module, you need to calibrate the touch screen so that touch coordinates match the actual pixel positions on the display.

1. Connect the device to your computer and open the serial monitor (115200 baud).
2. On the device, navigate to Settings and then select Touch Calibrate.
3. The screen will show red dots in the corners, one at a time. Tap each dot as precisely as you can with a stylus or fingertip.
4. After the calibration is complete, the serial monitor will print five integer values. These are the calibration constants for your specific display module.
5. Open the file `src/config.h` and find the line that defines the `TOUCH_CAL` array. Replace the five values with the ones printed by the serial monitor:

   ```cpp
   constexpr uint16_t TOUCH_CAL[5] = { val1, val2, val3, val4, val5 };
   ```

6. Upload the firmware one more time. The touch screen should now respond accurately to your taps.

You only need to do this once per display module. The calibration values are compiled into the firmware, so they persist across reboots automatically.

---

## Using the Device

1. Power on the device. The splash screen appears for about 3 seconds, then the home screen loads.
2. Insert a 74-series logic IC into the ZIF socket. Make sure pin 1 of the IC aligns with pin 1 of the ZIF socket (top-left, usually marked with a notch or dot on the IC package).
3. The device will automatically detect the IC within about half a second. A short beep confirms detection, and the identified IC number and name are shown on the screen.
4. Tap the test button to begin testing, or select a different IC manually if the auto-detection was incorrect.
5. Watch the test progress on screen. Each gate is tested in sequence, and the progress bar updates in real time.
6. When the test is complete, the results screen shows the verdict. A rising melody means the IC is good; a descending tone means it is faulty. Each gate is shown with its own pass or fail indicator, and you can scroll through the truth table to see exactly which input combinations produced incorrect outputs.
7. The result is automatically saved. You can view past results from the History screen.

---

## Supported ICs

The current firmware (version 1.1) supports the following 74-series TTL/CMOS logic ICs:

| IC Number | Description              | Pin Count | Number of Gates |
| :-------- | :----------------------- | :-------- | :-------------- |
| 7400      | Quad 2-Input NAND Gate   | 14        | 4               |
| 7402      | Quad 2-Input NOR Gate    | 14        | 4               |
| 7404      | Hex Inverter (NOT Gate)  | 14        | 6               |
| 7408      | Quad 2-Input AND Gate    | 14        | 4               |
| 7432      | Quad 2-Input OR Gate     | 14        | 4               |
| 7486      | Quad 2-Input XOR Gate    | 14        | 4               |

The database architecture supports AND, OR, NOT, NAND, NOR, XOR, XNOR, and BUFFER gate types with up to 3 inputs per gate. Both 14-pin and 16-pin IC packages are supported by the hardware and software.

---

## Adding New ICs to the Database

To add support for a new IC, you only need to edit one file: `src/engine/ic_database.cpp`. Each IC is defined as a set of C++ data structures:

1. Define the pin array: create a `const ICPin` array listing every pin on the IC with its ZIF socket pin number, role (VCC, GND, input, or output), and label.
2. Define the gate array: create a `const ICGate` array listing each gate with its type (AND, OR, NAND, etc.), input pin numbers, output pin number, and number of inputs.
3. Add an `ICDescriptor` entry that ties everything together with the IC part number, full name, description, pin count, gate count, and pointers to the pin and gate arrays.
4. Add the new descriptor to the master database array and increment the count.

The test engine and auto-detection module will automatically pick up the new IC without any other code changes. The expected output for each gate type is computed mathematically at test time, so you do not need to hard-code truth tables.

---

## Project Directory Structure

```
Digital IC checker/
    platformio.ini              Build configuration (board, libraries, flags)
    README.md                   This file
    src/
        main.cpp                Firmware entry point (setup and main loop)
        config.h                Pin assignments, timing constants, calibration
        lv_conf.h               LVGL library configuration
        engine/
            ic_database.h       IC descriptor data structures
            ic_database.cpp     IC database (all supported IC definitions)
            ic_tester.h         Test engine interface
            ic_tester.cpp       Test engine implementation
            auto_detect.h       Auto-detection interface
            auto_detect.cpp     Auto-detection implementation
        hal/
            tft_hal.h           Display and touch driver interface
            tft_hal.cpp         Display and touch driver implementation
            zif_hal.h           ZIF socket GPIO interface
            zif_hal.cpp         ZIF socket GPIO implementation
            battery_hal.h       Battery monitor interface
            battery_hal.cpp     Battery monitor implementation
            buzzer_hal.h        Buzzer driver interface
            buzzer_hal.cpp      Buzzer driver implementation
        ui/
            ui_theme.h          Colour palette, fonts, and style helpers
            ui_splash.h/.cpp    Splash screen
            ui_home.h/.cpp      Home screen with IC detection polling
            ui_ic_select.h/.cpp Manual IC selection list
            ui_test_running.h/.cpp  Live test progress screen
            ui_result.h/.cpp    Test results display
            ui_history.h/.cpp   Saved test history browser
            ui_settings.h/.cpp  Settings menu (brightness, buzzer, calibration)
            ui_calibrate.h/.cpp Touch calibration wizard
```

---

## Design Notes and Hardware Warnings

**ZIF socket power delivery.** In this design, the VCC pin of the IC under test is powered directly from an ESP32 GPIO pin set to OUTPUT HIGH. The ESP32-S3 GPIOs can source approximately 12 milliamps each at 3.3 volts. This is sufficient for testing 74HC-series CMOS logic ICs, which draw very little current. However, original 74LS-series TTL ICs can draw more current than a single GPIO can safely provide. For a more robust design intended for production use, the VCC rail should be switched through a P-channel MOSFET controlled by a GPIO, with the 3.3V power coming directly from the voltage regulator rather than through the MCU.

**Battery voltage divider.** The ESP32-S3's ADC can safely measure voltages up to about 3.1 volts (with the default 11 dB attenuation). A fully charged Li-Po cell is at 4.2 volts, which would exceed this range and potentially damage the ADC. The 100k/100k resistive voltage divider halves the battery voltage so that 4.2 volts becomes 2.1 volts at the ADC pin, comfortably within the safe range. If you use different resistor values, you will need to update the `BATTERY_DIVIDER` constant in `src/config.h`.

**Pin safety.** The ZIF HAL always sets all pins to high-impedance input mode (floating) before and after every test. This prevents accidental short circuits if the user inserts or removes an IC while the GPIOs are actively driving. The firmware also reconfigures the pins carefully during testing: VCC and GND are applied first, then input pins are driven, and only then are output pins read.

**Display backlight sequencing.** The firmware deliberately holds the backlight off during the entire initialisation process. The ST7789 display's VRAM contains random data after power-on, and turning the backlight on immediately would cause a visible flash of noise. Instead, the firmware initialises LVGL, renders the first clean frame to the display buffer, and only then enables the backlight via PWM. This results in a completely clean, professional boot experience.

**LVGL memory.** The LVGL configuration (`src/lv_conf.h`) is tuned for the ESP32-S3 with PSRAM. The display buffer and widget memory are allocated to take advantage of the 8 MB of external PSRAM. If you port this firmware to an ESP32 variant without PSRAM, you will need to significantly reduce the LVGL buffer sizes and simplify the UI to fit within the internal SRAM.

---

Built for digital electronics enthusiasts and anyone who works with 74-series logic ICs regularly. Contributions and IC database expansions are welcome.
