package com.example.network

import android.content.Context
import android.net.ConnectivityManager
import android.net.Network
import android.net.NetworkCapabilities
import android.net.NetworkRequest
import android.net.wifi.WifiNetworkSpecifier
import android.os.Build
import android.util.Log
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import java.io.InputStream
import java.io.OutputStream
import java.net.InetSocketAddress
import java.net.Socket
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

data class ErebusUiState(
    val wifiErebusConnected: Boolean = false,
    val localNetworkAvailable: Boolean = false,
    val socketConnected: Boolean = false,
    val isRequestingNetwork: Boolean = false,
    val targetIp: String = "192.168.4.1",
    val targetPort: Int = 5288,
    val rxBytes: Long = 0,
    val txBytes: Long = 0,
    val mobileDataStatus: String = "Non gérée par l'app (4G/5G laissée au système)",
    val internetStatus: String = "Indépendant d'Erebus (réseau local sans Internet)",
    val statusMessage: String = "Prêt à se connecter au SoftAP Erebus",
    val recentLogs: List<String> = emptyList()
)

class ErebusNetworkManager(private val context: Context) {

    private val connectivityManager =
        context.getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager

    private val scope = CoroutineScope(Dispatchers.IO + Job())

    private val _uiState = MutableStateFlow(ErebusUiState())
    val uiState: StateFlow<ErebusUiState> = _uiState.asStateFlow()

    private var activeNetwork: Network? = null
    private var networkCallback: ConnectivityManager.NetworkCallback? = null
    private var activeSocket: Socket? = null
    private var socketJob: Job? = null

    companion object {
        private const val TAG = "ErebusNetMgr"
        const val EREBUS_SSID = "Erebus"
        const val EREBUS_PASSPHRASE = "changeme"
        const val EREBUS_GATEWAY_IP = "192.168.4.1"
        const val EREBUS_PORT = 5288
    }

    private fun appendLog(message: String) {
        val timestamp = SimpleDateFormat("HH:mm:ss", Locale.getDefault()).format(Date())
        val entry = "[$timestamp] $message"
        Log.i(TAG, entry)
        _uiState.value = _uiState.value.copy(
            recentLogs = (_uiState.value.recentLogs + entry).takeLast(20)
        )
    }

    /**
     * Demande explicitement le réseau Wi-Fi local Erebus via WifiNetworkSpecifier.
     * NE FAIT PAS de bindProcessToNetwork() afin que tout le reste du système et des
     * applications continue à utiliser la 4G/5G pour Internet.
     */
    fun requestErebusNetwork() {
        if (networkCallback != null) {
            appendLog("Requête réseau déjà en cours")
            return
        }

        appendLog("Création NetworkRequest pour SSID \"$EREBUS_SSID\"")
        _uiState.value = _uiState.value.copy(
            isRequestingNetwork = true,
            statusMessage = "Recherche et association au réseau Wi-Fi Erebus..."
        )

        val callback = object : ConnectivityManager.NetworkCallback() {
            override fun onAvailable(network: Network) {
                appendLog("Réseau Wi-Fi Erebus disponible (Network ID: $network)")
                activeNetwork = network

                _uiState.value = _uiState.value.copy(
                    wifiErebusConnected = true,
                    localNetworkAvailable = true,
                    isRequestingNetwork = false,
                    statusMessage = "Connecté au Wi-Fi Erebus (192.168.4.x). Lancement socket..."
                )

                // Lancer la connexion du socket local en utilisant la SocketFactory du Network Erebus
                startLocalSocketTransport(network)
            }

            override fun onLost(network: Network) {
                appendLog("Réseau Wi-Fi Erebus perdu")
                if (activeNetwork == network) {
                    activeNetwork = null
                }
                closeLocalSocket()

                _uiState.value = _uiState.value.copy(
                    wifiErebusConnected = false,
                    localNetworkAvailable = false,
                    socketConnected = false,
                    statusMessage = "Réseau Erebus déconnecté. En attente..."
                )
            }

            override fun onUnavailable() {
                appendLog("Réseau Erebus indisponible ou refusé par l'utilisateur")
                _uiState.value = _uiState.value.copy(
                    wifiErebusConnected = false,
                    localNetworkAvailable = false,
                    isRequestingNetwork = false,
                    statusMessage = "Réseau Erebus non détecté. Vérifier l'ESP32-S3."
                )
            }
        }

        networkCallback = callback

        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                val specifier = WifiNetworkSpecifier.Builder()
                    .setSsid(EREBUS_SSID)
                    .setWpa2Passphrase(EREBUS_PASSPHRASE)
                    .build()

                val request = NetworkRequest.Builder()
                    .addTransportType(NetworkCapabilities.TRANSPORT_WIFI)
                    .removeCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET) // Explicitly local-only (no internet required)
                    .setNetworkSpecifier(specifier)
                    .build()

                connectivityManager.requestNetwork(request, callback)
            } else {
                // Fallback for pre-Android 10
                val request = NetworkRequest.Builder()
                    .addTransportType(NetworkCapabilities.TRANSPORT_WIFI)
                    .build()
                connectivityManager.requestNetwork(request, callback)
            }
        } catch (e: Exception) {
            appendLog("Erreur lors de la requête réseau: ${e.message}")
            _uiState.value = _uiState.value.copy(
                isRequestingNetwork = false,
                statusMessage = "Erreur: ${e.message}"
            )
        }
    }

    /**
     * Ouvre et maintient le socket TCP local vers 192.168.4.1:5288 en passant
     * EXCLUSIVEMENT par network.socketFactory.
     */
    private fun startLocalSocketTransport(network: Network) {
        socketJob?.cancel()
        socketJob = scope.launch {
            while (isActive && activeNetwork == network) {
                try {
                    appendLog("Ouverture socket TCP vers $EREBUS_GATEWAY_IP:$EREBUS_PORT...")
                    // Socket créé via la factory du réseau Erebus
                    val socket = network.socketFactory.createSocket()
                    activeSocket = socket

                    socket.connect(InetSocketAddress(EREBUS_GATEWAY_IP, EREBUS_PORT), 4000)
                    socket.soTimeout = 5000

                    appendLog("Socket TCP connecté à $EREBUS_GATEWAY_IP:$EREBUS_PORT")
                    _uiState.value = _uiState.value.copy(
                        socketConnected = true,
                        statusMessage = "Socket local connecté à l'ESP32-S3"
                    )

                    val inputStream: InputStream = socket.getInputStream()
                    val outputStream: OutputStream = socket.getOutputStream()
                    val buffer = ByteArray(2048)

                    while (isActive && socket.isConnected && !socket.isClosed) {
                        val readBytes = inputStream.read(buffer)
                        if (readBytes > 0) {
                            _uiState.value = _uiState.value.copy(
                                rxBytes = _uiState.value.rxBytes + readBytes
                            )
                        } else if (readBytes == -1) {
                            appendLog("Socket fermé par l'ESP32-S3")
                            break
                        }
                    }
                } catch (e: Exception) {
                    // Ne pas afficher comme une erreur bloquante
                    _uiState.value = _uiState.value.copy(socketConnected = false)
                    // Attente avant reconnexion du socket
                    delay(3000)
                } finally {
                    closeLocalSocket()
                    _uiState.value = _uiState.value.copy(socketConnected = false)
                }
            }
        }
    }

    private fun closeLocalSocket() {
        try {
            activeSocket?.close()
        } catch (_: Exception) {}
        activeSocket = null
    }

    /**
     * Libère la demande de réseau auprès du système.
     */
    fun releaseNetwork() {
        socketJob?.cancel()
        closeLocalSocket()

        networkCallback?.let {
            try {
                connectivityManager.unregisterNetworkCallback(it)
                appendLog("Demande réseau Erebus libérée")
            } catch (e: Exception) {
                Log.w(TAG, "Exception unregistering network callback: ${e.message}")
            }
        }
        networkCallback = null
        activeNetwork = null

        _uiState.value = _uiState.value.copy(
            wifiErebusConnected = false,
            localNetworkAvailable = false,
            socketConnected = false,
            isRequestingNetwork = false,
            statusMessage = "Déconnecté"
        )
    }
}
