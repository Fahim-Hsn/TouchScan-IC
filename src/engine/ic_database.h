/**
 * ic_database.h — IC Descriptor Database
 *
 * Defines the structure for describing 7400-series ICs:
 *   - Pin roles (VCC, GND, Input, Output)
 *   - Gate definitions (type, input pins, output pin)
 *   - Truth tables (auto-generated from gate type)
 *
 * Supported ICs (v1):
 *   7400 — Quad 2-input NAND
 *   7402 — Quad 2-input NOR
 *   7404 — Hex Inverter (NOT)
 *   7408 — Quad 2-input AND
 *   7432 — Quad 2-input OR
 *   7486 — Quad 2-input XOR
 */
#pragma once
#include <Arduino.h>
#include <cstdint>

// ─── Gate Type ────────────────────────────────────────────────────────────
enum class GateType : uint8_t {
    AND  = 0,
    OR,
    NOT,
    NAND,
    NOR,
    XOR,
    XNOR,
    BUFFER,
};

// ─── Pin Role ─────────────────────────────────────────────────────────────
enum class PinRole : uint8_t {
    VCC    = 0,
    GND,
    PIN_IN,
    PIN_OUT,
    NC,       // Not connected
};

// ─── IC Pin Descriptor ────────────────────────────────────────────────────
struct ICPin {
    uint8_t zif_pin;    // ZIF socket pin number (1–16)
    PinRole role;
    const char *label;  // e.g. "1A", "1Y", "VCC", "GND"
};

// ─── Gate Descriptor ─────────────────────────────────────────────────────
struct ICGate {
    uint8_t  id;                  // Gate number (1-based)
    GateType type;
    uint8_t  input_pins[2];       // ZIF pin numbers (0 if unused, e.g. NOT gate)
    uint8_t  output_pin;          // ZIF pin number
    uint8_t  num_inputs;          // 1 (NOT) or 2 (AND, OR, etc.)
};

// ─── IC Descriptor ────────────────────────────────────────────────────────
struct ICDescriptor {
    const char *ic_number;        // e.g. "7408"
    const char *full_name;        // e.g. "Quad 2-Input AND Gate"
    const char *description;      // Brief description
    uint8_t     pin_count;        // 14 or 16
    uint8_t     num_gates;

    const ICPin  *pins;           // Array of pin descriptors (pin_count entries)
    const ICGate *gates;          // Array of gate descriptors (num_gates entries)

    uint8_t vcc_pin;              // ZIF pin number for VCC
    uint8_t gnd_pin;              // ZIF pin number for GND
};

// ─── Truth Table Computation ──────────────────────────────────────────────
/**
 * @brief Compute expected output for a gate given its inputs.
 * @param type     Gate logic type
 * @param a        Input A (0 or 1)
 * @param b        Input B (0 or 1, ignored for NOT/BUFFER)
 * @return         Expected output (0 or 1)
 */
uint8_t ic_db_compute_expected(GateType type, uint8_t a, uint8_t b = 0);

// ─── Database Access ──────────────────────────────────────────────────────
/** @brief Return number of ICs in the database. */
uint8_t ic_db_count(void);

/** @brief Get IC descriptor by index (0-based). */
const ICDescriptor *ic_db_get(uint8_t index);

/** @brief Find IC descriptor by IC number string. Returns nullptr if not found. */
const ICDescriptor *ic_db_find(const char *ic_number);
