#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Perform mandatory hardware checks for ESP32-S3 N16R8:
 * - Flash size verification (16 MB)
 * - PSRAM initialization and detection (8 MB OPI)
 * - Heap diagnostics (Internal + PSRAM)
 * - Running partition validation
 *
 * Returns true if hardware matches N16R8 requirements, false otherwise.
 */
bool diagnostics_perform_hardware_check(void);

/**
 * Log current memory and heap metrics.
 */
void diagnostics_log_memory_status(void);

#ifdef __cplusplus
}
#endif
