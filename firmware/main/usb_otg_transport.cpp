#include "usb_otg_transport.h"
#include "app_config.h"
#include "esp_log.h"

static const char* TAG = "USB";
static bool s_usb_connected = false;

esp_err_t usb_otg_transport_init(void) {
    ESP_LOGI(TAG, "USB OTG native transport module registered (Phase 5 foundation)");
    return ESP_OK;
}

bool usb_otg_transport_is_connected(void) {
    return s_usb_connected;
}

void usb_otg_transport_deinit(void) {
    ESP_LOGI(TAG, "USB OTG transport de-initialized");
    s_usb_connected = false;
}
