#include "diagnostics.h"
#include "app_config.h"
#include "esp_log.h"
#include "esp_flash.h"
#include "esp_partition.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_ota_ops.h"
#if CONFIG_SPIRAM
#include "esp_psram.h"
#endif

static const char* TAG = "HW";

bool diagnostics_perform_hardware_check(void) {
    ESP_LOGI("BOOT", "[BOOT] Erebus bridge starting");
    ESP_LOGI(TAG, "[HW] Target: %s", EREBUS_TARGET_NAME);

    // 1. Flash Detection
    uint32_t flash_size_bytes = 0;
    esp_err_t flash_err = esp_flash_get_size(NULL, &flash_size_bytes);
    uint32_t flash_size_mb = (flash_err == ESP_OK) ? (flash_size_bytes / (1024 * 1024)) : 0;
    ESP_LOGI(TAG, "[HW] Flash detected: %u MB", (unsigned int)flash_size_mb);

    // 2. PSRAM Detection & Initialization
    bool psram_initialized = false;
    size_t psram_size_bytes = 0;

#if CONFIG_SPIRAM
    psram_size_bytes = esp_psram_get_size();
    psram_initialized = (psram_size_bytes > 0);
    size_t psram_size_mb = psram_size_bytes / (1024 * 1024);
    ESP_LOGI(TAG, "[HW] PSRAM detected: %u MB OPI", (unsigned int)psram_size_mb);
    ESP_LOGI(TAG, "[HW] PSRAM initialized: %s", psram_initialized ? "yes" : "no");
#else
    ESP_LOGE(TAG, "[HW] PSRAM detected: none (CONFIG_SPIRAM not enabled)");
    ESP_LOGE(TAG, "[HW] PSRAM initialized: no");
#endif

    // 3. Heap free diagnostics
    size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    ESP_LOGI(TAG, "[HW] Internal heap free: %u bytes", (unsigned int)free_internal);
    ESP_LOGI(TAG, "[HW] PSRAM heap free: %u bytes", (unsigned int)free_psram);

    // 4. Running Partition
    const esp_partition_t* running = esp_ota_get_running_partition();
    if (running) {
        ESP_LOGI(TAG, "[HW] Running partition: %s (offset: 0x%lx, size: %lu KB)",
                 running->label, (unsigned long)running->address, (unsigned long)(running->size / 1024));
    } else {
        const esp_partition_t* part = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
        ESP_LOGI(TAG, "[HW] Running partition: %s", part ? part->label : "unknown");
    }

    // Strict validation
    if (!psram_initialized || psram_size_bytes < (EREBUS_EXPECTED_PSRAM_MB * 1024 * 1024)) {
        ESP_LOGE(TAG, "[ERROR] Critical: 8 MB OPI PSRAM is REQUIRED for Erebus buffer pipelines!");
        ESP_LOGE(TAG, "[ERROR] Board must be ESP32-S3 N16R8. Halting execution to prevent silent corruption.");
        return false;
    }

    if (flash_size_mb < EREBUS_EXPECTED_FLASH_MB) {
        ESP_LOGW(TAG, "[WARNING] Flash detected (%u MB) is less than configured (%u MB)",
                 (unsigned int)flash_size_mb, EREBUS_EXPECTED_FLASH_MB);
    }

    return true;
}

void diagnostics_log_memory_status(void) {
    size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    ESP_LOGI(TAG, "[HW-METRICS] Free Internal: %u KB | Free PSRAM: %u KB",
             (unsigned int)(free_internal / 1024), (unsigned int)(free_psram / 1024));
}
