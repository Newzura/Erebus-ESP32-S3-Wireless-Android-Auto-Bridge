# 🚀 Guide de Test Rapide - Erebus Bridge (macOS)

## Objectif
Tester la connexion TCP entre ton téléphone Android et l'ESP32-S3 via le Wi-Fi `Erebus-Bridge`.

---

## 1. Firmware ESP32-S3

Le code TCP est **déjà intégré** dans le projet :
- `firmware/main/protocol_router.cpp` : Serveur TCP sur le port 5288
- `firmware/main/app_main.cpp` : Tâche de routage active

### Compilation & Flash

```bash
cd /Users/thomas/Documents/VSCODE/Projets/Erebus-ESP32-S3-Wireless-Android-Auto-Bridge

# Charger ESP-IDF
. $HOME/esp/esp-idf/export.sh

# Compiler
cd firmware
idf.py build

# Trouver le port
ls /dev/cu.*

# Flasher (remplace le port)
idf.py -p /dev/cu.usbserial-0001 flash monitor
```

### Logs attendus
```
I ROUTER: Initializing TCP server on port 5288
I ROUTER: TCP server listening on port 5288
I MAIN: Erebus ESP32-S3 N16R8 firmware started. TCP server on port 5288.
I WIFI: SoftAP permanently active. Gateway IP: 192.168.4.1, SSID: "Erebus"
```

---

## 2. Application Android

Le client TCP est **déjà intégré** :
- `android-app/app/src/main/java/com/example/ErebusBridgeClient.kt`
- Permissions réseau ajoutées dans `AndroidManifest.xml`
- Méthode `testBridgeConnection()` dans `MainActivity.kt`

### Compilation & Installation

```bash
cd /Users/thomas/Documents/VSCODE/Projets/Erebus-ESP32-S3-Wireless-Android-Auto-Bridge/android-app

# Compiler
./gradlew assembleDebug

# Installer (téléphone branché en USB + débogage activé)
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

---

## 3. Procédure de Test

1. **Flash l'ESP32** avec la commande ci-dessus
2. **Laisse le monitor ouvert** pour voir les logs
3. **Sur ton téléphone** :
   - Va dans Paramètres → Wi-Fi
   - Connecte-toi à `Erebus` (mot de passe : `changeme`)
   - *Note : "Pas d'internet" est normal*
4. **Ouvre l'appli Erebus**
5. **Appuie sur le bouton de test** (ou appelle `testBridgeConnection()`)

### Vérification

**Côté ESP32 (logs UART)** :
```
I ROUTER: Client connected from 192.168.4.x:y
I ROUTER: Received X bytes from Android: PING_FROM_ANDROID
```

**Côté Android (Logcat)** :
```
I ErebusBridge: Tentative de connexion à 192.168.4.1:5288...
I ErebusBridge: ✅ Connecté au Bridge Erebus !
D ErebusBridge: Envoyé au Bridge: PING_FROM_ANDROID
```

---

## 4. Dépannage

| Problème | Solution |
|----------|----------|
| "Connection refused" | Vérifie que l'ESP32 a bien démarré (logs UART) |
| "Timeout" | Téléphone mal connecté au Wi-Fi Erebus |
| Pas de logs ESP32 | Mauvais port USB dans la commande `idf.py -p` |
| APK ne s'installe pas | `adb devices` pour vérifier la connexion |

---

## Prochaines Étapes

Une fois cette connexion validée :
1. Implémenter le relais USB Host (AOA 2.0)
2. Gérer la négociation avec le head-unit Hyundai
3. Optimiser la latence pour la vidéo

**Si tu vois "Client connected" dans les logs ESP32, c'est gagné !** 🎉
