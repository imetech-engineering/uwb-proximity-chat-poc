/**
 * main.cpp - UWB Proximity Chat Unit Firmware
 * 
 * Arduino-style entry point for ESP32 + DW3000 ranging unit.
 * 
 * Architecture:
 * - setup(): Initialize hardware, Wi-Fi, and UWB
 * - loop(): Perform ranging cycles and send results to hub
 * 
 * This firmware measures distances to peer units using UWB ranging
 * and transmits results to a Raspberry Pi hub via UDP.
 * 
 * Hardware Required:
 * - ESP32 DevKit (or compatible)
 * - DW3000 UWB module
 * - Wi-Fi network access
 * 
 * Configuration:
 * - Edit config.h to set Wi-Fi credentials, hub IP, and unit ID
 * - See docs/WIRING.md for hardware connections
 * - See docs/CALIBRATION.md for ranging calibration
 */

#include <Arduino.h>
#include "config.h"
#include "utils.h"
#include "dw3000_driver.h"
#include "wifi_udp.h"

// =============================================================================
// GLOBAL STATE
// =============================================================================

// Performance statistics
struct SystemStats {
    unsigned long loopCount;
    unsigned long rangingAttempts;
    unsigned long rangingSuccesses;
    unsigned long rangingFailures;
    unsigned long udpSendSuccess;
    unsigned long udpSendFailures;
    float avgLoopTime;
} g_stats = {0};

// Timing variables
unsigned long g_lastHeartbeat = 0;
unsigned long g_lastStatsReport = 0;
unsigned long g_lastMemCheck = 0;

// Current peer index for ranging
int g_currentPeerIdx = 0;

// Loop timing measurement
RunningAverage<100> g_loopTimeAvg;

// =============================================================================
// SETUP - Initialize Hardware and Network
// =============================================================================

void setup() {
    // Initialize serial communication
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000);  // Wait for serial to stabilize
    
    LOG_INFO("");
    LOG_INFO("========================================");
    LOG_INFO("  UWB Proximity Chat Unit");
    LOG_INFO("  Firmware v0.1.0");
    LOG_INFO("========================================");
    LOG_INFO("");
    
    // Print system information
    printSystemInfo();
    
    // Initialize heartbeat LED
#if HEARTBEAT_ENABLE
    pinMode(HEARTBEAT_LED_PIN, OUTPUT);
    digitalWrite(HEARTBEAT_LED_PIN, LOW);
    blinkLED(3, 200, 200);  // 3 blinks to indicate startup
#endif
    
    // Initialize watchdog timer
#if ENABLE_WATCHDOG
    // Note: ESP32 Arduino core has built-in task watchdog
    // No explicit initialization needed, but we could configure it here
    LOG_INFO("Watchdog enabled: %d sec timeout", WATCHDOG_TIMEOUT_SEC);
#endif
    
    // Initialize Wi-Fi
    LOG_INFO("");
    LOG_INFO("Initializing Wi-Fi...");
    if (!wifi_init()) {
        LOG_ERROR("Wi-Fi initialization failed!");
        LOG_ERROR("Check SSID and password in config.h");
        blinkLED(5, 100, 100);  // Error indication
        
        // In production, might want to retry or enter config mode
        // For POC, we'll continue and retry in loop
    }
    
    // Initialize UDP
    LOG_INFO("");
    LOG_INFO("Initializing UDP...");
    if (!udp_init()) {
        LOG_ERROR("UDP initialization failed!");
    }
    
    // Run network diagnostics
    network_diagnostics();
    
    // Initialize UWB module
    LOG_INFO("");
    LOG_INFO("Initializing UWB module...");
    if (!uwb_init()) {
        LOG_ERROR("UWB initialization failed!");
        
#if !ENABLE_SIMULATION
        LOG_ERROR("Check DW3000 wiring (see docs/WIRING.md)");
        blinkLED(10, 100, 100);  // Error indication
        
        // Could enter error state or retry
        // For POC, continue (will retry in loop)
#else
        LOG_WARN("Continuing in simulation mode");
#endif
    }
    
    // Send startup notification to hub
    udp_send_status("startup");
    
    // Initialization complete
    LOG_INFO("");
    LOG_INFO("========================================");
    LOG_INFO("  Initialization Complete");
    LOG_INFO("  Starting main loop...");
    LOG_INFO("========================================");
    LOG_INFO("");
    
    blinkLED(2, 500, 200);  // Success indication
    
    // Initialize timing
    g_lastHeartbeat = millis();
    g_lastStatsReport = millis();
    g_lastMemCheck = millis();
}

// =============================================================================
// MAIN LOOP - Ranging and Communication
// =============================================================================

void loop() {
    unsigned long loopStart = micros();
    
    // Increment loop counter
    g_stats.loopCount++;
    
    // Monitor Wi-Fi connection
    wifi_monitor();
    
    // Check if we're connected before attempting ranging/transmission
    if (!wifi_is_connected()) {
        // Not connected, just wait and retry
        delay(1000);
        return;
    }
    
    // Check if UWB is ready
    if (!uwb_is_ready()) {
        // Try to reinitialize
        if (millis() % 5000 == 0) {  // Every 5 seconds
            LOG_WARN("UWB not ready, attempting reinitialization...");
            uwb_init();
        }
        delay(1000);
        return;
    }
    
    // -------------------------------------------------------------------------
    // RANGING CYCLE
    // -------------------------------------------------------------------------
    
    // Check if it's our time slot
    if (isMyTimeSlot()) {
        // Determine which peer to range with
        char peerID = PEER_IDS[g_currentPeerIdx];
        
        // Skip self
        if (peerID == UNIT_ID) {
            g_currentPeerIdx = (g_currentPeerIdx + 1) % NUM_PEERS;
            delay(10);
            return;
        }
        
        LOG_DEBUG("Ranging cycle: %c -> %c", UNIT_ID, peerID);
        
        // Perform ranging
        RangingResult result;
        g_stats.rangingAttempts++;
        
        if (uwb_range(peerID, result)) {
            g_stats.rangingSuccesses++;
            
            // Check quality threshold
            if (result.quality >= QUALITY_THRESHOLD) {
                // Send result to hub
                if (udp_send_distance(UNIT_ID, peerID, result.distance, result.quality)) {
                    g_stats.udpSendSuccess++;
                    
                    LOG_INFO("Range: %c->%c = %.2f m (Q=%.2f) [SENT]", 
                             UNIT_ID, peerID, result.distance, result.quality);
                } else {
                    g_stats.udpSendFailures++;
                    LOG_WARN("Range: %c->%c = %.2f m (Q=%.2f) [SEND FAILED]", 
                             UNIT_ID, peerID, result.distance, result.quality);
                }
            } else {
                LOG_WARN("Range: %c->%c = %.2f m (Q=%.2f) [LOW QUALITY]", 
                         UNIT_ID, peerID, result.distance, result.quality);
            }
        } else {
            g_stats.rangingFailures++;
            LOG_WARN("Ranging failed: %c -> %c", UNIT_ID, peerID);
        }
        
        // Move to next peer
        g_currentPeerIdx = (g_currentPeerIdx + 1) % NUM_PEERS;
        
        // Wait before next ranging attempt
        delay(RANGING_INTERVAL_MS);
        
    } else {
        // Not our time slot, wait briefly
        delay(50);
    }
    
    // -------------------------------------------------------------------------
    // PERIODIC TASKS
    // -------------------------------------------------------------------------
    
    unsigned long now = millis();
    
    // Heartbeat
#if HEARTBEAT_ENABLE
    if (now - g_lastHeartbeat >= HEARTBEAT_INTERVAL_MS) {
        g_lastHeartbeat = now;
        
        // Toggle LED
        static bool ledState = false;
        digitalWrite(HEARTBEAT_LED_PIN, ledState ? HIGH : LOW);
        ledState = !ledState;
        
        // Send heartbeat to hub
        udp_send_heartbeat();
        
        LOG_TRACE("Heartbeat");
    }
#endif
    
    // Performance statistics
#if ENABLE_PERF_STATS
    if (now - g_lastStatsReport >= PERF_STATS_INTERVAL_MS) {
        g_lastStatsReport = now;
        
        LOG_INFO("========================================");
        LOG_INFO("Performance Statistics:");
        LOG_INFO("  Loops: %lu", g_stats.loopCount);
        LOG_INFO("  Avg Loop Time: %.2f ms", g_stats.avgLoopTime);
        LOG_INFO("  Ranging: %lu attempts, %lu success, %lu fail", 
                 g_stats.rangingAttempts, 
                 g_stats.rangingSuccesses, 
                 g_stats.rangingFailures);
        
        if (g_stats.rangingAttempts > 0) {
            float successRate = 100.0f * g_stats.rangingSuccesses / g_stats.rangingAttempts;
            LOG_INFO("  Success Rate: %.1f%%", successRate);
        }
        
        LOG_INFO("  UDP: %lu success, %lu fail", 
                 g_stats.udpSendSuccess, 
                 g_stats.udpSendFailures);
        
        LOG_INFO("========================================");
        
        // Print network status
        network_print_stats();
        
        // Print UWB status
        uwb_print_status();
    }
#endif
    
    // Memory check
#if ENABLE_MEM_CHECK
    if (now - g_lastMemCheck >= MEM_CHECK_INTERVAL_MS) {
        g_lastMemCheck = now;
        checkMemory();
    }
#endif
    
    // -------------------------------------------------------------------------
    // LOOP TIMING
    // -------------------------------------------------------------------------
    
    unsigned long loopEnd = micros();
    unsigned long loopTime = loopEnd - loopStart;
    
    g_loopTimeAvg.add(loopTime / 1000.0f);  // Convert to ms
    g_stats.avgLoopTime = g_loopTimeAvg.get();
    
    LOG_TRACE("Loop time: %.2f ms", loopTime / 1000.0f);
    
    // Feed watchdog (if implemented)
    // For ESP32 Arduino, the framework handles this automatically
    // but we could explicitly yield or call watchdog reset here
    yield();
}

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

/**
 * Error handler - called when critical error occurs
 * @param message Error message
 */
void handleError(const char* message) {
    LOG_ERROR("CRITICAL ERROR: %s", message);
    
    // Send error to hub
    udp_send_status(message);
    
    // Blink LED rapidly
    blinkLED(20, 50, 50);
    
    // In production, might want to reset or enter safe mode
    // For POC, we'll continue and let watchdog handle hung state
}

/**
 * Reset system statistics
 */
void resetStats() {
    memset(&g_stats, 0, sizeof(g_stats));
    g_loopTimeAvg.reset();
    LOG_INFO("Statistics reset");
}

// =============================================================================
// ARDUINO FRAMEWORK CALLBACKS
// =============================================================================

/**
 * Called when ESP32 is about to reset
 * Opportunity to save state or notify hub
 */
void __attribute__((weak)) onReset() {
    LOG_WARN("System resetting...");
    udp_send_status("reset");
    delay(100);  // Give time for message to send
}

// Note: For more advanced features, could add:
// - OTA (Over-The-Air) firmware updates
// - Configuration via web interface or BLE
// - Sleep modes for power saving
// - Multi-task implementation with FreeRTOS tasks
// - More sophisticated collision avoidance (TDMA, CSMA/CA)

