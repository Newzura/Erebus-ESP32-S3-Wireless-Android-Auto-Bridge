#pragma once

#include "esp_err.h"
#include "esp_netif.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize ESP-NETIF, event loop, and Wi-Fi stack for Erebus.
 */
esp_err_t wifi_transport_init(void);

/**
 * Configure and start the Erebus SoftAP (SSID "Erebus", IP 192.168.4.1, DHCP server).
 * Safe against password leakage in UART logs.
 */
esp_err_t wifi_transport_start_ap(void);

/**
 * Check if a client station (e.g. phone) is currently connected to the AP.
 */
bool wifi_transport_is_client_connected(void);

/**
 * Get the current connected client station count.
 */
uint8_t wifi_transport_get_client_count(void);

/**
 * Stop Wi-Fi transport.
 */
void wifi_transport_stop(void);

#ifdef __cplusplus
}
#endif

