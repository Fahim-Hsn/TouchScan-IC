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
    // Heuristic: apply VCC to pin 14 and GND to pin 7 (standard 14-pin TTL).
    // If an IC is present, the VCC rail will have a small load pulling it slightly
    // below 3.3V, detectable via analogRead on the VCC pin in some setups.
    //
    // Simpler heuristic used here:
    //   1. Set all non-power pins as INPUT_PULLUP
    //   2. Drive VCC=pin14, GND=pin7
    //   3. If any of the "output" pins of a known IC read LOW despite PULLUP,
    //      it means the IC is driving them — IC is present.
    //
    // For simplicity in v1: check if pin 14 to GND resistance is low enough
    // to indicate a connected IC by reading pin 7 after driving pin 14 HIGH.
    // If pin 7 is LOW when driven LOW (GND), that's an IC path.

    // Quick check: try to detect resistance path between pin 14 (VCC) and pin 7 (GND)
    // This is just a presence check, not a type-detection.

    // Save state
    zif_hal_release_all();

    // Drive pin 14 HIGH (would be VCC), pin 7 LOW (would be GND)
    pinMode(zif_to_gpio(14), OUTPUT);
    digitalWrite(zif_to_gpio(14), HIGH);
    pinMode(zif_to_gpio(7), INPUT_PULLUP);
    delayMicroseconds(100);

    // If an IC is present and its VCC-GND path has the expected impedance,
    // pin 7 will be pulled towards LOW more than an open pin would
    bool level_pin7 = (digitalRead(zif_to_gpio(7)) == HIGH);

    // Restore safe state
    zif_hal_release_all();

    // If pin7 is being pulled LOW (not floating HIGH with PULLUP), IC detected
    return !level_pin7;
}
