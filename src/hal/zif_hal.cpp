/**
 * zif_hal.cpp — ZIF Socket GPIO Implementation
 */
#include "zif_hal.h"
#include "../config.h"

// ─── Internal helpers ─────────────────────────────────────────────────────

// Convert 1-indexed ZIF pin to GPIO number
static inline uint8_t zif_to_gpio(uint8_t zif_pin) {
    if (zif_pin < 1 || zif_pin > 16) return 0;
    return ZIF_GPIO[zif_pin];
}

// ─── Public API ───────────────────────────────────────────────────────────

void zif_hal_init(void) {
    for (uint8_t p = 1; p <= 16; p++) {
        uint8_t gpio = zif_to_gpio(p);
        if (gpio == 0) continue;
        pinMode(gpio, INPUT);  // High-Z — safe default
    }
    Serial.println("[ZIF] All 16 ZIF pins initialised as INPUT (high-Z)");
}

void zif_hal_release_all(void) {
    for (uint8_t p = 1; p <= 16; p++) {
        uint8_t gpio = zif_to_gpio(p);
        if (gpio == 0) continue;
        pinMode(gpio, INPUT);
    }
}

void zif_hal_configure(uint8_t zif_pin, ZIFPinRole role, bool value) {
    uint8_t gpio = zif_to_gpio(zif_pin);
    if (gpio == 0) return;

    switch (role) {
        case ZIFPinRole::FLOAT:
            pinMode(gpio, INPUT);
            break;

        case ZIFPinRole::VCC:
            // Drive HIGH — IC supply voltage (3.3V via GPIO, 12mA max)
            pinMode(gpio, OUTPUT);
            digitalWrite(gpio, HIGH);
            break;

        case ZIFPinRole::GND:
            // Drive LOW — IC ground
            pinMode(gpio, OUTPUT);
            digitalWrite(gpio, LOW);
            break;

        case ZIFPinRole::DUT_INPUT:
            // Output to IC input pin
            pinMode(gpio, OUTPUT);
            digitalWrite(gpio, value ? HIGH : LOW);
            break;

        case ZIFPinRole::DUT_OUTPUT:
            // Input from IC output pin (with pull-down to default to logic 0)
            pinMode(gpio, INPUT_PULLDOWN);
            break;
    }
}

void zif_hal_write(uint8_t zif_pin, bool value) {
    uint8_t gpio = zif_to_gpio(zif_pin);
    if (gpio == 0) return;
    digitalWrite(gpio, value ? HIGH : LOW);
}

bool zif_hal_read(uint8_t zif_pin) {
    uint8_t gpio = zif_to_gpio(zif_pin);
    if (gpio == 0) return false;
    return digitalRead(gpio) == HIGH;
}

bool zif_hal_detect_ic_presence(void) {
    // Reliable detection: Power up the IC, then check if output pins
    // are being actively driven (stable readings) vs floating (random/unstable).
    //
    // Method:
    //   1. Drive VCC and GND pins to power the IC
    //   2. Set all input pins to a known state (LOW)
    //   3. Read output pins multiple times with alternating pullup/pulldown
    //   4. If an IC is present, output pins will be actively driven to a 
    //      consistent level. If no IC, pins will follow the pull direction (float).

    zif_hal_release_all();

    // Power up: try both 14-pin (VCC=14, GND=7) and 16-pin (VCC=16, GND=8)
    // Drive pin 14 HIGH (VCC for 14-pin ICs)
    pinMode(zif_to_gpio(14), OUTPUT);
    digitalWrite(zif_to_gpio(14), HIGH);
    // Drive pin 7 LOW (GND for 14-pin ICs)
    pinMode(zif_to_gpio(7), OUTPUT);
    digitalWrite(zif_to_gpio(7), LOW);
    // Also drive 16-pin power rails
    pinMode(zif_to_gpio(16), OUTPUT);
    digitalWrite(zif_to_gpio(16), HIGH);
    pinMode(zif_to_gpio(8), OUTPUT);
    digitalWrite(zif_to_gpio(8), LOW);

    // Set some common input pins LOW (pins 1, 2, 4, 5 are inputs on most 74xx)
    uint8_t input_pins[] = {1, 2, 4, 5, 9, 10, 12, 13};
    for (uint8_t p : input_pins) {
        pinMode(zif_to_gpio(p), OUTPUT);
        digitalWrite(zif_to_gpio(p), LOW);
    }

    delay(2); // Let the IC settle

    // Check common output pins (pin 3, 6, 8, 11 are outputs on most 74xx ICs)
    // If IC is present, these will be actively driven and won't follow pullup/pulldown
    uint8_t test_pins[] = {3, 6, 11};
    uint8_t detected_count = 0;

    for (uint8_t p : test_pins) {
        uint8_t gpio = zif_to_gpio(p);
        
        // Read with PULLUP
        pinMode(gpio, INPUT_PULLUP);
        delayMicroseconds(50);
        bool val_pullup = digitalRead(gpio);

        // Read with PULLDOWN
        pinMode(gpio, INPUT_PULLDOWN);
        delayMicroseconds(50);
        bool val_pulldown = digitalRead(gpio);

        // If IC is driving the pin, both reads should give the SAME value
        // (the IC overrides the weak pull resistor).
        // If no IC (floating), pullup gives HIGH and pulldown gives LOW.
        if (val_pullup == val_pulldown) {
            detected_count++;
        }
    }

    // Restore safe state
    zif_hal_release_all();

    // If at least 2 out of 3 pins show active drive, IC is present
    return (detected_count >= 2);
}

