#pragma once

#include "esp_err.h"
#include <cstdint>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t protocol_router_init(void);
esp_err_t protocol_router_process(void);
void protocol_router_reset(void);

#ifdef __cplusplus
}
#endif
