/**
 * zif_hal.h — ZIF Socket GPIO Hardware Abstraction Layer
 * Controls the 16-pin ZIF socket for IC insertion and testing.
 *
 * SAFETY NOTE:
 *   VCC is sourced directly from ESP32-S3 GPIO (3.3V, ~12mA max).
 *   Always set non-test pins to INPUT (high-Z) before driving VCC/GND
 *   to avoid short-circuit damage.
 */
#pragma once
#include <Arduino.h>
#include <cstdint>

// Pin role during test
enum class ZIFPinRole : uint8_t {
    FLOAT = 0,    // High-Z (INPUT, default safe state)
    VCC,          // Drive HIGH as supply voltage
    GND,          // Drive LOW as ground
    DUT_INPUT,    // Drive LOW or HIGH (test input)
    DUT_OUTPUT,   // Read back (test output)
};

/**
 * @brief Initialise all ZIF pins as INPUT (high-Z, safe default).
 */
void zif_hal_init(void);

/**
 * @brief Set all ZIF pins to safe INPUT state.
 *        Call before/after every test to protect IC and MCU.
 */
void zif_hal_release_all(void);

/**
 * @brief Configure a single ZIF pin.
 * @param zif_pin  ZIF pin number 1–16
 * @param role     FLOAT, VCC, GND, DUT_INPUT, DUT_OUTPUT
 * @param value    Initial level for DUT_INPUT / VCC / GND (true=HIGH, false=LOW)
 */
void zif_hal_configure(uint8_t zif_pin, ZIFPinRole role, bool value = false);

/**
 * @brief Drive a ZIF pin HIGH or LOW (must be configured as DUT_INPUT first).
 * @param zif_pin  ZIF pin number 1–16
 * @param value    true = HIGH (logic 1), false = LOW (logic 0)
 */
void zif_hal_write(uint8_t zif_pin, bool value);

/**
 * @brief Read the logic level on a ZIF pin (must be configured as DUT_OUTPUT first).
 * @param zif_pin  ZIF pin number 1–16
 * @return true = HIGH (logic 1), false = LOW (logic 0)
 */
bool zif_hal_read(uint8_t zif_pin);

/**
 * @brief Check if any non-VCC/GND pin is being driven externally.
 *        Used to detect IC insertion (rough heuristic).
 * @return true if an IC signature is detected on the ZIF socket.
 */
bool zif_hal_detect_ic_presence(void);

/**
 * @brief Map logical IC pin to physical ZIF socket pin.
 * For a 16-pin ZIF socket, if a 14-pin IC is inserted top-aligned (Pin 1 to Pin 1):
 * - Left side: IC pins 1-7 go to ZIF pins 1-7
 * - Right side: IC pins 8-14 go to ZIF pins 10-16 (ZIF pins 8 and 9 are empty)
 */
static inline uint8_t physical_zif(uint8_t ic_pin, uint8_t pin_count) {
    if (pin_count == 14) {
        if (ic_pin >= 8 && ic_pin <= 14) {
            return ic_pin + 2;
        }
    }
    return ic_pin;
}
