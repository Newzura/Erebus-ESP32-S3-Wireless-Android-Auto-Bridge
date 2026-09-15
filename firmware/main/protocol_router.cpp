#include "protocol_router.h"
#include "app_config.h"
#include "wifi_transport.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include <cstring>

static const char* TAG = "ROUTER";

static int s_server_sockfd = -1;
static int s_client_sockfd = -1;
static bool s_tcp_initialized = false;

static const int TCP_BACKLOG = 1;
static const int RX_BUFFER_SIZE = 2048;

esp_err_t protocol_router_init(void) {
    if (s_tcp_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing TCP server on port %d", EREBUS_TRANSPORT_PORT);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    // Create socket
    s_server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (s_server_sockfd < 0) {
        ESP_LOGE(TAG, "Failed to create socket: errno %d", errno);
        return ESP_FAIL;
    }

    // Allow address reuse
    int optval = 1;
    setsockopt(s_server_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // Bind
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(EREBUS_TRANSPORT_PORT);

    if (bind(s_server_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Failed to bind socket: errno %d", errno);
        close(s_server_sockfd);
        s_server_sockfd = -1;
        return ESP_FAIL;
    }

    // Listen
    if (listen(s_server_sockfd, TCP_BACKLOG) != 0) {
        ESP_LOGE(TAG, "Failed to listen: errno %d", errno);
        close(s_server_sockfd);
        s_server_sockfd = -1;
        return ESP_FAIL;
    }

    s_tcp_initialized = true;
    ESP_LOGI(TAG, "TCP server listening on port %d", EREBUS_TRANSPORT_PORT);
    return ESP_OK;
}

esp_err_t protocol_router_process(void) {
    if (!s_tcp_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    fd_set readfds;
    FD_ZERO(&readfds);

    // Accept new connection if no client connected
    if (s_client_sockfd < 0) {
        FD_SET(s_server_sockfd, &readfds);
    }

    // Read from client if connected
    if (s_client_sockfd >= 0) {
        FD_SET(s_client_sockfd, &readfds);
    }

    struct timeval timeout = { .tv_sec = 0, .tv_usec = 100000 }; // 100ms
    int s = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);

    if (s < 0) {
        ESP_LOGE(TAG, "Select failed: errno %d", errno);
        return ESP_FAIL;
    }

    // Accept new client
    if (s_client_sockfd < 0 && FD_ISSET(s_server_sockfd, &readfds)) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        s_client_sockfd = accept(s_server_sockfd, (struct sockaddr*)&client_addr, &addr_len);
        
        if (s_client_sockfd >= 0) {
            ESP_LOGI(TAG, "Client connected from %s:%d", 
                     inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        } else {
            ESP_LOGE(TAG, "Accept failed: errno %d", errno);
        }
    }

    // Receive data from client
    if (s_client_sockfd >= 0 && FD_ISSET(s_client_sockfd, &readfds)) {
        char rx_buffer[RX_BUFFER_SIZE];
        int len = recv(s_client_sockfd, rx_buffer, sizeof(rx_buffer) - 1, 0);
        
        if (len > 0) {
            rx_buffer[len] = '\0';
            ESP_LOGI(TAG, "Received %d bytes from Android: %s", len, rx_buffer);
            
            // TODO Phase 6: Forward data to USB Host (AOA)
            // usb_otg_send(rx_buffer, len);
            
        } else if (len == 0) {
            ESP_LOGW(TAG, "Client disconnected");
            close(s_client_sockfd);
            s_client_sockfd = -1;
        } else {
            ESP_LOGE(TAG, "Receive error: errno %d", errno);
            close(s_client_sockfd);
            s_client_sockfd = -1;
        }
    }

    return ESP_OK;
}

void protocol_router_reset(void) {
    if (s_client_sockfd >= 0) {
        close(s_client_sockfd);
        s_client_sockfd = -1;
    }
    ESP_LOGI(TAG, "Protocol router buffers reset");
}
