/**
 * ic_tester.h — IC Test Engine
 *
 * Drives the ZIF socket to test an IC against its truth table.
 * Reports per-gate, per-combination pass/fail with actual vs expected values.
 */
#pragma once
#include <Arduino.h>
#include "ic_database.h"

// ─── Result Structures ────────────────────────────────────────────────────

/** Single truth-table row result */
struct TTRowResult {
    uint8_t input_a;     // Applied input A
    uint8_t input_b;     // Applied input B (0 if NOT gate)
    uint8_t input_c;     // Applied input C (0 if <= 2 inputs)
    uint8_t expected;    // Expected output
    uint8_t actual;      // Measured output
    bool    pass;        // expected == actual
};

/** Per-gate result (up to 8 combinations for 3-input, 4 for 2-input, 2 for NOT) */
struct GateResult {
    uint8_t     gate_id;          // 1-based gate number
    bool        gate_pass;        // True if ALL rows pass
    uint8_t     num_rows;         // Number of truth table rows
    TTRowResult rows[8];          // Max 8 rows (3-input gate: 8 combinations)
    uint8_t     output_pin;       // ZIF output pin number for this gate
    uint8_t     input_pins[3];    // ZIF input pin numbers
    GateType    type;
};

/** Full IC test result */
struct ICTestResult {
    const ICDescriptor *ic;       // Pointer to IC descriptor
    bool     overall_pass;        // True if ALL gates pass
    uint8_t  num_gates;
    GateResult gate_results[6];   // Max 6 gates (7404 hex inverter)
    uint32_t test_duration_ms;    // Total test time in milliseconds
    uint32_t timestamp;           // millis() at test completion
};

// ─── Progress Callback ────────────────────────────────────────────────────
typedef void (*TestProgressCallback)(uint8_t gate_index, uint8_t total_gates);

// ─── API ──────────────────────────────────────────────────────────────────

/**
 * @brief Run the full truth-table test for a given IC.
 *
 * @param desc      IC descriptor from ic_database
 * @param result    Output: filled with per-gate results
 * @param progress  Optional callback called after each gate is tested
 * @return true if test completed (even if IC fails), false on setup error
 */
bool ic_tester_run(
    const ICDescriptor *desc,
    ICTestResult       &result,
    TestProgressCallback progress = nullptr
);

/**
 * @brief Get a human-readable string for a gate type.
 * @param type  GateType enum
 * @return e.g. "AND", "NAND", "NOT"
 */
const char *ic_tester_gate_type_str(GateType type);
