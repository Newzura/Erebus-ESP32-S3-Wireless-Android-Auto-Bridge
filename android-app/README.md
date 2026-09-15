# Application Compagnon Android — Erebus

Application native Android (Kotlin / Jetpack Compose) servant d'interface minimale locale de diagnostic et de configuration pour le pont sans fil **ESP32-S3 N16R8**.

## Principes d'Implémentation

- **100% Local** : Zéro cloud, zéro backend distant, zéro télémétrie.
- **Réseau Local Dédié** : La permission réseau (`INTERNET`) est strictement réservée à l'ouverture d'un socket TCP local vers l'ESP32-S3 (`192.168.4.1:5288`).
- **Rôle Secondaire** : L'application est un outil de diagnostic compagnon pour le firmware (affichage des compteurs RX/TX, métriques Wi-Fi, déclenchement de reconnexion).
- **Communication BLE Sécurisée** : Échange de paramètres et diagnostics via BLE GATT chiffré.

## Configuration Locale

Copier les gabarits de configuration locale (strictement ignorés par Git) :

```bash
cp local.properties.example local.properties
cp secrets.properties.example secrets.properties
```

## Compilation

```bash
./gradlew assembleDebug
```
