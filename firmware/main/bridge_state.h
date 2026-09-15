#pragma once

#include <cstdint>

enum class BridgeState {
    BOOT,
    HARDWARE_CHECK,
    PSRAM_READY,
    WIFI_STARTING,
    WIFI_AP_READY,
    PHONE_CONNECTING,
    PHONE_CONNECTED,
    LOCAL_NETWORK_READY,
    LOCAL_TRANSPORT_CONNECTED,
    NETWORK_TRANSPORT_READY, // alias/compatibility
    USB_WAITING,
    USB_INITIALIZING,
    USB_ENUMERATING,
    USB_NEGOTIATING,
    AOA_NEGOTIATING,
    TRANSPORT_BRIDGING,
    ANDROID_AUTO_ACTIVE,
    RECOVERING,
    ERROR
};

class BridgeStateMachine {
public:
    static BridgeStateMachine& instance();

    void init();
    void transitionTo(BridgeState newState, const char* reason = nullptr);
    BridgeState getState() const;
    const char* getStateName(BridgeState state) const;

    void updateWifiRx(uint32_t bytes);
    void updateWifiTx(uint32_t bytes);
    void updateUsbRx(uint32_t bytes);
    void updateUsbTx(uint32_t bytes);

    uint64_t getWifiRxTotal() const { return m_wifiRxBytes; }
    uint64_t getWifiTxTotal() const { return m_wifiTxBytes; }
    uint64_t getUsbRxTotal() const { return m_usbRxBytes; }
    uint64_t getUsbTxTotal() const { return m_usbTxBytes; }

    void logMetrics() const;
    void cleanBuffersAndRecover(const char* reason);

private:
    BridgeStateMachine();
    BridgeState m_currentState;
    uint64_t m_wifiRxBytes;
    uint64_t m_wifiTxBytes;
    uint64_t m_usbRxBytes;
    uint64_t m_usbTxBytes;
};
