/**
 * auto_detect.cpp — IC Auto-Detection Implementation
 *
 * Reliability improvements:
 *   - Increased VCC/GND settling delay (5ms instead of 500µs)
 *   - Increased propagation delay (50µs instead of 10µs)
 *   - Proper gpio_reset_pin-based release between candidates
 *   - Settling delay after configuring DUT_OUTPUT before reading
 *   - Multi-sample reads via zif_hal_read (majority voting)
 */
#include "auto_detect.h"
#include "ic_tester.h"
#include "../hal/zif_hal.h"
#include <Arduino.h>

// ─── Score one IC candidate ───────────────────────────────────────────────
// Returns match score 0-100 (% of truth table rows that matched)
static uint8_t score_candidate(const ICDescriptor *desc) {
    uint16_t total_rows = 0;
    uint16_t pass_rows  = 0;
    uint8_t  pc         = desc->pin_count;

    // Full discharge reset before testing this candidate
    zif_hal_power_down();

    // Pre-ground ALL candidate input pins to prevent CMOS floating inputs
    for (uint8_t p = 0; p < desc->pin_count; p++) {
        const ICPin &pin = desc->pins[p];
        uint8_t phys_pin = physical_zif(pin.zif_pin, pc);
        if (pin.role == PinRole::PIN_IN) {
            zif_hal_configure(phys_pin, ZIFPinRole::DUT_INPUT, false); // Driven LOW (0V)
        } else if (pin.role == PinRole::PIN_OUT) {
            zif_hal_configure(phys_pin, ZIFPinRole::DUT_OUTPUT); // Safe listening mode
        }
    }

    // Apply VCC and GND using physical ZIF mapping with proper settling
    zif_hal_configure(physical_zif(desc->vcc_pin, pc), ZIFPinRole::VCC);
    zif_hal_configure(physical_zif(desc->gnd_pin, pc), ZIFPinRole::GND);
    delay(10); // 10ms for IC power rail to fully stabilise

    for (uint8_t g = 0; g < desc->num_gates; g++) {
        const ICGate &gate = desc->gates[g];
        uint8_t phys_out = physical_zif(gate.output_pin, pc);
        uint8_t phys_in0 = physical_zif(gate.input_pins[0], pc);
        uint8_t phys_in1 = (gate.num_inputs > 1 && gate.input_pins[1] != 0)
                           ? physical_zif(gate.input_pins[1], pc) : 0;
        uint8_t phys_in2 = (gate.num_inputs > 2 && gate.input_pins[2] != 0)
                           ? physical_zif(gate.input_pins[2], pc) : 0;

        // Ensure current gate pins are configured
        zif_hal_configure(phys_out, ZIFPinRole::DUT_OUTPUT);
        zif_hal_configure(phys_in0, ZIFPinRole::DUT_INPUT, false);
        if (phys_in1 != 0) zif_hal_configure(phys_in1, ZIFPinRole::DUT_INPUT, false);
        if (phys_in2 != 0) zif_hal_configure(phys_in2, ZIFPinRole::DUT_INPUT, false);

        // Brief settle after pin configuration
        delayMicroseconds(200);

        uint8_t num_combos = (gate.num_inputs == 1) ? 2 : (gate.num_inputs == 2 ? 4 : 8);
        for (uint8_t c = 0; c < num_combos; c++) {
            uint8_t in_a = (c >> 0) & 1;
            uint8_t in_b = (c >> 1) & 1;
            uint8_t in_c = (c >> 2) & 1;

            zif_hal_write(phys_in0, in_a == 1);
            if (phys_in1 != 0) zif_hal_write(phys_in1, in_b == 1);
            if (phys_in2 != 0) zif_hal_write(phys_in2, in_c == 1);

            delayMicroseconds(50); // Propagation delay (74HC max ~25ns + ZIF trace parasitic)
            
            uint8_t actual   = zif_hal_read(phys_out) ? 1 : 0;
            uint8_t expected = ic_db_compute_expected(gate.type, in_a, in_b, in_c, gate.num_inputs);

            total_rows++;
            if (actual == expected) pass_rows++;
        }

        // Return tested inputs to LOW (0V) — do NOT float them
        zif_hal_write(phys_in0, false);
        if (phys_in1 != 0) zif_hal_write(phys_in1, false);
        if (phys_in2 != 0) zif_hal_write(phys_in2, false);
    }

    zif_hal_power_down();

    if (total_rows == 0) return 0;
    return (uint8_t)((pass_rows * 100) / total_rows);
}

// ─── Public API ───────────────────────────────────────────────────────────
const ICDescriptor *auto_detect_ic(uint8_t *confidence_out) {
    const ICDescriptor *best_match  = nullptr;
    uint8_t             best_score  = 0;

    Serial.println("[DETECT] Starting auto-detection...");

    uint8_t db_size = ic_db_count();
    for (uint8_t i = 0; i < db_size; i++) {
        const ICDescriptor *candidate = ic_db_get(i);
        uint8_t score = score_candidate(candidate);

        Serial.printf("[DETECT]   %s: %d%%\n", candidate->ic_number, score);

        if (score > best_score) {
            best_score  = score;
            best_match  = candidate;
        }

        // Perfect match — stop early
        if (score == 100) break;

        // Yield to FreeRTOS to keep LVGL animations smooth
        delay(2);
    }

    if (confidence_out) *confidence_out = best_score;

    // Only return a match if confidence >= 80%
    if (best_score >= 80) {
        Serial.printf("[DETECT] Identified: %s (%d%% confidence)\n",
            best_match->ic_number, best_score);
        return best_match;
    }

    Serial.println("[DETECT] No IC identified");
    return nullptr;
}

