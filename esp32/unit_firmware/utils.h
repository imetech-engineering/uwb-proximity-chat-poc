/**
 * utils.h - Utility Functions for UWB Proximity Chat
 * 
 * Helper functions for:
 * - Time management
 * - Memory monitoring
 * - String formatting
 * - Mathematical utilities
 */

#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include "config.h"

// =============================================================================
// TIME UTILITIES
// =============================================================================

/**
 * Get current time in milliseconds since boot
 * Wrapper around millis() for consistency
 */
inline unsigned long getTimeMillis() {
    return millis();
}

/**
 * Get current time in microseconds since boot
 * Useful for high-precision timing
 */
inline unsigned long getTimeMicros() {
    return micros();
}

/**
 * Get current timestamp for JSON messages (Unix epoch seconds)
 * Note: ESP32 needs NTP sync for accurate wall-clock time
 * For POC, we use relative timestamps (millis()/1000)
 */
inline unsigned long getTimestamp() {
    // In production, use: time(nullptr) after NTP sync
    // For POC: just use seconds since boot
    return millis() / 1000;
}

/**
 * Check if a timeout has elapsed
 * @param startTime The starting timestamp (from millis())
 * @param timeoutMs The timeout duration in milliseconds
 * @return true if timeout has elapsed
 */
inline bool hasTimedOut(unsigned long startTime, unsigned long timeoutMs) {
    return (millis() - startTime) >= timeoutMs;
}

/**
 * Calculate time slot for this unit based on cycle timing
 * Returns true if currently in this unit's transmission slot
 */
inline bool isMyTimeSlot() {
    unsigned long cycleTime = millis() % CYCLE_DURATION_MS;
    unsigned long slotStart = MY_SLOT_OFFSET_MS;
    unsigned long slotEnd = slotStart + TIME_SLOT_DURATION_MS;
    return (cycleTime >= slotStart && cycleTime < slotEnd);
}

/**
 * Calculate which peer to range with based on round-robin schedule
 * @return Index of peer to range with (0, 1, or 2)
 */
inline int getCurrentPeerIndex() {
    // Rotate through peers within our time slot
    unsigned long slotTime = (millis() - MY_SLOT_OFFSET_MS) % TIME_SLOT_DURATION_MS;
    int peerCount = 0;
    
    // Count peers (exclude self)
    for (int i = 0; i < NUM_PEERS; i++) {
        if (PEER_IDS[i] != UNIT_ID) {
            peerCount++;
        }
    }
    
    if (peerCount == 0) return -1; // No peers
    
    // Cycle through peers
    int peerIdx = (slotTime / (TIME_SLOT_DURATION_MS / peerCount)) % peerCount;
    
    // Map to actual peer (skip self)
    int actualIdx = 0;
    for (int i = 0; i < NUM_PEERS; i++) {
        if (PEER_IDS[i] != UNIT_ID) {
            if (actualIdx == peerIdx) {
                return i;
            }
            actualIdx++;
        }
    }
    
    return 0; // Fallback
}

// =============================================================================
// MEMORY UTILITIES
// =============================================================================

/**
 * Get free heap memory in bytes
 * @return Free heap size in bytes
 */
inline uint32_t getFreeHeap() {
    return ESP.getFreeHeap();
}

/**
 * Get free heap memory in kilobytes
 * @return Free heap size in KB
 */
inline uint32_t getFreeHeapKB() {
    return ESP.getFreeHeap() / 1024;
}

/**
 * Check if free memory is below warning threshold
 * Logs warning if memory is low
 * @return true if memory is low
 */
inline bool checkMemory() {
#if ENABLE_MEM_CHECK
    uint32_t freeKB = getFreeHeapKB();
    if (freeKB < MEM_WARNING_THRESHOLD_KB) {
        LOG_WARN("Low memory: %lu KB free", freeKB);
        return true;
    }
#endif
    return false;
}

/**
 * Print memory statistics to serial
 */
inline void printMemoryStats() {
    LOG_INFO("Free heap: %lu KB, Total heap: %lu KB", 
             getFreeHeapKB(), 
             ESP.getHeapSize() / 1024);
}

// =============================================================================
// STRING UTILITIES
// =============================================================================

/**
 * Format a float to fixed decimal places
 * @param value The float value
 * @param buffer Output buffer
 * @param bufSize Size of output buffer
 * @param decimals Number of decimal places
 */
inline void formatFloat(float value, char* buffer, size_t bufSize, int decimals = 2) {
    dtostrf(value, 0, decimals, buffer);
}

/**
 * Build JSON message for distance measurement
 * @param node Source node ID
 * @param peer Peer node ID
 * @param distance Distance in meters
 * @param quality Quality metric (0.0-1.0)
 * @param buffer Output buffer
 * @param bufSize Size of output buffer
 */
inline void buildDistanceJSON(char node, char peer, float distance, float quality, 
                              char* buffer, size_t bufSize) {
    char distStr[16], qualStr[16];
    formatFloat(distance, distStr, sizeof(distStr), JSON_PRECISION);
    formatFloat(quality, qualStr, sizeof(qualStr), JSON_PRECISION);
    
    snprintf(buffer, bufSize,
             "{\"node\":\"%c\",\"peer\":\"%c\",\"distance\":%s,\"quality\":%s,\"ts\":%lu}",
             node, peer, distStr, qualStr, getTimestamp());
}

// =============================================================================
// MATHEMATICAL UTILITIES
// =============================================================================

/**
 * Clamp a value between min and max
 * @param value The value to clamp
 * @param minVal Minimum value
 * @param maxVal Maximum value
 * @return Clamped value
 */
template<typename T>
inline T clamp(T value, T minVal, T maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

/**
 * Linear interpolation
 * @param a Start value
 * @param b End value
 * @param t Interpolation factor (0.0-1.0)
 * @return Interpolated value
 */
inline float lerp(float a, float b, float t) {
    return a + (b - a) * clamp(t, 0.0f, 1.0f);
}

/**
 * Map a value from one range to another
 * @param value Input value
 * @param inMin Input range minimum
 * @param inMax Input range maximum
 * @param outMin Output range minimum
 * @param outMax Output range maximum
 * @return Mapped value
 */
inline float mapRange(float value, float inMin, float inMax, float outMin, float outMax) {
    float t = (value - inMin) / (inMax - inMin);
    return lerp(outMin, outMax, t);
}

/**
 * Calculate distance from UWB time-of-flight
 * @param tof Time of flight in DW3000 device time units (~15.65ps each)
 * @return Distance in meters
 */
inline float tofToDistance(uint64_t tof) {
    // Speed of light in m/s
    const double SPEED_OF_LIGHT = 299792458.0;
    
    // DW3000 device time unit in seconds (~15.65 picoseconds)
    const double DW_TIME_UNIT = 1.0 / 499.2e6 / 128.0;
    
    // Convert to seconds, then to distance (divide by 2 for round-trip)
    double timeSeconds = tof * DW_TIME_UNIT;
    double distance = (timeSeconds * SPEED_OF_LIGHT) / 2.0;
    
    // Apply calibration offset
    distance += DIST_OFFSET_M;
    
    return (float)distance;
}

/**
 * Generate simulated distance for testing
 * Creates oscillating distances for demonstration
 * @param peer Peer ID
 * @return Simulated distance in meters
 */
inline float generateSimulatedDistance(char peer) {
#if ENABLE_SIMULATION
    // Different phase for each peer to create variety
    float phase = (peer - 'A') * 2.0 * PI / NUM_PEERS;
    
    // Oscillate distance sinusoidally
    float t = (millis() % SIM_PERIOD_MS) / (float)SIM_PERIOD_MS;
    float angle = 2.0 * PI * t + phase;
    
    float distance = SIM_BASE_DISTANCE_M + SIM_AMPLITUDE_M * sin(angle);
    
    // Ensure positive distance
    return max(0.1f, distance);
#else
    return 0.0f;
#endif
}

// =============================================================================
// CONVERSION UTILITIES
// =============================================================================

/**
 * Convert char ID to numeric index
 * @param id Unit ID ('A', 'B', 'C')
 * @return Numeric index (0, 1, 2) or -1 if invalid
 */
inline int charToIndex(char id) {
    if (id >= 'A' && id <= 'C') {
        return id - 'A';
    }
    return -1;
}

/**
 * Convert numeric index to char ID
 * @param index Numeric index (0, 1, 2)
 * @return Unit ID ('A', 'B', 'C') or '?' if invalid
 */
inline char indexToChar(int index) {
    if (index >= 0 && index < NUM_PEERS) {
        return 'A' + index;
    }
    return '?';
}

// =============================================================================
// PERFORMANCE UTILITIES
// =============================================================================

/**
 * Simple performance timer
 * Usage:
 *   PerfTimer timer;
 *   timer.start();
 *   // ... code to measure ...
 *   unsigned long elapsed = timer.stop();
 */
class PerfTimer {
private:
    unsigned long startTime;
    unsigned long endTime;
    
public:
    PerfTimer() : startTime(0), endTime(0) {}
    
    void start() {
        startTime = micros();
    }
    
    unsigned long stop() {
        endTime = micros();
        return elapsed();
    }
    
    unsigned long elapsed() const {
        if (endTime >= startTime) {
            return endTime - startTime;
        }
        return 0;
    }
    
    float elapsedMs() const {
        return elapsed() / 1000.0f;
    }
};

/**
 * Running average calculator
 * Maintains a moving average over N samples
 */
template<int N>
class RunningAverage {
private:
    float samples[N];
    int index;
    int count;
    float sum;
    
public:
    RunningAverage() : index(0), count(0), sum(0.0f) {
        for (int i = 0; i < N; i++) {
            samples[i] = 0.0f;
        }
    }
    
    void add(float value) {
        sum -= samples[index];
        samples[index] = value;
        sum += value;
        index = (index + 1) % N;
        if (count < N) count++;
    }
    
    float get() const {
        if (count == 0) return 0.0f;
        return sum / count;
    }
    
    void reset() {
        index = 0;
        count = 0;
        sum = 0.0f;
        for (int i = 0; i < N; i++) {
            samples[i] = 0.0f;
        }
    }
};

// =============================================================================
// SYSTEM UTILITIES
// =============================================================================

/**
 * Print system information to serial
 */
inline void printSystemInfo() {
    LOG_INFO("========================================");
    LOG_INFO("  UWB Proximity Chat Unit");
    LOG_INFO("========================================");
    LOG_INFO("Unit ID:      %c", UNIT_ID);
    LOG_INFO("Chip:         ESP32");
    LOG_INFO("CPU Freq:     %d MHz", ESP.getCpuFreqMHz());
    LOG_INFO("Flash:        %d KB", ESP.getFlashChipSize() / 1024);
    LOG_INFO("Free Heap:    %lu KB", getFreeHeapKB());
    LOG_INFO("Simulation:   %s", ENABLE_SIMULATION ? "YES" : "NO");
    LOG_INFO("Log Level:    %d", LOG_LEVEL);
    LOG_INFO("UWB Channel:  %d", UWB_CHANNEL);
    LOG_INFO("Hub:          %s:%d", HUB_UDP_IP, HUB_UDP_PORT);
    LOG_INFO("========================================");
}

/**
 * Blink LED pattern
 * @param times Number of blinks
 * @param onMs LED on duration (ms)
 * @param offMs LED off duration (ms)
 */
inline void blinkLED(int times, int onMs = 100, int offMs = 100) {
#if HEARTBEAT_ENABLE
    for (int i = 0; i < times; i++) {
        digitalWrite(HEARTBEAT_LED_PIN, HIGH);
        delay(onMs);
        digitalWrite(HEARTBEAT_LED_PIN, LOW);
        if (i < times - 1) {
            delay(offMs);
        }
    }
#endif
}

#endif // UTILS_H

