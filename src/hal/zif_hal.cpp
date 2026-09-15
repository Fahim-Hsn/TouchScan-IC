/**
 * zif_hal.cpp — ZIF Socket GPIO Implementation
 */
#include "zif_hal.h"
#include "../config.h"
#include "driver/gpio.h"

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
        
        // Reset pin to detach from JTAG/other alternate functions (esp. GPIO 39-42 on S3)
        gpio_reset_pin((gpio_num_t)gpio);
        
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
    // Reliable, non-destructive detection:
    // 1. Power the IC (VCC=3.3V, GND=0V).
    // 2. Keep all logic pins in high-Z input mode (no forced OUTPUT LOW)
    //    to avoid short circuits / bus contention with IC outputs (e.g. 7402, 7404).
    // 3. For all socket logic pins, test if the pin is actively driven by an IC output:
    //    - With INPUT_PULLUP (weak ~45k pull-up): read pin logic state
    //    - With INPUT_PULLDOWN (weak ~45k pull-down): read pin logic state
    //    - If an IC gate is driving the pin HIGH or LOW, both reads match (val_pullup == val_pulldown).
    //    - If the pin is empty / open circuit (floating), pullup gives 1 and pulldown gives 0.

    zif_hal_release_all();

    // --- Try 14-Pin IC Configuration (VCC=ZIF 16, GND=ZIF 7) ---
    pinMode(zif_to_gpio(16), OUTPUT);
    digitalWrite(zif_to_gpio(16), HIGH);
    pinMode(zif_to_gpio(7), OUTPUT);
    digitalWrite(zif_to_gpio(7), LOW);

    delay(2); // Let power rail stabilise

    // Active pins on a 14-pin IC top-aligned:
    // Left side: ZIF 1..6 (IC 1..6)
    // Right side: ZIF 10..15 (IC 8..13)
    const uint8_t pins_14[] = {1, 2, 3, 4, 5, 6, 10, 11, 12, 13, 14, 15};
    uint8_t detected_14 = 0;

    for (uint8_t p : pins_14) {
        uint8_t gpio = zif_to_gpio(p);
        if (gpio == 0) continue;

        pinMode(gpio, INPUT_PULLUP);
        delayMicroseconds(50);
        bool val_pullup = (digitalRead(gpio) == HIGH);

        pinMode(gpio, INPUT_PULLDOWN);
        delayMicroseconds(50);
        bool val_pulldown = (digitalRead(gpio) == HIGH);

        if (val_pullup == val_pulldown) {
            detected_14++;
        }
    }

    zif_hal_release_all();

    if (detected_14 >= 2) {
        return true;
    }

    // --- Try 16-Pin IC Configuration (VCC=ZIF 16, GND=ZIF 8) ---
    pinMode(zif_to_gpio(16), OUTPUT);
    digitalWrite(zif_to_gpio(16), HIGH);
    pinMode(zif_to_gpio(8), OUTPUT);
    digitalWrite(zif_to_gpio(8), LOW);

    delay(2);

    const uint8_t pins_16[] = {1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15};
    uint8_t detected_16 = 0;

    for (uint8_t p : pins_16) {
        uint8_t gpio = zif_to_gpio(p);
        if (gpio == 0) continue;

        pinMode(gpio, INPUT_PULLUP);
        delayMicroseconds(50);
        bool val_pullup = (digitalRead(gpio) == HIGH);

        pinMode(gpio, INPUT_PULLDOWN);
        delayMicroseconds(50);
        bool val_pulldown = (digitalRead(gpio) == HIGH);

        if (val_pullup == val_pulldown) {
            detected_16++;
        }
    }

    zif_hal_release_all();

    return (detected_16 >= 2);
}


