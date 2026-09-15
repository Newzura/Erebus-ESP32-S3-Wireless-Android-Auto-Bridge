# Firmware Erebus — ESP32-S3 N16R8

Firmware embarqué natif C/C++ pour microcontrôleur Espressif **ESP32-S3 N16R8** (16 Mo Flash, 8 Mo PSRAM OPI).

## Architecture du Code

- `main/app_main.cpp` : Point d'entrée, initialisation matérielle et boucle système.
- `main/app_config.h` : Constantes matérielles, mémoire, buffers et timeouts.
- `main/bridge_state.h/cpp` : Machine d'état formelle du pont Erebus et transitions.
- `main/diagnostics.h/cpp` : Détection matérielle (Flash 16MB, PSRAM 8MB OPI, heaps, partitions).
- `main/wifi_transport.h/cpp` : Gestionnaire AP Wi-Fi local et sockets TCP.
- `main/usb_otg_transport.h/cpp` : Driver USB OTG natif pour Desktop Head Unit (DHU).
- `main/protocol_router.h/cpp` : Routage des flux de transport sans simulation.
- `main/secrets.example.h` : Modèle de configuration des identifiants locaux.
- `main/secrets.h` : Secrets locaux (ignoré par Git).

## Compilation et Flash

```bash
# Activer l'environnement ESP-IDF
. /opt/esp-idf/export.sh

# Définir la cible
idf.py set-target esp32s3

# Compiler
idf.py build

# Flasher
idf.py -p /dev/ttyUSB0 flash monitor
```
