/**
 * auto_detect.h — IC Auto-Detection Module
 *
 * Identifies an unknown IC in the ZIF socket by probing its
 * outputs against all known IC patterns in the database.
 *
 * Algorithm:
 *   1. Try 14-pin configuration (VCC=14, GND=7) first
 *   2. For each IC in database: run a "quick test" (subset of truth table)
 *   3. Score each IC by number of matching outputs
 *   4. Return the IC with score == 100% (exact match)
 *   5. If no exact match, try 16-pin config (VCC=16, GND=8)
 *   6. Return nullptr if no match found
 */
#pragma once
#include "ic_database.h"

/**
 * @brief Attempt to auto-detect the IC in the ZIF socket.
 *
 * @param confidence_out  Optional: 0-100% match confidence of best candidate
 * @return Pointer to matched ICDescriptor, or nullptr if not identified.
 */
const ICDescriptor *auto_detect_ic(uint8_t *confidence_out = nullptr);
