/**
 * ic_tester.cpp — IC Test Engine Implementation
 *
 * Reliability improvements:
 *   - 5ms VCC/GND stabilisation delay (was 500µs)
 *   - 50µs propagation delay (was 10µs)
 *   - 200µs settling delay after pin configuration
 *   - Multi-sample reads via zif_hal_read (majority voting in HAL)
 *   - gpio_reset_pin-based release_all for clean state
 *
 * Test sequence for each gate:
 *   1. Set VCC and GND pins
 *   2. Set all other pins as INPUT (high-Z)
 *   3. For each truth table combination:
 *      a. Drive input pins
 *      b. Wait for propagation (50µs — safe for 74HCxx + ZIF parasitic)
 *      c. Read output pin (3-sample majority vote in HAL)
 *      d. Compare with expected value
 *   4. Record PASS/FAIL per row
 *   5. Release all pins (safe state)
 */
#include "ic_tester.h"
#include "../hal/zif_hal.h"
#include <Arduino.h>

// Propagation delay after setting inputs before reading output
// 74HCxx: typ 7ns, max 25ns. We use 50µs to be safe with:
//   - ESP32 GPIO transition slew rate
//   - ZIF socket parasitic capacitance
//   - IC internal gate delays (cascaded in some ICs)
constexpr uint32_t PROP_DELAY_US = 50;

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
    memset(&result, 0, sizeof(result));
    result.ic           = desc;
    result.overall_pass = true;
    result.num_gates    = desc->num_gates;
    result.timestamp    = millis();

    uint32_t t_start = millis();
    uint8_t pc = desc->pin_count;

    // === STEP 1: Cold power-cycle reset (active discharge) ===
    // Drains any residual capacitance/charge from previous test down to 0V
    zif_hal_power_down();

    // === STEP 2: Pre-ground ALL IC input pins and set outputs to listen ===
    // Critical CMOS rule: NEVER leave unused CMOS inputs floating!
    // Floating inputs drift to VDD/2, causing both PMOS and NMOS to conduct
    // (shoot-through / cross-conduction current >50mA across idle gates),
    // which collapses the ESP32 GPIO VCC rail and produces false fails.
    // Driving all unused inputs LOW holds them in 0mA quiescent state.
    for (uint8_t p = 0; p < desc->pin_count; p++) {
        const ICPin &pin = desc->pins[p];
        uint8_t phys_pin = physical_zif(pin.zif_pin, pc);
        if (pin.role == PinRole::PIN_IN) {
            zif_hal_configure(phys_pin, ZIFPinRole::DUT_INPUT, false); // Driven LOW (0V)
        } else if (pin.role == PinRole::PIN_OUT) {
            zif_hal_configure(phys_pin, ZIFPinRole::DUT_OUTPUT); // Safe listening mode
        }
    }

    // === STEP 3: Apply VCC and GND with max drive (40mA capability) ===
    zif_hal_configure(physical_zif(desc->vcc_pin, pc), ZIFPinRole::VCC);
    zif_hal_configure(physical_zif(desc->gnd_pin, pc), ZIFPinRole::GND);
    delay(10); // 10ms for IC power rail and internal logic to fully stabilize

    // === STEP 4: Test each gate ===
    for (uint8_t g = 0; g < desc->num_gates; g++) {
        const ICGate &gate = desc->gates[g];
        GateResult   &gr   = result.gate_results[g];

        gr.gate_id    = gate.id;
        gr.type       = gate.type;
        gr.gate_pass  = true;
        gr.output_pin = gate.output_pin;
        gr.input_pins[0] = gate.input_pins[0];
        gr.input_pins[1] = gate.input_pins[1];
        gr.input_pins[2] = gate.input_pins[2];

        uint8_t phys_out = physical_zif(gate.output_pin, pc);
        uint8_t phys_in0 = physical_zif(gate.input_pins[0], pc);
        uint8_t phys_in1 = gate.num_inputs > 1 ? physical_zif(gate.input_pins[1], pc) : 0;
        uint8_t phys_in2 = gate.num_inputs > 2 ? physical_zif(gate.input_pins[2], pc) : 0;

        // Ensure current gate pins are configured
        zif_hal_configure(phys_out, ZIFPinRole::DUT_OUTPUT);
        zif_hal_configure(phys_in0, ZIFPinRole::DUT_INPUT, false);
        if (phys_in1 != 0) zif_hal_configure(phys_in1, ZIFPinRole::DUT_INPUT, false);
        if (phys_in2 != 0) zif_hal_configure(phys_in2, ZIFPinRole::DUT_INPUT, false);

        delayMicroseconds(200);

        // --- Truth table: iterate all input combinations ---
        uint8_t num_combos = (gate.num_inputs == 1) ? 2 : (gate.num_inputs == 2 ? 4 : 8);
        gr.num_rows = num_combos;

        for (uint8_t combo = 0; combo < num_combos; combo++) {
            uint8_t in_a = (combo >> 0) & 1;
            uint8_t in_b = (combo >> 1) & 1;
            uint8_t in_c = (combo >> 2) & 1;

            // Apply inputs with max drive strength
            zif_hal_write(phys_in0, in_a == 1);
            if (phys_in1 != 0) zif_hal_write(phys_in1, in_b == 1);
            if (phys_in2 != 0) zif_hal_write(phys_in2, in_c == 1);

            delayMicroseconds(PROP_DELAY_US); // Propagation delay (50µs)

            // Read output (3-sample majority vote)
            uint8_t actual   = zif_hal_read(phys_out) ? 1 : 0;
            uint8_t expected = ic_db_compute_expected(gate.type, in_a, in_b, in_c, gate.num_inputs);
            bool    row_pass = (actual == expected);

            gr.rows[combo] = {
                in_a, in_b, in_c, expected, actual, row_pass
            };

            if (!row_pass) {
                gr.gate_pass        = false;
                result.overall_pass = false;
            }
        }

        // Return current gate's inputs to LOW (0V) — do NOT float them!
        // This maintains a quiet, stable low-power state while testing other gates.
        zif_hal_write(phys_in0, false);
        if (phys_in1 != 0) zif_hal_write(phys_in1, false);
        if (phys_in2 != 0) zif_hal_write(phys_in2, false);

        // Progress callback
        if (progress) progress(g + 1, desc->num_gates);

        delay(2);
    }

    // === STEP 5: Safe power down and active discharge ===
    zif_hal_power_down();

    result.test_duration_ms = millis() - t_start;

    Serial.printf("[TEST] %s — %s in %lums\n",
        desc->ic_number,
        result.overall_pass ? "PASS" : "FAIL",
        result.test_duration_ms);

    return true;
}
