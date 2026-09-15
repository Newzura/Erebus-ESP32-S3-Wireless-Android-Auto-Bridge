# Directives de Contribution — Erebus

Bienvenue sur le projet **Erebus**. Pour garantir la rigueur technique et la reproductibilité du projet :

1. **Aucun code Cloud ou Télémétrie** : Les contributions ne doivent comporter aucun SDK externe, aucune API tierce et aucune dépendance cloud.
2. **Honnêteté des Logs** : Ne jamais ajouter de statut fictif ou d'étiquette trompeuse (par exemple `AA_ACTIVE`) sans flux vérifié avec le DHU.
3. **Sécurité des Données & Secrets** : Aucun secret, identifiant Wi-Fi réel ou clé privée ne doit être committé. Toujours utiliser les fichiers gabarits `*.example.*`.
4. **Validation du Build** :
   - Tout changement sur le firmware doit compiler avec `idf.py build` ciblant l'ESP32-S3 N16R8.
   - Tout changement sur l'application Android doit compiler avec `./gradlew assembleDebug`.
5. **Mise à jour du statut** : Chaque avancée doit être consignée dans le tableau de bord `docs/protocol-status.md`.
