# Plan de Test Expérimental — Erebus

## Procédures de Test pas-à-pas

### Test 1 : Compilation et Détection Mémoire ESP32-S3 N16R8
- **Commande** :
  ```bash
  cd firmware
  idf.py set-target esp32s3
  idf.py build
  ```
- **Validation** :
  - Compilation réussie sans warning fatal.
  - Image binaire générée dans `firmware/build/erebus-firmware.bin`.
  - Flash configurée à 16 Mo, PSRAM OPI configurée à 8 Mo.

### Test 2 : Diagnostic Flash et PSRAM au Boot
- **Commande** :
  ```bash
  idf.py -p /dev/ttyUSB0 monitor
  ```
- **Validation** :
  - Présence des logs `[HW] Target: ESP32-S3 N16R8`, `Flash detected: 16 MB`, `PSRAM detected: 8 MB OPI`, `PSRAM initialized: yes`.
  - Pas de kernel panic ou assertion d'initialisation mémoire.

### Test 3 : Point d'accès Wi-Fi Erebus
- **Validation** :
  - SSID `Erebus` visible sur le téléphone.
  - Connexion avec mot de passe local défini dans `secrets.h`.
  - DHCP attribue une adresse IP dans la plage `192.168.4.x`.
  - Logs UART affichent `[WIFI] Client connected: 192.168.4.x`.

### Test 4 : Transport Local Socket TCP 5288
- **Validation** :
  - Client Android ou script netcat se connecte sur `192.168.4.1:5288`.
  - Échange bidirectionnel de données de test.
  - Compteurs RX/TX incrémentés de façon cohérente.
  - Pas d'annonce `ANDROID_AUTO_ACTIVE` prématurée.

### Test 5 : USB OTG et DHU
- **Validation** :
  - Connexion USB OTG S3 vers hôte DHU.
  - Énumération capturée dans les logs d'hôte et logs S3.
  - Analyse des endpoints Bulk IN/OUT.
