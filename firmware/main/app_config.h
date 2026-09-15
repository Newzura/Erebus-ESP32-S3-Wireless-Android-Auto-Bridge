#pragma once

#include <cstdint>
#include <cstddef>

#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif

// Hardware specifications for ESP32-S3 N16R8
#define EREBUS_TARGET_NAME          "ESP32-S3 N16R8"
#define EREBUS_EXPECTED_FLASH_MB    16
#define EREBUS_EXPECTED_PSRAM_MB    8

// Network defaults
#define EREBUS_DEFAULT_GATEWAY_IP   "192.168.4.1"
#define EREBUS_DEFAULT_NETMASK      "255.255.255.0"
#define EREBUS_WIFI_CHANNEL         6
#define EREBUS_MAX_STA_CONN         4

// Static Buffer Sizing (avoid dynamic allocations in hot loops)
#define EREBUS_RX_BUFFER_SIZE       (32 * 1024)   // 32 KB
#define EREBUS_TX_BUFFER_SIZE       (32 * 1024)   // 32 KB

// USB Pins for ESP32-S3 Native USB OTG Controller
#define EREBUS_USB_DM_GPIO          19
#define EREBUS_USB_DP_GPIO          20

// Timeouts (in milliseconds)
#define EREBUS_SOCKET_TIMEOUT_MS    5000
#define EREBUS_USB_TIMEOUT_MS       5000
