/**
 * zif_hal.cpp — ZIF Socket GPIO Implementation
 *
 * Reliability improvements:
 *   - gpio_reset_pin() on every release to fully detach alternate functions
 *   - GPIO_DRIVE_CAP_3 (40mA) for DUT_INPUT and VCC pins
 *   - Multi-sample reads with majority voting for noise immunity
 *   - Proper settling delays after pin configuration changes
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
        
        // Full reset: detach any alternate function, clear drive mode,
        // discharge residual state — critical for GPIOs 39-42 (ex-JTAG)
        gpio_reset_pin((gpio_num_t)gpio);
        pinMode(gpio, INPUT);
    }
    // Brief settle time for all pins to reach high-Z
    delayMicroseconds(100);
}

void zif_hal_power_down(void) {
    // 1. Actively discharge all socket pins to GND (0V)
    // Drains residual charge from decoupling capacitors and internal CMOS gates
    for (uint8_t p = 1; p <= 16; p++) {
        uint8_t gpio = zif_to_gpio(p);
        if (gpio == 0) continue;
        pinMode(gpio, OUTPUT);
        digitalWrite(gpio, LOW);
    }
    // Hold at 0V for 15ms to ensure complete discharge (cold baseline)
    delay(15);

    // 2. Safely release all pins to high-Z INPUT mode
    for (uint8_t p = 1; p <= 16; p++) {
        uint8_t gpio = zif_to_gpio(p);
        if (gpio == 0) continue;
        gpio_reset_pin((gpio_num_t)gpio);
        pinMode(gpio, INPUT);
    }
    delay(2);
}

void zif_hal_configure(uint8_t zif_pin, ZIFPinRole role, bool value) {
    uint8_t gpio = zif_to_gpio(zif_pin);
    if (gpio == 0) return;

    switch (role) {
        case ZIFPinRole::FLOAT:
            gpio_reset_pin((gpio_num_t)gpio);
            pinMode(gpio, INPUT);
            break;

        case ZIFPinRole::VCC:
            // Drive HIGH — IC supply voltage (3.3V)
            // Use max drive strength for reliable power delivery through ZIF traces
            pinMode(gpio, OUTPUT);
            gpio_set_drive_capability((gpio_num_t)gpio, GPIO_DRIVE_CAP_3); // 40mA max
            digitalWrite(gpio, HIGH);
            break;

        case ZIFPinRole::GND:
            // Drive LOW — IC ground
            // Use max drive strength for reliable ground path
            pinMode(gpio, OUTPUT);
            gpio_set_drive_capability((gpio_num_t)gpio, GPIO_DRIVE_CAP_3); // 40mA max
            digitalWrite(gpio, LOW);
            break;

        case ZIFPinRole::DUT_INPUT:
            // Output to IC input pin — max drive for clean logic levels
            pinMode(gpio, OUTPUT);
            gpio_set_drive_capability((gpio_num_t)gpio, GPIO_DRIVE_CAP_3); // 40mA max
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
    
    // Multi-sample read with majority voting for noise immunity.
    // ZIF socket traces can pick up transient noise — a single digitalRead()
    // is unreliable. 3 samples with small gaps filters out glitches.
    uint8_t highs = 0;
    for (uint8_t i = 0; i < 3; i++) {
        if (digitalRead(gpio) == HIGH) highs++;
        delayMicroseconds(5);
    }
    return (highs >= 2);  // Majority vote: 2 out of 3
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
    // Note: For a 14-pin IC top-aligned, VCC is IC pin 14 → ZIF pin 16,
    //       GND is IC pin 7 → ZIF pin 7
    uint8_t vcc_gpio = zif_to_gpio(16);
    uint8_t gnd_gpio = zif_to_gpio(7);
    
    pinMode(vcc_gpio, OUTPUT);
    gpio_set_drive_capability((gpio_num_t)vcc_gpio, GPIO_DRIVE_CAP_3);
    digitalWrite(vcc_gpio, HIGH);
    pinMode(gnd_gpio, OUTPUT);
    gpio_set_drive_capability((gpio_num_t)gnd_gpio, GPIO_DRIVE_CAP_3);
    digitalWrite(gnd_gpio, LOW);

    delay(5); // Let power rail fully stabilise (IC internal capacitance)

    // Active pins on a 14-pin IC top-aligned:
    // Left side: ZIF 1..6 (IC 1..6)
    // Right side: ZIF 10..15 (IC 8..13)
    const uint8_t pins_14[] = {1, 2, 3, 4, 5, 6, 10, 11, 12, 13, 14, 15};
    uint8_t detected_14 = 0;

    for (uint8_t p : pins_14) {
        uint8_t gpio = zif_to_gpio(p);
        if (gpio == 0) continue;

        pinMode(gpio, INPUT_PULLUP);
        delayMicroseconds(100);
        bool val_pullup = (digitalRead(gpio) == HIGH);

        pinMode(gpio, INPUT_PULLDOWN);
        delayMicroseconds(100);
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
    vcc_gpio = zif_to_gpio(16);
    gnd_gpio = zif_to_gpio(8);
    
    pinMode(vcc_gpio, OUTPUT);
    gpio_set_drive_capability((gpio_num_t)vcc_gpio, GPIO_DRIVE_CAP_3);
    digitalWrite(vcc_gpio, HIGH);
    pinMode(gnd_gpio, OUTPUT);
    gpio_set_drive_capability((gpio_num_t)gnd_gpio, GPIO_DRIVE_CAP_3);
    digitalWrite(gnd_gpio, LOW);

    delay(5);

    const uint8_t pins_16[] = {1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15};
    uint8_t detected_16 = 0;

    for (uint8_t p : pins_16) {
        uint8_t gpio = zif_to_gpio(p);
        if (gpio == 0) continue;

        pinMode(gpio, INPUT_PULLUP);
        delayMicroseconds(100);
        bool val_pullup = (digitalRead(gpio) == HIGH);

        pinMode(gpio, INPUT_PULLDOWN);
        delayMicroseconds(100);
        bool val_pulldown = (digitalRead(gpio) == HIGH);

        if (val_pullup == val_pulldown) {
            detected_16++;
        }
    }

    zif_hal_release_all();

    return (detected_16 >= 2);
}


