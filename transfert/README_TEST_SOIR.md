# 🚀 Erebus Bridge : Guide de Test "Ce Soir" (macOS)

Ce guide permet de transformer ton ESP32-S3 en pont Wi-Fi/USB pour connecter ton application **Erebus** à ta Hyundai filaire, sans modifier l'architecture globale.

## 📋 Prérequis

*   **Matériel** : ESP32-S3 (N16R8 recommandé), Mac, Câble USB-C vers USB-A (pour la voiture) + Câble USB-C vers USB-C (pour le flash).
*   **Logiciel** :
    *   ESP-IDF v5.x installé (`~/esp/esp-idf`).
    *   Android Studio installé.
    *   Accès au dépôt `Newzura/Erebus` (Android) et `Newzura/Erebus-ESP32-S3-Wireless-Android-Auto-Bridge` (Firmware).

---

## 1️⃣ Partie Firmware (ESP32-S3)

Nous allons créer un point d'accès Wi-Fi (`Erebus-Bridge`) et un serveur TCP sur le port `5288` qui fera le relais avec l'USB de la voiture.

### A. Structure du projet

Dans ton terminal macOS :

```bash
cd ~/Projects # Ou ton dossier de travail
git clone https://github.com/Newzura/Erebus-ESP32-S3-Wireless-Android-Auto-Bridge.git
cd Erebus-ESP32-S3-Wireless-Android-Auto-Bridge
```

### B. Fichier Principal (`main/bridge_main.cpp`)

Crée ou remplace le fichier `main/bridge_main.cpp` par ce code complet. Il gère le Wi-Fi, le serveur TCP et la boucle de relais.

```cpp
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
```

### C. Compilation et Flash (macOS)

```bash
# 1. Activer ESP-IDF
. $HOME/esp/esp-idf/export.sh

# 2. Configurer pour ESP32-S3
idf.py set-target esp32s3

# 3. Compiler
idf.py build

# 4. Trouver le port (ex: /dev/cu.usbserial-0001)
ls /dev/cu.*

# 5. Flasher (REMPLACE LE PORT CI-DESSOUS)
idf.py -p /dev/cu.usbserial-0001 flash monitor
```

*Laisse le monitor ouvert pour voir les logs de connexion.*

---

## 2️⃣ Partie Application Android (Erebus)

Nous allons ajouter un client TCP simple qui se connecte à l'ESP32 dès que le Wi-Fi est actif.

### A. Permissions (`AndroidManifest.xml`)

Ajoute ces lignes dans `app/src/main/AndroidManifest.xml` avant la balise `<application>` :

```xml
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
<uses-permission android:name="android.permission.CHANGE_NETWORK_STATE" />
<!-- Nécessaire pour Android 13+ si on scanne le Wi-Fi, mais ici on se connecte juste -->
<uses-permission android:name="android.permission.NEARBY_WIFI_DEVICES" android:usesPermissionFlags="neverForLocation"/>
```

### B. Classe Client (`ErebusBridgeClient.kt`)

Crée le fichier `app/src/main/java/com/newzura/erebus/ErebusBridgeClient.kt` :

```kotlin
package com.newzura.erebus

import android.util.Log
import java.io.InputStream
import java.io.OutputStream
import java.net.Socket
import kotlin.concurrent.thread

class ErebusBridgeClient {
    private var socket: Socket? = null
    private var outputStream: OutputStream? = null
    private var inputStream: InputStream? = null
    private var isConnected = false
    private var readThread: Thread? = null

    companion object {
        private const val TAG = "ErebusBridge"
        const val DEFAULT_IP = "192.168.4.1" // IP par défaut de l'ESP32 en AP
        const val DEFAULT_PORT = 5288
    }

    interface MessageListener {
        fun onMessageReceived(message: String)
        fun onConnectionLost()
    }

    private var listener: MessageListener? = null

    fun setListener(listener: MessageListener?) {
        this.listener = listener
    }

    fun connect(ip: String = DEFAULT_IP, port: Int = DEFAULT_PORT) {
        if (isConnected) return

        thread {
            try {
                Log.i(TAG, "Tentative de connexion à $ip:$port...")
                socket = Socket(ip, port)
                socket?.soTimeout = 0 // Bloquant pour la lecture
                
                outputStream = socket!!.getOutputStream()
                inputStream = socket!!.getInputStream()
                isConnected = true
                
                Log.i(TAG, "✅ Connecté au Bridge Erebus !")
                
                // Lancer thread de lecture
                readThread = thread {
                    try {
                        val buffer = ByteArray(1024)
                        while (isConnected && inputStream != null) {
                            val bytesRead = inputStream!!.read(buffer)
                            if (bytesRead > 0) {
                                val msg = String(buffer, 0, bytesRead, Charsets.UTF_8).trim()
                                Log.d(TAG, "Reçu du Bridge: $msg")
                                listener?.onMessageReceived(msg)
                            } else if (bytesRead == -1) {
                                Log.w(TAG, "Fin de flux (déconnexion)")
                                break
                            }
                        }
                    } catch (e: Exception) {
                        if (isConnected) Log.e(TAG, "Erreur lecture: ${e.message}")
                    } finally {
                        if (isConnected) {
                            isConnected = false
                            listener?.onConnectionLost()
                        }
                    }
                }

            } catch (e: Exception) {
                Log.e(TAG, "❌ Échec connexion: ${e.message}")
                isConnected = false
                listener?.onConnectionLost()
            }
        }
    }

    fun send(command: String) {
        if (!isConnected || outputStream == null) {
            Log.w(TAG, "Impossible d'envoyer: non connecté. Cmd: $command")
            return
        }
        try {
            val data = (command + "\n").toByteArray(Charsets.UTF_8)
            outputStream!!.write(data)
            outputStream!!.flush()
            Log.d(TAG, "Envoyé au Bridge: $command")
        } catch (e: Exception) {
            Log.e(TAG, "Erreur envoi: ${e.message}")
            isConnected = false
            listener?.onConnectionLost()
        }
    }

    fun disconnect() {
        isConnected = false
        try {
            readThread?.interrupt()
            inputStream?.close()
            outputStream?.close()
            socket?.close()
        } catch (e: Exception) { }
        socket = null
        Log.i(TAG, "Déconnecté du Bridge")
    }
    
    fun isConnecting(): Boolean = isConnected
}
```

### C. Intégration Rapide (`MainActivity.kt`)

Dans ton `MainActivity.kt` (ou celle qui gère l'UI principale), ajoute ceci pour tester la connexion :

```kotlin
// Déclare le client en variable membre
private val bridgeClient = ErebusBridgeClient()

override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    // ... ton code existant ...

    // Configurer l'écouteur
    bridgeClient.setListener(object : ErebusBridgeClient.MessageListener {
        override fun onMessageReceived(message: String) {
            runOnUiThread {
                // Mettre à jour l'UI avec le message reçu
                Log.i("UI", "Message Bridge: $message")
            }
        }
        override fun onConnectionLost() {
            runOnUiThread {
                Log.w("UI", "Connexion Bridge perdue")
                // Mettre à jour l'UI (bouton rouge par exemple)
            }
        }
    })
}

// Appelable depuis un bouton "Tester Connexion"
fun testBridgeConnection() {
    if (!bridgeClient.isConnecting()) {
        bridgeClient.connect()
    } else {
        bridgeClient.send("PING_FROM_ANDROID")
    }
}
```

### D. Build et Install (macOS)

```bash
cd ~/Projects/Erebus # Chemin vers ton repo Android

# 1. Nettoyer (optionnel mais recommandé)
./gradlew clean

# 2. Compiler le APK Debug
./gradlew assembleDebug

# 3. Installer sur le téléphone (via ADB)
# Assure-toi que le téléphone est branché et en mode débogage
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

---

## 3️⃣ Procédure de Test (Ce Soir)

1.  **Flash ESP32** : Lance la commande `idf.py flash monitor`. Tu devrais voir :
    ```text
    I ErebusBridge: SoftAP démarré. IP: 192.168.4.1
    I ErebusBridge: Serveur TCP en écoute sur le port 5288
    ```
2.  **Prépare le Téléphone** :
    *   Va dans les paramètres Wi-Fi.
    *   Connecte-toi au réseau **`Erebus-Bridge`** (Mot de passe : `erebus123`).
    *   *Note : Ton téléphone peut dire "Pas d'accès Internet", c'est normal, reste connecté.*
3.  **Lance l'App Erebus** :
    *   Ouvre l'application.
    *   Appuie sur ton bouton de test (ou lance l'action qui appelle `bridgeClient.connect()`).
4.  **Vérifie les Logs** :
    *   **Côté ESP32 (Terminal)** : Tu dois voir `I ErebusBridge: Client connecté !`.
    *   **Côté Android (Logcat)** : Tu dois voir `✅ Connecté au Bridge Erebus !` et `Envoyé au Bridge: PING_FROM_ANDROID`.

## ✅ Prochaines étapes (après ce test)

Une fois cette connexion TCP validée :
1.  Implémenter la logique de **relais USB Host** dans le firmware (envoi des paquets reçus via TCP vers le port USB de la voiture).
2.  Gérer la négociation **AOA 2.0** complète.
3.  Optimiser la latence pour la vidéo.

Mais pour ce soir, si tu vois "Client connecté" dans les logs ESP32, **c'est gagné**. 🎉
