/**
 * auto_detect.cpp — IC Auto-Detection Implementation
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

    // Apply VCC and GND
    zif_hal_release_all();
    zif_hal_configure(desc->vcc_pin, ZIFPinRole::VCC);
    zif_hal_configure(desc->gnd_pin, ZIFPinRole::GND);
    delayMicroseconds(500);

    for (uint8_t g = 0; g < desc->num_gates; g++) {
        const ICGate &gate = desc->gates[g];

        // Set up pins
        zif_hal_configure(gate.output_pin, ZIFPinRole::DUT_OUTPUT);
        zif_hal_configure(gate.input_pins[0], ZIFPinRole::DUT_INPUT, false);
        if (gate.num_inputs > 1 && gate.input_pins[1] != 0) {
            zif_hal_configure(gate.input_pins[1], ZIFPinRole::DUT_INPUT, false);
        }

        uint8_t num_combos = (gate.num_inputs == 1) ? 2 : 4;
        for (uint8_t c = 0; c < num_combos; c++) {
            uint8_t in_a = (c >> 0) & 1;
            uint8_t in_b = (c >> 1) & 1;

            zif_hal_write(gate.input_pins[0], in_a == 1);
            if (gate.num_inputs > 1 && gate.input_pins[1] != 0) {
                zif_hal_write(gate.input_pins[1], in_b == 1);
            }

            delayMicroseconds(10);
            uint8_t actual   = zif_hal_read(gate.output_pin) ? 1 : 0;
            uint8_t expected = ic_db_compute_expected(gate.type, in_a, in_b);

            total_rows++;
            if (actual == expected) pass_rows++;
        }

        // Float pins
        zif_hal_configure(gate.input_pins[0], ZIFPinRole::FLOAT);
        if (gate.num_inputs > 1 && gate.input_pins[1] != 0) {
            zif_hal_configure(gate.input_pins[1], ZIFPinRole::FLOAT);
        }
        zif_hal_configure(gate.output_pin, ZIFPinRole::FLOAT);
    }

    zif_hal_release_all();

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
    }

    if (confidence_out) *confidence_out = best_score;

    // Only return a match if confidence >= 75%
    if (best_score >= 75) {
        Serial.printf("[DETECT] Identified: %s (%d%% confidence)\n",
            best_match->ic_number, best_score);
        return best_match;
    }

    Serial.println("[DETECT] No IC identified");
    return nullptr;
}
