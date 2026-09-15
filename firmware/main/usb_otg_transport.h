#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t usb_otg_transport_init(void);
bool usb_otg_transport_is_connected(void);
void usb_otg_transport_deinit(void);

#ifdef __cplusplus
}
#endif
