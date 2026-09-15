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
| Build Android local | Reproductible | cd android-app && ./gradlew assembleDebug |
| Flash 16 Mo | Configuré, non testé matériellement | sdkconfig.defaults |
| PSRAM 8 Mo OPI | Configurée, non testée matériellement | sdkconfig.defaults |
| UART | Implémenté, non testé matériellement | diagnostics.cpp |
| Wi‑Fi Erebus | SoftAP permanent, diag déconnexion, canal 6 | commit 71d4b6b |
| Transport TCP 5288 (Android) | Socket bindé Network sans bindProcessToNetwork | MainActivity / ErebusNetworkManager |
| USB OTG | Non commencé | — |
| AOA | Non commencé | — |
| Android Auto réel | Non commencé | — |

## Derniers commits

- chore: consolidate android project under android-app
- 525baf6 build: add reproducible local android debug apk
- 07d16a0 feat: bind android local transport to erebus wifi network
- 71d4b6b fix: improve erebus softap stability and disconnect diagnostics
- 465bd24 chore: version prototype wifi credentials for local testing
- 9f9742c chore: consolidate project tracking into project status

## Configuration des secrets (Dérogation prototype)

- `firmware/main/secrets.h` est temporairement versionné avec un mot de passe de démonstration non sensible (`changeme`) ; à retirer de Git avant toute utilisation réelle ou publication.

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
