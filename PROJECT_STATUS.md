# Erebus — État du projet

## Objectif

Téléphone Android
↕ Wi‑Fi local Erebus
ESP32-S3 N16R8
↕ USB OTG
DHU / autoradio Android Auto

## Matériel

- ESP32-S3 N16R8
- Flash : 16 Mo
- PSRAM : 8 Mo OPI
- Port UART/COM : flash et logs
- Port USB natif : GPIO20 D+, GPIO19 D−
- eFuse USB_PHY_SEL : interdite

## État actuel

| Élément | État | Preuve |
|---|---|---|
| Build ESP32-S3 | Compilé | idf.py build |
| Flash 16 Mo | Configuré, non testé matériellement | sdkconfig.defaults |
| PSRAM 8 Mo OPI | Configurée, non testée matériellement | sdkconfig.defaults |
| UART | Implémenté, non testé matériellement | diagnostics.cpp |
| Wi‑Fi Erebus | Implémenté, non testé matériellement | commit e564500 |
| Transport TCP 5288 | Non commencé | — |
| USB OTG | Non commencé | — |
| AOA | Non commencé | — |
| Android Auto réel | Non commencé | — |

## Dernier commit

- 7521376 docs: add official erebus technical tracking journal

## Prochaine action unique

Flasher l’ESP32-S3 N16R8 par le port UART/COM, puis capturer les logs de boot
validant Flash 16 Mo, PSRAM OPI 8 Mo et le démarrage de l’AP Wi‑Fi Erebus.

## Résultat attendu du prochain test

```text
[BOOT] Erebus bridge starting
[HW] Flash detected: 16 MB
[HW] PSRAM detected: 8 MB OPI
[HW] PSRAM initialized: yes
[WIFI] AP started: SSID=Erebus, gateway=192.168.4.1
```

## Règles permanentes

- Aucun cloud.
- Aucune API externe.
- Aucun mot de passe dans Git.
- Aucun mot de passe dans les logs.
- UART pour flash et diagnostics.
- USB OTG réservé au DHU.
- Ne jamais graver USB_PHY_SEL.
- Ne jamais annoncer Android Auto sans test DHU réel.
- Une compilation ne vaut pas un test matériel.
- Un test Wi‑Fi ne vaut pas un test Android Auto.
