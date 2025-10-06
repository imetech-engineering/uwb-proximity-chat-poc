/**
 * wifi_udp.h - Wi-Fi and UDP Communication Helpers
 * 
 * Handles:
 * - Wi-Fi connection and reconnection
 * - UDP packet transmission to hub
 * - Network status monitoring
 */

#ifndef WIFI_UDP_H
#define WIFI_UDP_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "config.h"
#include "utils.h"

// =============================================================================
// GLOBAL STATE
// =============================================================================

static WiFiUDP g_udp;
static bool g_wifiConnected = false;
static unsigned long g_lastWiFiCheck = 0;
static unsigned long g_lastReconnectAttempt = 0;
static int g_wifiReconnectCount = 0;

// =============================================================================
// WI-FI CONNECTION
// =============================================================================

/**
 * Initialize Wi-Fi in station mode
 * @return true if connected successfully
 */
inline bool wifi_init() {
    LOG_INFO("Connecting to Wi-Fi: %s", WIFI_SSID);
    
    // Set Wi-Fi mode to station (client)
    WiFi.mode(WIFI_STA);
    
    // Set hostname for easier identification
    char hostname[32];
    snprintf(hostname, sizeof(hostname), "UWB-Unit-%c", UNIT_ID);
    WiFi.setHostname(hostname);
    
    // Begin connection
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    // Wait for connection with timeout
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (hasTimedOut(startTime, WIFI_CONNECT_TIMEOUT_MS)) {
            LOG_ERROR("Wi-Fi connection timeout");
            return false;
        }
        
        delay(500);
        Serial.print(".");
    }
    
    Serial.println();
    g_wifiConnected = true;
    
    // Print connection info
    LOG_INFO("Wi-Fi connected!");
    LOG_INFO("  IP address: %s", WiFi.localIP().toString().c_str());
    LOG_INFO("  RSSI: %d dBm", WiFi.RSSI());
    LOG_INFO("  Hostname: %s", hostname);
    
    return true;
}

/**
 * Check if Wi-Fi is connected
 * @return true if connected
 */
inline bool wifi_is_connected() {
    return (WiFi.status() == WL_CONNECTED);
}

/**
 * Monitor Wi-Fi connection and attempt reconnection if needed
 * Call this periodically from main loop
 */
inline void wifi_monitor() {
    unsigned long now = millis();
    
    // Check connection status periodically
    if (now - g_lastWiFiCheck < 5000) {
        return;  // Check every 5 seconds
    }
    g_lastWiFiCheck = now;
    
    bool connected = wifi_is_connected();
    
    // State change detection
    if (connected && !g_wifiConnected) {
        LOG_INFO("Wi-Fi reconnected (IP: %s)", WiFi.localIP().toString().c_str());
        g_wifiConnected = true;
        g_wifiReconnectCount++;
    } else if (!connected && g_wifiConnected) {
        LOG_WARN("Wi-Fi disconnected");
        g_wifiConnected = false;
    }
    
    // Attempt reconnection if disconnected
    if (!connected) {
        if (now - g_lastReconnectAttempt >= WIFI_RECONNECT_INTERVAL_MS) {
            LOG_INFO("Attempting Wi-Fi reconnection...");
            g_lastReconnectAttempt = now;
            WiFi.reconnect();
        }
    }
}

/**
 * Get Wi-Fi signal strength
 * @return RSSI in dBm
 */
inline int wifi_get_rssi() {
    return WiFi.RSSI();
}

/**
 * Print Wi-Fi status information
 */
inline void wifi_print_status() {
    if (wifi_is_connected()) {
        LOG_INFO("Wi-Fi: Connected");
        LOG_INFO("  SSID: %s", WiFi.SSID().c_str());
        LOG_INFO("  IP: %s", WiFi.localIP().toString().c_str());
        LOG_INFO("  RSSI: %d dBm", WiFi.RSSI());
        LOG_INFO("  Reconnects: %d", g_wifiReconnectCount);
    } else {
        LOG_INFO("Wi-Fi: Disconnected");
    }
}

// =============================================================================
// UDP COMMUNICATION
// =============================================================================

/**
 * Initialize UDP client
 * @return true if successful
 */
inline bool udp_init() {
    // UDP doesn't require explicit "connection" like TCP
    // Just log that we're ready
    LOG_INFO("UDP client initialized (hub: %s:%d)", HUB_UDP_IP, HUB_UDP_PORT);
    return true;
}

/**
 * Send UDP packet to hub
 * @param data Pointer to data buffer
 * @param length Length of data in bytes
 * @return true if sent successfully
 */
inline bool udp_send(const char* data, size_t length) {
    if (!wifi_is_connected()) {
        LOG_WARN("Cannot send UDP: Wi-Fi not connected");
        return false;
    }
    
    // Parse hub IP address
    IPAddress hubIP;
    if (!hubIP.fromString(HUB_UDP_IP)) {
        LOG_ERROR("Invalid hub IP address: %s", HUB_UDP_IP);
        return false;
    }
    
    // Send UDP packet with retry logic
    for (int attempt = 0; attempt < UDP_RETRY_COUNT; attempt++) {
        g_udp.beginPacket(hubIP, HUB_UDP_PORT);
        size_t written = g_udp.write((const uint8_t*)data, length);
        
        if (g_udp.endPacket()) {
            if (written == length) {
                LOG_TRACE("UDP sent (%d bytes): %s", length, data);
                return true;
            } else {
                LOG_WARN("UDP partial write: %d/%d bytes", written, length);
            }
        }
        
        // Retry with backoff
        if (attempt < UDP_RETRY_COUNT - 1) {
            LOG_DEBUG("UDP send failed, retry %d/%d", attempt + 1, UDP_RETRY_COUNT);
            delay(UDP_RETRY_DELAY_MS * (attempt + 1));
        }
    }
    
    LOG_ERROR("UDP send failed after %d attempts", UDP_RETRY_COUNT);
    return false;
}

/**
 * Send distance measurement to hub
 * @param node Source node ID
 * @param peer Peer node ID
 * @param distance Distance in meters
 * @param quality Quality metric (0.0-1.0)
 * @return true if sent successfully
 */
inline bool udp_send_distance(char node, char peer, float distance, float quality) {
    // Build JSON message
    char jsonBuffer[JSON_MAX_SIZE];
    buildDistanceJSON(node, peer, distance, quality, jsonBuffer, sizeof(jsonBuffer));
    
    // Send to hub
    bool success = udp_send(jsonBuffer, strlen(jsonBuffer));
    
    if (success) {
        LOG_DEBUG("Sent: %c->%c: %.2fm (Q=%.2f)", node, peer, distance, quality);
    }
    
    return success;
}

/**
 * Send heartbeat/keepalive message to hub
 * @return true if sent successfully
 */
inline bool udp_send_heartbeat() {
    char jsonBuffer[JSON_MAX_SIZE];
    snprintf(jsonBuffer, sizeof(jsonBuffer),
             "{\"node\":\"%c\",\"type\":\"heartbeat\",\"ts\":%lu,\"rssi\":%d}",
             UNIT_ID, getTimestamp(), wifi_get_rssi());
    
    return udp_send(jsonBuffer, strlen(jsonBuffer));
}

/**
 * Send error/status message to hub
 * @param message Error or status message
 * @return true if sent successfully
 */
inline bool udp_send_status(const char* message) {
    char jsonBuffer[JSON_MAX_SIZE];
    snprintf(jsonBuffer, sizeof(jsonBuffer),
             "{\"node\":\"%c\",\"type\":\"status\",\"msg\":\"%s\",\"ts\":%lu}",
             UNIT_ID, message, getTimestamp());
    
    return udp_send(jsonBuffer, strlen(jsonBuffer));
}

// =============================================================================
// NETWORK DIAGNOSTICS
// =============================================================================

/**
 * Perform network diagnostics
 * Tests connectivity to hub and reports results
 */
inline void network_diagnostics() {
    LOG_INFO("Running network diagnostics...");
    
    // Check Wi-Fi
    if (!wifi_is_connected()) {
        LOG_ERROR("  [FAIL] Wi-Fi not connected");
        return;
    }
    LOG_INFO("  [OK] Wi-Fi connected");
    
    // Check IP address
    IPAddress localIP = WiFi.localIP();
    if (localIP == IPAddress(0, 0, 0, 0)) {
        LOG_ERROR("  [FAIL] No IP address assigned");
        return;
    }
    LOG_INFO("  [OK] IP address: %s", localIP.toString().c_str());
    
    // Check hub IP validity
    IPAddress hubIP;
    if (!hubIP.fromString(HUB_UDP_IP)) {
        LOG_ERROR("  [FAIL] Invalid hub IP: %s", HUB_UDP_IP);
        return;
    }
    LOG_INFO("  [OK] Hub IP valid: %s", HUB_UDP_IP);
    
    // Try to send test packet
    LOG_INFO("  Testing UDP transmission...");
    if (udp_send_status("diagnostics")) {
        LOG_INFO("  [OK] UDP test packet sent");
    } else {
        LOG_ERROR("  [FAIL] UDP send failed");
    }
    
    LOG_INFO("Diagnostics complete");
}

/**
 * Get network statistics
 */
struct NetworkStats {
    bool connected;
    int rssi;
    IPAddress ip;
    int reconnectCount;
};

inline NetworkStats network_get_stats() {
    NetworkStats stats;
    stats.connected = wifi_is_connected();
    stats.rssi = wifi_get_rssi();
    stats.ip = WiFi.localIP();
    stats.reconnectCount = g_wifiReconnectCount;
    return stats;
}

/**
 * Print network statistics
 */
inline void network_print_stats() {
    NetworkStats stats = network_get_stats();
    LOG_INFO("Network Stats:");
    LOG_INFO("  Connected: %s", stats.connected ? "Yes" : "No");
    if (stats.connected) {
        LOG_INFO("  IP: %s", stats.ip.toString().c_str());
        LOG_INFO("  RSSI: %d dBm", stats.rssi);
    }
    LOG_INFO("  Reconnects: %d", stats.reconnectCount);
}

#endif // WIFI_UDP_H

