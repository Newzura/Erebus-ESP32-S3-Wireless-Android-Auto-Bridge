package com.example

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.network.ErebusNetworkManager
import com.example.network.ErebusUiState
import com.example.ui.theme.MyApplicationTheme

class MainActivity : ComponentActivity() {
    private lateinit var networkManager: ErebusNetworkManager

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        networkManager = ErebusNetworkManager(applicationContext)

        setContent {
            MyApplicationTheme {
                val uiState by networkManager.uiState.collectAsState()

                Scaffold(
                    modifier = Modifier.fillMaxSize(),
                    topBar = { ErebusTopBar() }
                ) { innerPadding ->
                    ErebusDashboard(
                        state = uiState,
                        onRequestConnect = { networkManager.requestErebusNetwork() },
                        onDisconnect = { networkManager.releaseNetwork() },
                        modifier = Modifier
                            .fillMaxSize()
                            .padding(innerPadding)
                    )
                }
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        networkManager.releaseNetwork()
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ErebusTopBar() {
    TopAppBar(
        title = {
            Column {
                Text(
                    text = "Erebus Bridge",
                    fontWeight = FontWeight.Bold,
                    fontSize = 20.sp
                )
                Text(
                    text = "ESP32-S3 Local Companion (Multi-Network)",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        },
        colors = TopAppBarDefaults.topAppBarColors(
            containerColor = MaterialTheme.colorScheme.surfaceContainer
        )
    )
}

@Composable
fun ErebusDashboard(
    state: ErebusUiState,
    onRequestConnect: () -> Unit,
    onDisconnect: () -> Unit,
    modifier: Modifier = Modifier
) {
    LazyColumn(
        modifier = modifier
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(16.dp)
    ) {
        // 1. Primary Network Status Card
        item {
            NetworkStatusCard(state = state)
        }

        // 2. Control Buttons
        item {
            ControlSection(
                state = state,
                onRequestConnect = onRequestConnect,
                onDisconnect = onDisconnect
            )
        }

        // 3. Network Architecture & Isolation Notice Card
        item {
            NetworkPolicyCard(state = state)
        }

        // 4. Activity Logs
        item {
            DiagnosticsLogsCard(logs = state.recentLogs)
        }
    }
}

@Composable
fun NetworkStatusCard(state: ErebusUiState) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .testTag("network_status_card"),
        shape = RoundedCornerShape(16.dp),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.surfaceVariant
        )
    ) {
        Column(
            modifier = Modifier.padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            Text(
                text = "État des Liaisons Réseau",
                style = MaterialTheme.typography.titleMedium,
                fontWeight = FontWeight.Bold
            )

            HorizontalDivider()

            // Wi-Fi Erebus
            StatusRow(
                label = "Wi‑Fi Erebus",
                detail = "SSID: Erebus (Canal 6)",
                isConnected = state.wifiErebusConnected,
                connectedText = "Connecté",
                disconnectedText = "Déconnecté"
            )

            // Réseau local S3
            StatusRow(
                label = "Réseau local S3",
                detail = "Passerelle 192.168.4.1/24",
                isConnected = state.localNetworkAvailable,
                connectedText = "Disponible",
                disconnectedText = "Indisponible"
            )

            // Socket TCP 192.168.4.1:5288
            StatusRow(
                label = "Socket TCP 5288",
                detail = "192.168.4.1:5288 (Transport local)",
                isConnected = state.socketConnected,
                connectedText = "Connecté",
                disconnectedText = "Déconnecté"
            )

            HorizontalDivider()

            // Données mobiles
            InfoRow(
                label = "Données mobiles (4G/5G)",
                value = "Préservées pour le système (non gérées par l'app)"
            )

            // Internet téléphone
            InfoRow(
                label = "Internet téléphone",
                value = "Actif via réseau mobile — indépendant du Wi‑Fi Erebus"
            )
        }
    }
}

@Composable
fun StatusRow(
    label: String,
    detail: String,
    isConnected: Boolean,
    connectedText: String,
    disconnectedText: String
) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = label,
                style = MaterialTheme.typography.bodyMedium,
                fontWeight = FontWeight.SemiBold
            )
            Text(
                text = detail,
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
        }

        StatusBadge(
            isActive = isConnected,
            text = if (isConnected) connectedText else disconnectedText
        )
    }
}

@Composable
fun StatusBadge(isActive: Boolean, text: String) {
    val bgColor = if (isActive) Color(0xFF2E7D32) else Color(0xFFC62828)
    val contentColor = Color.White

    Surface(
        color = bgColor,
        shape = RoundedCornerShape(12.dp)
    ) {
        Row(
            modifier = Modifier.padding(horizontal = 10.dp, vertical = 4.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Box(
                modifier = Modifier
                    .size(8.dp)
                    .background(contentColor, CircleShape)
            )
            Spacer(modifier = Modifier.width(6.dp))
            Text(
                text = text,
                color = contentColor,
                fontSize = 12.sp,
                fontWeight = FontWeight.Bold
            )
        }
    }
}

@Composable
fun InfoRow(label: String, value: String) {
    Column {
        Text(
            text = label,
            style = MaterialTheme.typography.bodySmall,
            fontWeight = FontWeight.SemiBold,
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )
        Text(
            text = value,
            style = MaterialTheme.typography.bodySmall,
            color = MaterialTheme.colorScheme.onSurface
        )
    }
}

@Composable
fun ControlSection(
    state: ErebusUiState,
    onRequestConnect: () -> Unit,
    onDisconnect: () -> Unit
) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        Button(
            onClick = onRequestConnect,
            enabled = !state.wifiErebusConnected && !state.isRequestingNetwork,
            modifier = Modifier
                .weight(1f)
                .testTag("connect_button")
        ) {
            Text(
                text = if (state.isRequestingNetwork) "Recherche..." else "Rejoindre Erebus"
            )
        }

        OutlinedButton(
            onClick = onDisconnect,
            enabled = state.wifiErebusConnected || state.isRequestingNetwork,
            modifier = Modifier
                .weight(1f)
                .testTag("disconnect_button"),
            colors = ButtonDefaults.outlinedButtonColors(
                contentColor = MaterialTheme.colorScheme.error
            )
        ) {
            Text("Libérer")
        }
    }
}

@Composable
fun NetworkPolicyCard(state: ErebusUiState) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        shape = RoundedCornerShape(12.dp),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.surfaceContainerLow
        )
    ) {
        Column(
            modifier = Modifier.padding(14.dp),
            verticalArrangement = Arrangement.spacedBy(6.dp)
        ) {
            Text(
                text = "Spécificité d'accès local",
                style = MaterialTheme.typography.titleSmall,
                fontWeight = FontWeight.Bold
            )
            Text(
                text = "• Le réseau Wi‑Fi Erebus est un réseau d'accessoire local sans passerelle Internet publique.\n" +
                       "• L'application n'utilise pas de liaison globale (bindProcessToNetwork) : vos données mobiles 4G/5G restent prioritaires pour toutes les autres applications.\n" +
                       "• Le socket local vers 192.168.4.1:5288 est exclusivement bindé au Network Wi‑Fi Erebus.",
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
        }
    }
}

@Composable
fun DiagnosticsLogsCard(logs: List<String>) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .testTag("diagnostics_card"),
        shape = RoundedCornerShape(12.dp),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.surfaceContainerHighest
        )
    ) {
        Column(
            modifier = Modifier.padding(14.dp),
            verticalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Text(
                text = "Journal local des événements",
                style = MaterialTheme.typography.titleSmall,
                fontWeight = FontWeight.Bold
            )

            if (logs.isEmpty()) {
                Text(
                    text = "Aucun événement pour le moment.",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            } else {
                logs.reversed().take(8).forEach { entry ->
                    Text(
                        text = entry,
                        fontFamily = FontFamily.Monospace,
                        fontSize = 11.sp,
                        color = MaterialTheme.colorScheme.onSurface
                    )
                }
            }
        }
    }
}

