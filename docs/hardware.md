# Cible Matérielle & Configuration Mémoire — ESP32-S3 N16R8

## Spécifications Matérielles

- **SoC** : Espressif ESP32-S3 (Xtensa® Dual-core 32-bit LX7, jusqu'à 240 MHz).
- **Package** : QFN56.
- **Flash SPI** : 16 Mo (Quad SPI / OPI configuré à 80 MHz, DIO/QIO).
- **PSRAM** : 8 Mo OPI (Octal SPI / 8-line PSRAM à 80 MHz).
- **Connectivité Sans-Fil** : Wi-Fi 802.11 b/g/n (2.4 GHz) + Bluetooth 5 (LE) avec GATT sécurisé.
- **USB** : Contrôleur USB 2.0 Full-Speed natif (OTG) sur GPIO 19 (USB D-) et GPIO 20 (USB D+).
- **UART Diagnostic** : UART0 matériel (TX: GPIO 43, RX: GPIO 44 ou brochage standard devkit), 115200 8N1.

## Table de Partitions (16 Mo Flash)

Le partitionnement est optimisé pour 16 Mo (16777216 octets) avec deux slots OTA larges :

| Name | Type | SubType | Offset | Size | Flags |
|---|---|---|---|---|---|
| nvs | data | nvs | 0x9000 | 0x6000 (24 KB) | |
| otadata | data | ota | 0xf000 | 0x2000 (8 KB) | |
| phy_init | data | phy | 0x11000 | 0x1000 (4 KB) | |
| ota_0 | app | ota_0 | 0x20000 | 0x780000 (7.5 MB) | |
| ota_1 | app | ota_1 | 0x7a0000 | 0x780000 (7.5 MB) | |
| storage | data | spiffs | 0xf20000 | 0xd0000 (832 KB) | |

## Initialisation et Logs de Démarrage

Au démarrage, le firmware initialise impérativement la PSRAM et vérifie la taille de la Flash. Les logs affichent :

```
[BOOT] Erebus bridge starting
[HW] Target: ESP32-S3 N16R8
[HW] Flash detected: 16 MB
[HW] PSRAM detected: 8 MB OPI
[HW] PSRAM initialized: yes
[HW] Internal heap free: <bytes>
[HW] PSRAM heap free: <bytes>
[HW] Running partition: <partition_name>
```

Si la PSRAM ne s'initialise pas ou fait défaut, une alerte critique est levée et le système bascule dans l'état `ERROR` sans tenter de continuer silencieusement.
