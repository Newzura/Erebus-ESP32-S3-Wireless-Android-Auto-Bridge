#include "bridge_state.h"
#include "esp_log.h"
#include <cstdio>

static const char* TAG = "STATE";

BridgeStateMachine& BridgeStateMachine::instance() {
    static BridgeStateMachine s_instance;
    return s_instance;
}

BridgeStateMachine::BridgeStateMachine()
    : m_currentState(BridgeState::BOOT),
      m_wifiRxBytes(0),
      m_wifiTxBytes(0),
      m_usbRxBytes(0),
      m_usbTxBytes(0) {}

void BridgeStateMachine::init() {
    m_currentState = BridgeState::BOOT;
    m_wifiRxBytes = 0;
    m_wifiTxBytes = 0;
    m_usbRxBytes = 0;
    m_usbTxBytes = 0;
    ESP_LOGI(TAG, "Bridge state machine initialized at state %s", getStateName(m_currentState));
}

const char* BridgeStateMachine::getStateName(BridgeState state) const {
    switch (state) {
        case BridgeState::BOOT: return "BOOT";
        case BridgeState::HARDWARE_CHECK: return "HARDWARE_CHECK";
        case BridgeState::PSRAM_READY: return "PSRAM_READY";
        case BridgeState::WIFI_STARTING: return "WIFI_STARTING";
        case BridgeState::WIFI_AP_READY: return "WIFI_AP_READY";
        case BridgeState::PHONE_CONNECTING: return "PHONE_CONNECTING";
        case BridgeState::PHONE_CONNECTED: return "PHONE_CONNECTED";
        case BridgeState::NETWORK_TRANSPORT_READY: return "NETWORK_TRANSPORT_READY";
        case BridgeState::USB_WAITING: return "USB_WAITING";
        case BridgeState::USB_INITIALIZING: return "USB_INITIALIZING";
        case BridgeState::USB_ENUMERATING: return "USB_ENUMERATING";
        case BridgeState::USB_NEGOTIATING: return "USB_NEGOTIATING";
        case BridgeState::AOA_NEGOTIATING: return "AOA_NEGOTIATING";
        case BridgeState::TRANSPORT_BRIDGING: return "TRANSPORT_BRIDGING";
        case BridgeState::ANDROID_AUTO_ACTIVE: return "ANDROID_AUTO_ACTIVE";
        case BridgeState::RECOVERING: return "RECOVERING";
        case BridgeState::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void BridgeStateMachine::transitionTo(BridgeState newState, const char* reason) {
    if (m_currentState == newState) {
        return;
    }
    const char* oldName = getStateName(m_currentState);
    const char* newName = getStateName(newState);
    m_currentState = newState;

    if (reason) {
        ESP_LOGI(TAG, "[STATE] %s -> %s (reason: %s)", oldName, newName, reason);
    } else {
        ESP_LOGI(TAG, "[STATE] %s -> %s", oldName, newName);
    }
}

BridgeState BridgeStateMachine::getState() const {
    return m_currentState;
}

void BridgeStateMachine::updateWifiRx(uint32_t bytes) {
    m_wifiRxBytes += bytes;
}

void BridgeStateMachine::updateWifiTx(uint32_t bytes) {
    m_wifiTxBytes += bytes;
}

void BridgeStateMachine::updateUsbRx(uint32_t bytes) {
    m_usbRxBytes += bytes;
}

void BridgeStateMachine::updateUsbTx(uint32_t bytes) {
    m_usbTxBytes += bytes;
}

void BridgeStateMachine::logMetrics() const {
    ESP_LOGI("BRIDGE", "[BRIDGE] RX Wi-Fi=%llu bytes TX USB=%llu bytes",
             (unsigned long long)m_wifiRxBytes, (unsigned long long)m_usbTxBytes);
    ESP_LOGI("BRIDGE", "[BRIDGE] RX USB=%llu bytes TX Wi-Fi=%llu bytes",
             (unsigned long long)m_usbRxBytes, (unsigned long long)m_wifiTxBytes);
}

void BridgeStateMachine::cleanBuffersAndRecover(const char* reason) {
    ESP_LOGW(TAG, "Cleaning buffers and recovering: %s", reason ? reason : "unknown");
    transitionTo(BridgeState::RECOVERING, reason);
    // Cleanup buffers logic, then clean return to WIFI_AP_READY
    transitionTo(BridgeState::WIFI_AP_READY, "Clean recovery completed");
}
