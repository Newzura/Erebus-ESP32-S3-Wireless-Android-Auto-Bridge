#include "wifi_transport.h"
#include "app_config.h"
#include "bridge_state.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include <cstring>
#include <cinttypes>

static const char* TAG = "WIFI";

static bool s_client_connected = false;
static uint8_t s_connected_stations = 0;
static esp_netif_t* s_ap_netif = nullptr;
static esp_event_handler_instance_t s_wifi_event_instance = nullptr;
static esp_event_handler_instance_t s_ip_event_instance = nullptr;
static bool s_wifi_initialized = false;

// Diagnostics & Stability tracking
static uint32_t s_total_disconnects = 0;
static uint32_t s_station_reconnect_count = 0;
static int64_t s_station_connected_timestamp_us = 0;
static uint8_t s_last_station_mac[6] = {0};
static bool s_has_previous_station = false;

static const char* get_wifi_disassoc_reason_name(uint8_t reason) {
    switch (reason) {
        case 1: return "UNSPECIFIED";
        case 2: return "PREV_AUTH_NOT_VALID";
        case 3: return "DEAUTH_LEAVING";
        case 4: return "DISASSOC_DUE_TO_INACTIVITY";
        case 5: return "DISASSOC_AP_BUSY";
        case 6: return "CLASS2_FRAME_FROM_NONAUTH_STA";
        case 7: return "CLASS3_FRAME_FROM_NONASSOC_STA";
        case 8: return "DISASSOC_STA_HAS_LEFT";
        case 9: return "STA_REQ_ASSOC_WITHOUT_AUTH";
        case 13: return "INVALID_IE";
        case 14: return "MIC_FAILURE";
        case 15: return "4WAY_HANDSHAKE_TIMEOUT";
        case 16: return "GROUP_KEY_UPDATE_TIMEOUT";
        case 17: return "IE_IN_4WAY_DIFFERS";
        case 18: return "GROUP_CIPHER_INVALID";
        case 19: return "PAIRWISE_CIPHER_INVALID";
        case 23: return "IEEE_802_1X_AUTH_FAILED";
        case 200: return "BEACON_TIMEOUT";
        case 201: return "NO_AP_FOUND";
        case 202: return "AUTH_FAIL";
        case 203: return "ASSOC_FAIL";
        case 204: return "HANDSHAKE_TIMEOUT";
        case 205: return "CONNECTION_FAIL";
        case 206: return "AP_TSF_RESET";
        case 207: return "ROAMING";
        default: return "OTHER_REASON";
    }
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    BridgeStateMachine& sm = BridgeStateMachine::instance();

    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_AP_START: {
                ESP_LOGI(TAG, "[WIFI] SoftAP permanently active. Gateway IP: %s, SSID: \"%s\"",
                         EREBUS_DEFAULT_GATEWAY_IP, EREBUS_WIFI_SSID);
                sm.transitionTo(BridgeState::WIFI_AP_READY, "SoftAP active and listening");
                break;
            }
            case WIFI_EVENT_AP_STOP: {
                ESP_LOGW(TAG, "[WIFI] SoftAP stopped");
                s_connected_stations = 0;
                s_client_connected = false;
                sm.transitionTo(BridgeState::WIFI_STARTING, "SoftAP stopped");
                break;
            }
            case WIFI_EVENT_AP_STACONNECTED: {
                wifi_event_ap_staconnected_t* evt = (wifi_event_ap_staconnected_t*)event_data;
                s_connected_stations++;
                s_client_connected = true;
                s_station_connected_timestamp_us = esp_timer_get_time();

                if (s_has_previous_station && memcmp(s_last_station_mac, evt->mac, 6) == 0) {
                    s_station_reconnect_count++;
                } else {
                    memcpy(s_last_station_mac, evt->mac, 6);
                    s_has_previous_station = true;
                }

                ESP_LOGI(TAG, "[WIFI] Client connected: MAC=" MACSTR ", AID=%d (station reconnect count: %u)",
                         MAC2STR(evt->mac), evt->aid, (unsigned int)s_station_reconnect_count);
                sm.transitionTo(BridgeState::PHONE_CONNECTED, "Client station associated with AP");
                break;
            }
            case WIFI_EVENT_AP_STADISCONNECTED: {
                wifi_event_ap_stadisconnected_t* evt = (wifi_event_ap_stadisconnected_t*)event_data;
                if (s_connected_stations > 0) {
                    s_connected_stations--;
                }
                s_client_connected = (s_connected_stations > 0);
                s_total_disconnects++;

                int64_t now_us = esp_timer_get_time();
                int64_t duration_ms = (now_us >= s_station_connected_timestamp_us)
                    ? ((now_us - s_station_connected_timestamp_us) / 1000)
                    : 0;

                ESP_LOGW(TAG, "[WIFI] Client disconnected: MAC=" MACSTR ", AID=%d, reason=%u (%s), duration=%lld ms, total disconnects=%u",
                         MAC2STR(evt->mac), evt->aid, (unsigned int)evt->reason,
                         get_wifi_disassoc_reason_name(evt->reason),
                         (long long)duration_ms, (unsigned int)s_total_disconnects);

                // SOFTAP PRESERVATION RULE: Never stop or restart the SoftAP on client disconnect!
                // Only clean client state, network session, and return state to WIFI_AP_READY.
                if (!s_client_connected) {
                    sm.cleanBuffersAndRecover("Station disconnected from AP");
                    sm.transitionTo(BridgeState::WIFI_AP_READY, "SoftAP ready for station reconnection");
                }
                break;
            }
            default:
                break;
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_AP_STAIPASSIGNED) {
            ip_event_ap_staipassigned_t* evt = (ip_event_ap_staipassigned_t*)event_data;
            ESP_LOGI(TAG, "[WIFI] DHCP lease granted: IP=" IPSTR " to MAC=" MACSTR,
                     IP2STR(&evt->ip), MAC2STR(evt->mac));
            // LOCAL_NETWORK_READY signifies only: "the phone has an active DHCP lease on Erebus"
            sm.transitionTo(BridgeState::LOCAL_NETWORK_READY, "Client DHCP lease active (192.168.4.x)");
        }
    }
}

esp_err_t wifi_transport_init(void) {
    if (s_wifi_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "[WIFI] Initializing Netif and Wi-Fi subsystem for %s", EREBUS_TARGET_NAME);

    // 1. Initialize TCP/IP stack
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to initialize netif: %s", esp_err_to_name(err));
        return err;
    }

    // 2. Create default event loop if not already present
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to create default event loop: %s", esp_err_to_name(err));
        return err;
    }

    // 3. Create default AP netif
    s_ap_netif = esp_netif_create_default_wifi_ap();
    if (!s_ap_netif) {
        ESP_LOGE(TAG, "Failed to create default Wi-Fi AP netif");
        return ESP_FAIL;
    }

    // 4. Configure Static Gateway IP and DHCP server (Local network without internet)
    esp_netif_dhcps_stop(s_ap_netif);

    esp_netif_ip_info_t ip_info;
    memset(&ip_info, 0, sizeof(ip_info));
    ip_info.ip.addr = esp_ip4addr_aton(EREBUS_DEFAULT_GATEWAY_IP);
    ip_info.gw.addr = esp_ip4addr_aton(EREBUS_DEFAULT_GATEWAY_IP);
    ip_info.netmask.addr = esp_ip4addr_aton(EREBUS_DEFAULT_NETMASK);

    err = esp_netif_set_ip_info(s_ap_netif, &ip_info);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set static IP info: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_netif_dhcps_start(s_ap_netif);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start DHCP server: %s", esp_err_to_name(err));
        return err;
    }

    // 5. Initialize Wi-Fi driver with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize Wi-Fi: %s", esp_err_to_name(err));
        return err;
    }

    // 6. Register Wi-Fi and IP event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr, &s_wifi_event_instance));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &wifi_event_handler, nullptr, &s_ip_event_instance));

    // 7. Store configuration in RAM only (prevent flash wear)
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    s_wifi_initialized = true;
    ESP_LOGI(TAG, "Wi-Fi subsystem initialized successfully");
    return ESP_OK;
}

esp_err_t wifi_transport_start_ap(void) {
    if (!s_wifi_initialized) {
        esp_err_t err = wifi_transport_init();
        if (err != ESP_OK) {
            return err;
        }
    }

    BridgeStateMachine::instance().transitionTo(BridgeState::WIFI_STARTING, "Configuring SoftAP");

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));

    // Configure SSID
    size_t ssid_len = strlen(EREBUS_WIFI_SSID);
    if (ssid_len > sizeof(wifi_config.ap.ssid)) {
        ssid_len = sizeof(wifi_config.ap.ssid);
    }
    memcpy(wifi_config.ap.ssid, EREBUS_WIFI_SSID, ssid_len);
    wifi_config.ap.ssid_len = ssid_len;

    // Strict channel, beacon and visibility settings
    wifi_config.ap.channel = EREBUS_WIFI_CHANNEL; // 6
    wifi_config.ap.ssid_hidden = EREBUS_WIFI_SSID_HIDDEN; // 0 (visible)
    wifi_config.ap.beacon_interval = EREBUS_WIFI_BEACON_INTERVAL; // 100 TU
    wifi_config.ap.max_connection = EREBUS_MAX_STA_CONN; // 1 station max during OnePlus tests

    // Configure Authentication (Password NEVER logged to UART!)
    size_t pass_len = strlen(EREBUS_WIFI_PASSWORD);
    if (pass_len >= 8) {
        size_t copy_len = (pass_len > sizeof(wifi_config.ap.password)) ? sizeof(wifi_config.ap.password) : pass_len;
        memcpy(wifi_config.ap.password, EREBUS_WIFI_PASSWORD, copy_len);
        wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    } else {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    wifi_config.ap.pmf_cfg.required = false;

    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set Wi-Fi AP mode: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to apply AP config: %s", esp_err_to_name(err));
        return err;
    }

    // Verify real applied configuration
    wifi_config_t actual_cfg;
    memset(&actual_cfg, 0, sizeof(actual_cfg));
    err = esp_wifi_get_config(WIFI_IF_AP, &actual_cfg);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "[WIFI] Config verified: SSID=\"%s\", Channel=%u, Hidden=%u, Auth=%s, BeaconInterval=%u TU, MaxConn=%u",
                 actual_cfg.ap.ssid,
                 actual_cfg.ap.channel,
                 actual_cfg.ap.ssid_hidden,
                 (actual_cfg.ap.authmode == WIFI_AUTH_OPEN) ? "OPEN" : "WPA2-PSK",
                 actual_cfg.ap.beacon_interval,
                 actual_cfg.ap.max_connection);
    } else {
        ESP_LOGW(TAG, "Could not verify AP config via esp_wifi_get_config: %s", esp_err_to_name(err));
    }

    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start Wi-Fi AP: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

bool wifi_transport_is_client_connected(void) {
    return s_client_connected;
}

uint8_t wifi_transport_get_client_count(void) {
    return s_connected_stations;
}

void wifi_transport_stop(void) {
    if (s_wifi_initialized) {
        esp_wifi_stop();
        s_client_connected = false;
        s_connected_stations = 0;
        ESP_LOGI(TAG, "Wi-Fi AP stopped");
    }
}


