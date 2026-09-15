# Décision Technique Matérielle — USB PHY ESP32-S3 N16R8

> **AVERTISSEMENT CRITIQUE DE SÉCURITÉ MATÉRIELLE** :  
> L'eFuse `USB_PHY_SEL` est une opération **irréversible** (One-Time Programmable / OTP).  
> **Aucune gravure d'eFuse ne sera effectuée ni proposée dans ce projet sans validation formelle et test préalable de récupération UART matérielle.**

---

## 1. Modèle Exact de Carte Cible

- **Module** : Espressif ESP32-S3-WROOM-1 (ou WROOM-2) N16R8.
- **Boîtier SoC** : ESP32-S3 (QFN56).
- **Mémoire Flash** : 16 Mo SPI Flash (Quad/Octal SPI).
- **Mémoire PSRAM** : 8 Mo Octal SPI (OPI) intégrée au boîtier/module.
- **Format Carte Type** : ESP32-S3-DevKitC-1-N16R8 (ou clone compatible avec double port USB Type-C).

---

## 2. Description des Ports USB Physiques Disponibles

La carte de développement ESP32-S3-DevKitC-1 dispose typiquement de **deux connecteurs USB Type-C distincts** :

```
                +-----------------------------------------+
                |         ESP32-S3-DevKitC-1              |
                |                                         |
[USB Type-C 1]  |  Bridge CP2102N / CH343  -> UART0       |  Label: "UART" ou "COM"
(Flash & Logs)  |  (TX: GPIO43, RX: GPIO44)               |  (ou USB-Serial-JTAG natif)
                |                                         |
[USB Type-C 2]  |  Direct SoC USB Pins     -> OTG FS      |  Label: "USB"
(Liaison DHU)   |  (D-: GPIO19, D+: GPIO20)               |  (USB OTG natif Full-Speed)
                +-----------------------------------------+
```

1. **Port USB 1 ("UART" / "COM")** :
   - Connecté soit à une puce passerelle USB-UART dédiée (ex: CP2102N ou CH343P) reliée à l'UART0 (GPIO43/GPIO44), soit directement à l'interface interne USB Serial/JTAG selon la révision matérielle.
   - Rôle : Flash initial du firmware, téléchargement ROM bootloader, console de logs `ESP_LOGx` temps réel à 115200 bauds.
2. **Port USB 2 ("USB")** :
   - Connecté directement aux broches de signaux différentiels du SoC ESP32-S3 : **GPIO19 (USB D-)** et **GPIO20 (USB D+)**.
   - Rôle visé : Interface USB OTG native (Full-Speed 12 Mbps) vers l'hôte Android Auto (DHU / Autoradio).

---

## 3. Câblage des Broches GPIO19 et GPIO20

- **GPIO19** : Signal différentiel USB D- (Data Moins).
- **GPIO20** : Signal différentiel USB D+ (Data Plus).
- Ces broches sont reliées aux plots du connecteur Type-C n°2 ("USB").
- **Précision fondamentale** : La présence physique de ces broches sur le port Type-C **ne garantit pas** à elle seule l'activation d'USB OTG. Le contrôleur interne USB OTG FS doit être configuré via le stack logiciel ESP-IDF (TinyUSB ou driver bas niveau OTG), sans conflit avec le contrôleur USB Serial/JTAG qui partage le même PHY interne par défaut.

---

## 4. Port Réservé pour Flash et Debug

- **Le port physique "UART" (UART0 matériel)** est exclusivement dédié au flashage, aux diagnostics et à la console de logs système.
- **Règle absolue** : La console de debug ne doit **jamais** utiliser le contrôleur USB OTG du port "USB". Les logs doivent rester isolés sur UART0 matériel (`CONFIG_ESP_CONSOLE_UART_NUM=0`) pour ne pas polluer l'interface USB OTG avec des trames CDC série lors de la connexion au DHU.

---

## 5. Partage du PHY Interne : USB Serial/JTAG vs USB OTG

L'ESP32-S3 intègre :
- **1 PHY USB Full-Speed interne** unique.
- **2 contrôleurs USB internes** :
  1. Le contrôleur **USB Serial/JTAG** (utilisé par le bootloader ROM pour le debug/flash CDC/JTAG).
  2. Le contrôleur **USB OTG Full-Speed** (supportant les rôles Device et Host pour classes personnalisées, AOA, etc.).

Par défaut au reset :
- Le contrôleur USB Serial/JTAG et le contrôleur USB OTG peuvent être multiplexés sur le PHY interne via des registres système (`USB_WRAP` / `SYSTEM_USB_DEVICE_CONF_REG`).
- ESP-IDF permet de commuter logiciellement le PHY interne vers le contrôleur USB OTG sans graver d'eFuse, à l'initialisation du driver TinyUSB ou USB Device.

---

## 6. Présence d'un PHY USB Externe

- Sur les cartes standard ESP32-S3-DevKitC-1-N16R8, **AUCUN PHY externe n'est présent**. La carte exploite exclusivement le PHY Full-Speed 12 Mbps interne au silicium de l'ESP32-S3.
- L'utilisation d'un PHY USB externe (ULPI) nécessiterait une puce dédiée (ex: USB3300) et la mobilisation de plus de 8 broches GPIO rapides, ce qui n'est pas le cas sur le module standard N16R8.

---

## 7. Conséquences Exactes de l'eFuse `USB_PHY_SEL`

Dans le bloc d'eFuses de l'ESP32-S3 :
- Le bit `USB_PHY_SEL` sélectionne de façon permanente le contrôleur **USB-OTG** au lieu du contrôleur **USB Serial/JTAG** pour la connexion par défaut du PHY USB interne (GPIO19 / GPIO20).
- Graver cette eFuse modifie définitivement l'état matériel au reset : le PHY interne est alors connecté dès le boot au contrôleur USB-OTG plutôt qu'au périphérique USB Serial/JTAG de la ROM.
- **DANGER & INUTILITÉ** : Bien que le contrôleur visé par notre projet soit effectivement USB-OTG, graver cette eFuse est une opération physique irréversible (One-Time Programmable / OTP, un bit brûlé à 1 ne peut plus jamais être effacé). Elle est strictement inutile dans notre architecture car le stack ESP-IDF permet de commuter logiciellement le PHY interne vers le contrôleur USB-OTG au runtime de manière totalement fiable, sans aucune modification matérielle permanente.

---

## 8. Procédure de Récupération / Flash de Secours via UART Matériel

Si une mauvaise manipulation ou corruption logicielle survenait sur le port USB :
1. Connecter un câble série (ou le port USB-UART CP2102/CH343) sur les broches **TX0 (GPIO43)**, **RX0 (GPIO44)** et **GND**.
2. Forcer le passage en mode Bootloader ROM matériel :
   - Maintenir le bouton **BOOT** (GPIO0 à l'état bas).
   - Appuyer brièvement sur le bouton **RESET** (EN).
   - Relâcher le bouton **BOOT**.
3. Exécuter la commande esptool sur l'UART physique :
   ```bash
   esptool.py --port /dev/ttyUSB0 --chip esp32s3 chip_id
   esptool.py --port /dev/ttyUSB0 --chip esp32s3 erase_flash
   ```
4. Flasher le firmware via UART0 :
   ```bash
   ./tools/flash.sh /dev/ttyUSB0 115200
   ```

---

## 9. Décision Retenue Avant Toute Écriture Irréversible

1. **UART0 pour flash et logs** : Le port UART matériel reste le canal de référence pour le flashage et la console système `ESP_LOGx`.
2. **Port USB natif GPIO20/GPIO19 pour USB-OTG** : Le port Type-C n°2 est dédié à la liaison USB-OTG avec l'hôte Android Auto (DHU / autoradio).
3. **Commutation USB-OTG runtime via ESP-IDF** : Le multiplexage du PHY interne vers le contrôleur USB-OTG est opéré exclusivement par logiciel au démarrage du driver.
4. **Aucune commande `espefuse.py`** : Interdiction totale d'ajouter ou d'exécuter la moindre commande de gravure d'eFuse dans le projet.

---

## 10. Statut Explicite

**STATUT : DÉCISION VALIDÉE — TEST USB-OTG RUNTIME EN ATTENTE**
