#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "app_config.h"
#include "bridge_state.h"
#include "diagnostics.h"
#include "wifi_transport.h"
#include "usb_otg_transport.h"
#include "protocol_router.h"

static const char* TAG = "MAIN";

static void diagnostics_task(void* pvParameters) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        diagnostics_log_memory_status();
        BridgeStateMachine::instance().logMetrics();
    }
}

extern "C" void app_main(void) {
    // 1. Initialize State Machine
    BridgeStateMachine& stateMachine = BridgeStateMachine::instance();
    stateMachine.init();

    // 2. Initialize Non-Volatile Storage (NVS)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 3. Hardware Diagnostics & Memory Verification
    stateMachine.transitionTo(BridgeState::HARDWARE_CHECK, "Verifying ESP32-S3 N16R8 hardware");
    bool hw_ok = diagnostics_perform_hardware_check();

    if (!hw_ok) {
        stateMachine.transitionTo(BridgeState::ERROR, "Hardware validation failed (Flash/PSRAM mismatch)");
        ESP_LOGE(TAG, "[ERROR] System halted due to critical hardware requirements failure.");
        return;
    }

    stateMachine.transitionTo(BridgeState::PSRAM_READY, "8 MB OPI PSRAM verified and mapped");

    // 4. Initialize Modular Subsystems
    esp_err_t wifi_err = wifi_transport_init();
    if (wifi_err == ESP_OK) {
        wifi_transport_start_ap();
    } else {
        ESP_LOGE(TAG, "Failed to initialize Wi-Fi transport: %s", esp_err_to_name(wifi_err));
    }
    usb_otg_transport_init();
    protocol_router_init();

    // 5. Start Background Diagnostic Monitor Task
    xTaskCreatePinnedToCore(diagnostics_task, "diag_mon", 3072, NULL, 1, NULL, 0);

    ESP_LOGI(TAG, "Erebus ESP32-S3 N16R8 base firmware started successfully.");
}
