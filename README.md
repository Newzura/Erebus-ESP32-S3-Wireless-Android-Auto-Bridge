# Erebus — ESP32-S3 Wireless Android Auto Bridge

> **Projet Expérimental & Embarqué Strict** : Pont sans fil entre un smartphone Android et une unité de bord Android Auto (DHU / Autoradio véhicule) propulsé par un microcontrôleur **ESP32-S3 N16R8** (16 Mo Flash, 8 Mo PSRAM OPI).

```
Téléphone Android
       ↕ Wi‑Fi local (192.168.4.1:5288) & BLE GATT sécurisé
ESP32-S3 N16R8
       ↕ USB OTG (GPIO 19/20 natif)
Android Auto Desktop Head Unit (DHU) / Autoradio
```

---

## ⚠️ Engagement d'Honnêteté Technique

- Un point d'accès Wi-Fi actif n'est pas une session Android Auto.
- Une connexion TCP active n'est pas une session Android Auto.
- Quelques octets échangés ne sont pas une session Android Auto.
- Un périphérique USB série détecté n'est pas une interface Android Auto.
- Le statut `ANDROID_AUTO_ACTIVE` n'est proclamé que sous preuve irréfutable (flux de projection valide).

---

## Structure du Monorepo

```
/
├── android-app/      # Application Android native Kotlin (compagnon local)
├── firmware/         # Firmware embarqué C/C++ ESP-IDF pour ESP32-S3 N16R8
├── docs/             # Spécifications, rôles USB, protocoles, tests
├── tools/            # Scripts d'automatisation (build, flash, monitor, DHU)
├── README.md
├── LICENSE
├── .gitignore
└── CONTRIBUTING.md
```

---

## Cible Matérielle

- **SoC** : ESP32-S3 QFN56 (N16R8)
- **Flash** : 16 Mo Quad/Octal SPI
- **PSRAM** : 8 Mo Octal SPI (OPI)
- **USB** : USB OTG natif (GPIO 19 USB_D-, GPIO 20 USB_D+)
- **UART** : Logs de diagnostic temps réel (115200 bauds)

---

## Démarrage Rapide

### 1. Cloner et configurer les secrets

```bash
# Copier les gabarits de secrets
cp firmware/main/secrets.example.h firmware/main/secrets.h
cp android-app/local.properties.example android-app/local.properties
cp android-app/secrets.properties.example android-app/secrets.properties
```

### 2. Compiler le Firmware ESP32-S3

```bash
./tools/build-firmware.sh
```

### 3. Flasher le Firmware

```bash
./tools/flash.sh /dev/ttyUSB0
```

### 4. Compiler l'application Compagnon Android

```bash
./tools/build-android.sh
```

### 5. Tester avec le DHU

```bash
./tools/test-dhu.sh
```
