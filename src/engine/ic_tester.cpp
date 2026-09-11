/**
 * ic_tester.cpp — IC Test Engine Implementation
 *
 * Test sequence for each gate:
 *   1. Set VCC and GND pins
 *   2. Set all other pins as INPUT (high-Z)
 *   3. For each truth table combination:
 *      a. Drive input pins
 *      b. Wait for propagation (1µs typical for 74HCxx)
 *      c. Read output pin
 *      d. Compare with expected value
 *   4. Record PASS/FAIL per row
 *   5. Release all pins (safe state)
 */
#include "ic_tester.h"
#include "../hal/zif_hal.h"
#include <Arduino.h>

// Propagation delay after setting inputs before reading output
// 74HCxx: typ 7ns, max 25ns. We use 10µs to be safe with ESP32 GPIO overhead.
constexpr uint32_t PROP_DELAY_US = 10;

// ─── Gate type string ─────────────────────────────────────────────────────
const char *ic_tester_gate_type_str(GateType type) {
    switch (type) {
        case GateType::AND:    return "AND";
        case GateType::OR:     return "OR";
        case GateType::NOT:    return "NOT";
        case GateType::NAND:   return "NAND";
        case GateType::NOR:    return "NOR";
        case GateType::XOR:    return "XOR";
        case GateType::XNOR:   return "XNOR";
        case GateType::BUFFER: return "BUF";
        default:               return "???";
    }
}

// ─── Main test function ───────────────────────────────────────────────────
bool ic_tester_run(
    const ICDescriptor  *desc,
    ICTestResult        &result,
    TestProgressCallback progress)
{
    if (!desc) return false;

    // --- Initialise result struct ---
    result.ic           = desc;
    result.overall_pass = true;
    result.num_gates    = desc->num_gates;
    result.timestamp    = millis();

    uint32_t t_start = millis();

    // === STEP 1: Release all ZIF pins to safe state ===
    zif_hal_release_all();

    // === STEP 2: Apply VCC and GND ===
    zif_hal_configure(desc->vcc_pin, ZIFPinRole::VCC);
    zif_hal_configure(desc->gnd_pin, ZIFPinRole::GND);
    delayMicroseconds(500); // Let supply stabilise

    // === STEP 3: Test each gate ===
    for (uint8_t g = 0; g < desc->num_gates; g++) {
        const ICGate &gate = desc->gates[g];
        GateResult   &gr   = result.gate_results[g];

        gr.gate_id    = gate.id;
        gr.type       = gate.type;
        gr.gate_pass  = true;
        gr.output_pin = gate.output_pin;
        gr.input_pins[0] = gate.input_pins[0];
        gr.input_pins[1] = gate.input_pins[1];

        // Configure output pin as DUT_OUTPUT (read from IC)
        zif_hal_configure(gate.output_pin, ZIFPinRole::DUT_OUTPUT);

        // Configure input pins as DUT_INPUT
        zif_hal_configure(gate.input_pins[0], ZIFPinRole::DUT_INPUT, false);
        if (gate.num_inputs > 1 && gate.input_pins[1] != 0) {
            zif_hal_configure(gate.input_pins[1], ZIFPinRole::DUT_INPUT, false);
        }

        // --- Truth table: iterate all input combinations ---
        uint8_t num_combos = (gate.num_inputs == 1) ? 2 : 4;
        gr.num_rows = num_combos;

        for (uint8_t combo = 0; combo < num_combos; combo++) {
            uint8_t in_a = (combo >> 0) & 1;
            uint8_t in_b = (combo >> 1) & 1;

            // Apply inputs
            zif_hal_write(gate.input_pins[0], in_a == 1);
            if (gate.num_inputs > 1 && gate.input_pins[1] != 0) {
                zif_hal_write(gate.input_pins[1], in_b == 1);
            }

            delayMicroseconds(PROP_DELAY_US); // Propagation delay

            // Read output
            uint8_t actual   = zif_hal_read(gate.output_pin) ? 1 : 0;
            uint8_t expected = ic_db_compute_expected(gate.type, in_a, in_b);
            bool    row_pass = (actual == expected);

            gr.rows[combo] = {
                in_a, in_b, expected, actual, row_pass
            };

            if (!row_pass) {
                gr.gate_pass    = false;
                result.overall_pass = false;
            }
        }

        // Float input pins after testing this gate (before next gate)
        zif_hal_configure(gate.input_pins[0], ZIFPinRole::FLOAT);
        if (gate.num_inputs > 1 && gate.input_pins[1] != 0) {
            zif_hal_configure(gate.input_pins[1], ZIFPinRole::FLOAT);
        }
        zif_hal_configure(gate.output_pin, ZIFPinRole::FLOAT);

        // Progress callback
        if (progress) progress(g + 1, desc->num_gates);

        // Small delay between gates
        delay(5);
    }

    // === STEP 4: Safe release ===
    zif_hal_release_all();

    result.test_duration_ms = millis() - t_start;

    Serial.printf("[TEST] %s — %s in %lums\n",
        desc->ic_number,
        result.overall_pass ? "PASS" : "FAIL",
        result.test_duration_ms);

    return true;
}
