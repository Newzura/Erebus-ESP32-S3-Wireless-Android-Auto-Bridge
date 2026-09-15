#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "esp_netif.h"

// --- Configuration ---
#define WIFI_SSID "Erebus-Bridge"
#define WIFI_PASS "erebus123"
#define TCP_PORT 5288
#define BUFFER_SIZE 1024

static const char *TAG = "ErebusBridge";

int server_sockfd = -1;
int client_sockfd = -1;

// Initialisation Wi-Fi en mode Point d'Accès (SoftAP)
void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    assert(ap_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .channel = 6,
            .password = WIFI_PASS,
            .max_connection = 2,
            .authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    
    ESP_LOGI(TAG, "Configuration SoftAP: SSID=%s, PASS=%s", WIFI_SSID, WIFI_PASS);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "SoftAP démarré. IP: 192.168.4.1");
}

// Tâche Serveur TCP
void tcp_server_task(void *pvParameters)
{
    struct sockaddr_in server_addr, client_addr;
    int sock_opt = 1;
    char rx_buffer[BUFFER_SIZE];

    // Création socket
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd < 0) {
        ESP_LOGE(TAG, "Erreur création socket: %d", errno);
        vTaskDelete(NULL);
    }

    // Réutilisation adresse
    setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof(sock_opt));

    // Bind
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(TCP_PORT);

    if (bind(server_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Erreur bind: %d", errno);
        vTaskDelete(NULL);
    }

    // Listen
    if (listen(server_sockfd, 1) != 0) {
        ESP_LOGE(TAG, "Erreur listen: %d", errno);
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "Serveur TCP en écoute sur le port %d", TCP_PORT);

    while (1) {
        // Accepter connexion
        if (client_sockfd < 0) {
            socklen_t addr_len = sizeof(client_addr);
            client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, &addr_len);
            if (client_sockfd >= 0) {
                ESP_LOGI(TAG, "Client connecté !");
            }
        } else {
            // Lire données si client connecté
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(client_sockfd, &readfds);
            
            struct timeval timeout = { .tv_sec = 0, .tv_usec = 100000 }; // 100ms
            int s = select(client_sockfd + 1, &readfds, NULL, NULL, &timeout);

            if (s > 0 && FD_ISSET(client_sockfd, &readfds)) {
                int len = recv(client_sockfd, rx_buffer, sizeof(rx_buffer) - 1, 0);
                if (len > 0) {
                    rx_buffer[len] = 0;
                    ESP_LOGI(TAG, "Reçu (%d octets): %s", len, rx_buffer);
                    
                    // TODO: Ici, injecter les données dans le stack USB Host (AOA)
                    // Pour le test ce soir, on fait juste un echo/log
                } else if (len == 0) {
                    ESP_LOGW(TAG, "Client déconnecté");
                    close(client_sockfd);
                    client_sockfd = -1;
                }
            }
        }
        
        // Petit délai pour ne pas saturer
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    if (client_sockfd >= 0) close(client_sockfd);
    close(server_sockfd);
    vTaskDelete(NULL);
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "=== Démarrage Erebus Bridge ===");
    
    // Init NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Init Wi-Fi
    wifi_init_softap();

    // Lancer serveur TCP
    xTaskCreate(tcp_server_task, "tcp_server", 4096 * 2, NULL, 5, NULL);
}
