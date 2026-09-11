/**
 * ic_database.cpp — IC Descriptor Definitions
 *
 * Adding a new IC:
 *   1. Define its pins array (ICPin[])
 *   2. Define its gates array (ICGate[])
 *   3. Define its ICDescriptor
 *   4. Add a pointer to ic_database[]
 */
#include "ic_database.h"
#include <cstring>

// ═══════════════════════════════════════════════════════════════════════════
//  7408 — Quad 2-Input AND Gate (DIP-14)
//  Pinout: 1A-1, 1B-2, 1Y-3, 2A-4, 2B-5, 2Y-6, GND-7,
//           3Y-8, 3A-9, 3B-10, 4Y-11, 4A-12, 4B-13, VCC-14
// ═══════════════════════════════════════════════════════════════════════════
static const ICPin pins_7408[] = {
    {  1, PinRole::PIN_IN,  "1A"  },
    {  2, PinRole::PIN_IN,  "1B"  },
    {  3, PinRole::PIN_OUT, "1Y"  },
    {  4, PinRole::PIN_IN,  "2A"  },
    {  5, PinRole::PIN_IN,  "2B"  },
    {  6, PinRole::PIN_OUT, "2Y"  },
    {  7, PinRole::GND,    "GND" },
    {  8, PinRole::PIN_OUT, "3Y"  },
    {  9, PinRole::PIN_IN,  "3A"  },
    { 10, PinRole::PIN_IN,  "3B"  },
    { 11, PinRole::PIN_OUT, "4Y"  },
    { 12, PinRole::PIN_IN,  "4A"  },
    { 13, PinRole::PIN_IN,  "4B"  },
    { 14, PinRole::VCC,    "VCC" },
};
static const ICGate gates_7408[] = {
    { 1, GateType::AND, { 1,  2},  3, 2 },
    { 2, GateType::AND, { 4,  5},  6, 2 },
    { 3, GateType::AND, { 9, 10},  8, 2 },
    { 4, GateType::AND, {12, 13}, 11, 2 },
};
static const ICDescriptor ic_7408 = {
    "7408", "Quad 2-Input AND Gate",
    "4x AND gates, active-high output",
    14, 4, pins_7408, gates_7408, 14, 7
};

// ═══════════════════════════════════════════════════════════════════════════
//  7400 — Quad 2-Input NAND Gate (DIP-14)
//  Pinout: 1A-1, 1B-2, 1Y-3, 2A-4, 2B-5, 2Y-6, GND-7,
//           3Y-8, 3A-9, 3B-10, 4Y-11, 4A-12, 4B-13, VCC-14
// ═══════════════════════════════════════════════════════════════════════════
static const ICPin pins_7400[] = {
    {  1, PinRole::PIN_IN,  "1A"  },
    {  2, PinRole::PIN_IN,  "1B"  },
    {  3, PinRole::PIN_OUT, "1Y"  },
    {  4, PinRole::PIN_IN,  "2A"  },
    {  5, PinRole::PIN_IN,  "2B"  },
    {  6, PinRole::PIN_OUT, "2Y"  },
    {  7, PinRole::GND,    "GND" },
    {  8, PinRole::PIN_OUT, "3Y"  },
    {  9, PinRole::PIN_IN,  "3A"  },
    { 10, PinRole::PIN_IN,  "3B"  },
    { 11, PinRole::PIN_OUT, "4Y"  },
    { 12, PinRole::PIN_IN,  "4A"  },
    { 13, PinRole::PIN_IN,  "4B"  },
    { 14, PinRole::VCC,    "VCC" },
};
static const ICGate gates_7400[] = {
    { 1, GateType::NAND, { 1,  2},  3, 2 },
    { 2, GateType::NAND, { 4,  5},  6, 2 },
    { 3, GateType::NAND, { 9, 10},  8, 2 },
    { 4, GateType::NAND, {12, 13}, 11, 2 },
};
static const ICDescriptor ic_7400 = {
    "7400", "Quad 2-Input NAND Gate",
    "4x NAND gates, inverted AND output",
    14, 4, pins_7400, gates_7400, 14, 7
};

// ═══════════════════════════════════════════════════════════════════════════
//  7402 — Quad 2-Input NOR Gate (DIP-14)
//  NOTE: Different pin order — outputs are at pins 1, 4, 10, 13!
//  Pinout: 1Y-1, 1A-2, 1B-3, 2Y-4, 2A-5, 2B-6, GND-7,
//           3A-8, 3B-9, 3Y-10, 4A-11, 4B-12, 4Y-13, VCC-14
// ═══════════════════════════════════════════════════════════════════════════
static const ICPin pins_7402[] = {
    {  1, PinRole::PIN_OUT, "1Y"  },
    {  2, PinRole::PIN_IN,  "1A"  },
    {  3, PinRole::PIN_IN,  "1B"  },
    {  4, PinRole::PIN_OUT, "2Y"  },
    {  5, PinRole::PIN_IN,  "2A"  },
    {  6, PinRole::PIN_IN,  "2B"  },
    {  7, PinRole::GND,    "GND" },
    {  8, PinRole::PIN_IN,  "3A"  },
    {  9, PinRole::PIN_IN,  "3B"  },
    { 10, PinRole::PIN_OUT, "3Y"  },
    { 11, PinRole::PIN_IN,  "4A"  },
    { 12, PinRole::PIN_IN,  "4B"  },
    { 13, PinRole::PIN_OUT, "4Y"  },
    { 14, PinRole::VCC,    "VCC" },
};
static const ICGate gates_7402[] = {
    { 1, GateType::NOR, { 2,  3},  1, 2 },
    { 2, GateType::NOR, { 5,  6},  4, 2 },
    { 3, GateType::NOR, { 8,  9}, 10, 2 },
    { 4, GateType::NOR, {11, 12}, 13, 2 },
};
static const ICDescriptor ic_7402 = {
    "7402", "Quad 2-Input NOR Gate",
    "4x NOR gates, inverted OR output",
    14, 4, pins_7402, gates_7402, 14, 7
};

// ═══════════════════════════════════════════════════════════════════════════
//  7404 — Hex Inverter / NOT Gate (DIP-14)
//  Pinout: 1A-1, 1Y-2, 2A-3, 2Y-4, 3A-5, 3Y-6, GND-7,
//           4Y-8, 4A-9, 5Y-10, 5A-11, 6Y-12, 6A-13, VCC-14
// ═══════════════════════════════════════════════════════════════════════════
static const ICPin pins_7404[] = {
    {  1, PinRole::PIN_IN,  "1A"  },
    {  2, PinRole::PIN_OUT, "1Y"  },
    {  3, PinRole::PIN_IN,  "2A"  },
    {  4, PinRole::PIN_OUT, "2Y"  },
    {  5, PinRole::PIN_IN,  "3A"  },
    {  6, PinRole::PIN_OUT, "3Y"  },
    {  7, PinRole::GND,    "GND" },
    {  8, PinRole::PIN_OUT, "4Y"  },
    {  9, PinRole::PIN_IN,  "4A"  },
    { 10, PinRole::PIN_OUT, "5Y"  },
    { 11, PinRole::PIN_IN,  "5A"  },
    { 12, PinRole::PIN_OUT, "6Y"  },
    { 13, PinRole::PIN_IN,  "6A"  },
    { 14, PinRole::VCC,    "VCC" },
};
static const ICGate gates_7404[] = {
    { 1, GateType::NOT, { 1, 0},  2, 1 },
    { 2, GateType::NOT, { 3, 0},  4, 1 },
    { 3, GateType::NOT, { 5, 0},  6, 1 },
    { 4, GateType::NOT, { 9, 0},  8, 1 },
    { 5, GateType::NOT, {11, 0}, 10, 1 },
    { 6, GateType::NOT, {13, 0}, 12, 1 },
};
static const ICDescriptor ic_7404 = {
    "7404", "Hex Inverter (NOT Gate)",
    "6x NOT gates (inverters)",
    14, 6, pins_7404, gates_7404, 14, 7
};

// ═══════════════════════════════════════════════════════════════════════════
//  7432 — Quad 2-Input OR Gate (DIP-14)
//  Same pinout as 7408 (AND)
// ═══════════════════════════════════════════════════════════════════════════
static const ICPin pins_7432[] = {
    {  1, PinRole::PIN_IN,  "1A"  },
    {  2, PinRole::PIN_IN,  "1B"  },
    {  3, PinRole::PIN_OUT, "1Y"  },
    {  4, PinRole::PIN_IN,  "2A"  },
    {  5, PinRole::PIN_IN,  "2B"  },
    {  6, PinRole::PIN_OUT, "2Y"  },
    {  7, PinRole::GND,    "GND" },
    {  8, PinRole::PIN_OUT, "3Y"  },
    {  9, PinRole::PIN_IN,  "3A"  },
    { 10, PinRole::PIN_IN,  "3B"  },
    { 11, PinRole::PIN_OUT, "4Y"  },
    { 12, PinRole::PIN_IN,  "4A"  },
    { 13, PinRole::PIN_IN,  "4B"  },
    { 14, PinRole::VCC,    "VCC" },
};
static const ICGate gates_7432[] = {
    { 1, GateType::OR, { 1,  2},  3, 2 },
    { 2, GateType::OR, { 4,  5},  6, 2 },
    { 3, GateType::OR, { 9, 10},  8, 2 },
    { 4, GateType::OR, {12, 13}, 11, 2 },
};
static const ICDescriptor ic_7432 = {
    "7432", "Quad 2-Input OR Gate",
    "4x OR gates",
    14, 4, pins_7432, gates_7432, 14, 7
};

// ═══════════════════════════════════════════════════════════════════════════
//  7486 — Quad 2-Input XOR Gate (DIP-14)
//  Same pinout as 7408 (AND)
// ═══════════════════════════════════════════════════════════════════════════
static const ICPin pins_7486[] = {
    {  1, PinRole::PIN_IN,  "1A"  },
    {  2, PinRole::PIN_IN,  "1B"  },
    {  3, PinRole::PIN_OUT, "1Y"  },
    {  4, PinRole::PIN_IN,  "2A"  },
    {  5, PinRole::PIN_IN,  "2B"  },
    {  6, PinRole::PIN_OUT, "2Y"  },
    {  7, PinRole::GND,    "GND" },
    {  8, PinRole::PIN_OUT, "3Y"  },
    {  9, PinRole::PIN_IN,  "3A"  },
    { 10, PinRole::PIN_IN,  "3B"  },
    { 11, PinRole::PIN_OUT, "4Y"  },
    { 12, PinRole::PIN_IN,  "4A"  },
    { 13, PinRole::PIN_IN,  "4B"  },
    { 14, PinRole::VCC,    "VCC" },
};
static const ICGate gates_7486[] = {
    { 1, GateType::XOR, { 1,  2},  3, 2 },
    { 2, GateType::XOR, { 4,  5},  6, 2 },
    { 3, GateType::XOR, { 9, 10},  8, 2 },
    { 4, GateType::XOR, {12, 13}, 11, 2 },
};
static const ICDescriptor ic_7486 = {
    "7486", "Quad 2-Input XOR Gate",
    "4x XOR gates (exclusive OR)",
    14, 4, pins_7486, gates_7486, 14, 7
};

// ═══════════════════════════════════════════════════════════════════════════
//  Database
// ═══════════════════════════════════════════════════════════════════════════
static const ICDescriptor *ic_database[] = {
    &ic_7408,
    &ic_7400,
    &ic_7402,
    &ic_7404,
    &ic_7432,
    &ic_7486,
};
constexpr uint8_t IC_DB_SIZE = sizeof(ic_database) / sizeof(ic_database[0]);

// ─── API Implementation ───────────────────────────────────────────────────

uint8_t ic_db_compute_expected(GateType type, uint8_t a, uint8_t b) {
    switch (type) {
        case GateType::AND:    return a & b;
        case GateType::OR:     return a | b;
        case GateType::NOT:    return !a;
        case GateType::NAND:   return !(a & b);
        case GateType::NOR:    return !(a | b);
        case GateType::XOR:    return a ^ b;
        case GateType::XNOR:   return !(a ^ b);
        case GateType::BUFFER: return a;
        default:               return 0;
    }
}

uint8_t ic_db_count(void) {
    return IC_DB_SIZE;
}

const ICDescriptor *ic_db_get(uint8_t index) {
    if (index >= IC_DB_SIZE) return nullptr;
    return ic_database[index];
}

const ICDescriptor *ic_db_find(const char *ic_number) {
    for (uint8_t i = 0; i < IC_DB_SIZE; i++) {
        if (strcmp(ic_database[i]->ic_number, ic_number) == 0) {
            return ic_database[i];
        }
    }
    return nullptr;
}
