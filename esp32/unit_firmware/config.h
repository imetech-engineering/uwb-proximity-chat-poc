/**
 * config.h - Centralized Configuration for UWB Proximity Chat Unit
 * 
 * ALL CONFIGURABLE PARAMETERS FOR ESP32 FIRMWARE
 * Edit this file to customize behavior for your deployment.
 * 
 * This file is the SINGLE source of truth for all settings.
 * No other files should contain hardcoded configuration values.
 */

#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// NETWORK CONFIGURATION
// =============================================================================

// Wi-Fi Credentials (2.4GHz only - ESP32 doesn't support 5GHz)
#define WIFI_SSID        "YourWiFiNetwork"     // Change to your network SSID 
#define WIFI_PASS        "YourWiFiPassword"    // Change to your network password 

// Hub (Raspberry Pi) Configuration
#define HUB_UDP_IP       "192.168.1.100"       // IP address of Raspberry Pi hub
#define HUB_UDP_PORT     9999                  // UDP port hub is listening on

// Network Timeouts and Retry Settings
#define WIFI_CONNECT_TIMEOUT_MS     10000      // Max time to wait for Wi-Fi connection
#define WIFI_RECONNECT_INTERVAL_MS  5000       // Time between reconnection attempts
#define UDP_RETRY_COUNT             3          // Number of UDP send retries
#define UDP_RETRY_DELAY_MS          100        // Delay between UDP retries

// =============================================================================
// UNIT IDENTIFICATION
// =============================================================================

// Unit ID: Must be unique for each device ('A', 'B', or 'C')
// *** CHANGE THIS FOR EACH UNIT YOU FLASH ***
#define UNIT_ID          'A'                   // Options: 'A', 'B', 'C'

// Numeric ID (derived from UNIT_ID, used internally)
#define UNIT_NUMERIC_ID  (UNIT_ID - 'A')       // A=0, B=1, C=2

// Peer IDs (other units to range with)
static const char PEER_IDS[] = {'A', 'B', 'C'}; // All unit IDs in system
static const int NUM_PEERS = 3;                  // Total number of units

// =============================================================================
// UWB (DW3000) CONFIGURATION
// =============================================================================

// SPI Pin Assignments (ESP32 VSPI)
#define DW3000_CS_PIN    5                     // Chip Select
#define DW3000_IRQ_PIN   17                    // Interrupt Request
#define DW3000_RST_PIN   27                    // Reset
#define DW3000_SCK_PIN   18                    // SPI Clock (VSPI default)
#define DW3000_MISO_PIN  19                    // SPI MISO (VSPI default)
#define DW3000_MOSI_PIN  23                    // SPI MOSI (VSPI default)

// SPI Speed
#define DW3000_SPI_SPEED 8000000               // 8 MHz (safe for breadboard)

// UWB Radio Configuration
#define UWB_CHANNEL      5                     // Channel: 5 (6.5GHz) or 9 (8GHz)
                                               // Channel 5 recommended for better range
#define UWB_PREAMBLE_LEN 128                   // Preamble length: 64, 128, 256, 512, 1024
                                               // Longer = better sensitivity, slower
#define UWB_DATA_RATE    1                     // 0=110kbps, 1=850kbps, 2=6.8Mbps
                                               // 850kbps recommended for balance
#define UWB_PRF          2                     // Pulse Repetition Freq: 1=16MHz, 2=64MHz
                                               // PRF64 recommended for accuracy
#define UWB_PCODE        9                     // Preamble code (channel dependent)
                                               // Ch5: codes 3,4,9,10,11,12
                                               // Ch9: codes 9,10,11,12

// STS (Scrambled Timestamp Sequence) - Enhanced security and precision
#define UWB_STS_ENABLE   false                 // true = enable STS (requires config)
                                               // false = disable (simpler, for POC)

// TX Power (in 0.5dB steps, range depends on regulatory domain)
#define UWB_TX_POWER     0x1F1F1F1F            // Full power (example)
                                               // Check local regulations!

// Ranging Configuration
#define RANGING_INTERVAL_MS      500           // Time between ranging attempts (ms)
                                               // 500ms = 2Hz update rate
#define RANGING_TIMEOUT_MS       100           // Max time to wait for range response
#define RANGING_MAX_RETRIES      3             // Retries before marking peer as lost

// =============================================================================
// CALIBRATION CONSTANTS
// =============================================================================

// Antenna Delay (in DW3000 time units, ~15.65ps each)
// These values compensate for PCB trace delays and antenna characteristics
// Typical range: 16300-16600 for DW3000
// *** CALIBRATE THESE VALUES USING CALIBRATION.md PROCEDURE ***
#define ANTENNA_DELAY_TX         16450         // TX antenna delay
#define ANTENNA_DELAY_RX         16450         // RX antenna delay

// Distance Offset (meters)
// Fine-tuning to correct systematic ranging errors
// Positive value = measured distances are too short (add offset)
// Negative value = measured distances are too long (subtract offset)
#define DIST_OFFSET_M            0.00          // Distance offset in meters

// Quality Threshold (0.0 - 1.0)
// Discard measurements below this quality to filter noise
#define QUALITY_THRESHOLD        0.5           // Minimum acceptable quality

// =============================================================================
// SCHEDULING & COORDINATION
// =============================================================================

// Time Slot Configuration (to avoid UWB collisions)
// Each unit gets a time slot to initiate ranging
#define TIME_SLOT_DURATION_MS    200           // Duration of each unit's slot
#define CYCLE_DURATION_MS        (NUM_PEERS * TIME_SLOT_DURATION_MS)  // Full cycle

// Slot assignment (based on UNIT_ID)
// Unit A: slot 0 (0-200ms)
// Unit B: slot 1 (200-400ms)  
// Unit C: slot 2 (400-600ms)
#define MY_SLOT_OFFSET_MS        (UNIT_NUMERIC_ID * TIME_SLOT_DURATION_MS)

// =============================================================================
// SIMULATION MODE
// =============================================================================

// Enable simulation mode to test system without DW3000 hardware
// When true, firmware generates synthetic distance measurements
// Useful for:
// - Testing network communication
// - Developing hub software
// - Demonstrating UI without hardware
#define ENABLE_SIMULATION        false         // true = simulate, false = use real UWB

// Simulation Parameters (only used if ENABLE_SIMULATION == true)
#define SIM_BASE_DISTANCE_M      2.0           // Base distance (meters)
#define SIM_AMPLITUDE_M          1.0           // Oscillation amplitude
#define SIM_PERIOD_MS            10000         // Oscillation period (10 seconds)
#define SIM_QUALITY              0.95          // Simulated quality factor

// =============================================================================
// DIAGNOSTICS & LOGGING
// =============================================================================

// Serial Configuration
#define SERIAL_BAUD_RATE         115200        // Serial monitor baud rate

// Log Levels (lower number = more critical)
#define LOG_LEVEL_ERROR          0             // Critical errors only
#define LOG_LEVEL_WARN           1             // Warnings + errors
#define LOG_LEVEL_INFO           2             // Info + warnings + errors
#define LOG_LEVEL_DEBUG          3             // Debug + all above
#define LOG_LEVEL_TRACE          4             // Trace (very verbose) + all above

// Active Log Level (set this to control verbosity)
#define LOG_LEVEL                LOG_LEVEL_INFO   // Change to DEBUG or TRACE for troubleshooting

// Heartbeat (status LED and periodic logging)
#define HEARTBEAT_ENABLE         true          // Enable status heartbeat
#define HEARTBEAT_INTERVAL_MS    2000          // Heartbeat interval
#define HEARTBEAT_LED_PIN        2             // Built-in LED (GPIO 2 on most ESP32)

// Performance Monitoring
#define ENABLE_PERF_STATS        true          // Track and log performance metrics
#define PERF_STATS_INTERVAL_MS   10000         // How often to print stats

// =============================================================================
// WATCHDOG & RELIABILITY
// =============================================================================

// Watchdog Timer (auto-reset if firmware hangs)
#define ENABLE_WATCHDOG          true          // Enable watchdog timer
#define WATCHDOG_TIMEOUT_SEC     10            // Watchdog timeout (seconds)

// Memory Monitoring
#define ENABLE_MEM_CHECK         true          // Monitor free heap
#define MEM_CHECK_INTERVAL_MS    30000         // How often to check memory
#define MEM_WARNING_THRESHOLD_KB 20            // Warn if free heap below this (KB)

// =============================================================================
// ADVANCED SETTINGS (usually don't need to change)
// =============================================================================

// Buffer Sizes
#define UDP_PACKET_MAX_SIZE      512           // Maximum UDP packet size
#define SERIAL_BUFFER_SIZE       256           // Serial output buffer

// JSON Message Settings
#define JSON_MAX_SIZE            256           // Max JSON message size
#define JSON_PRECISION           2             // Decimal places for float values

// Task Priorities (FreeRTOS)
#define TASK_PRIORITY_UWB        2             // UWB ranging task priority
#define TASK_PRIORITY_NETWORK    1             // Network task priority
#define TASK_PRIORITY_HEARTBEAT  0             // Heartbeat task priority (lowest)

// Stack Sizes (bytes)
#define STACK_SIZE_UWB           4096          // Stack for UWB task
#define STACK_SIZE_NETWORK       4096          // Stack for network task
#define STACK_SIZE_HEARTBEAT     2048          // Stack for heartbeat task

// =============================================================================
// COMPILE-TIME VALIDATION
// =============================================================================

// Ensure UNIT_ID is valid
#if (UNIT_ID != 'A' && UNIT_ID != 'B' && UNIT_ID != 'C')
  #error "UNIT_ID must be 'A', 'B', or 'C'"
#endif

// Ensure channel is valid
#if (UWB_CHANNEL != 5 && UWB_CHANNEL != 9)
  #error "UWB_CHANNEL must be 5 or 9"
#endif

// Ensure data rate is valid
#if (UWB_DATA_RATE < 0 || UWB_DATA_RATE > 2)
  #error "UWB_DATA_RATE must be 0, 1, or 2"
#endif

// =============================================================================
// HELPER MACROS
// =============================================================================

// Logging macros (only compile if log level is enabled)
#if LOG_LEVEL >= LOG_LEVEL_ERROR
  #define LOG_ERROR(fmt, ...) Serial.printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#else
  #define LOG_ERROR(fmt, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARN
  #define LOG_WARN(fmt, ...) Serial.printf("[WARN]  " fmt "\n", ##__VA_ARGS__)
#else
  #define LOG_WARN(fmt, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
  #define LOG_INFO(fmt, ...) Serial.printf("[INFO]  " fmt "\n", ##__VA_ARGS__)
#else
  #define LOG_INFO(fmt, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
  #define LOG_DEBUG(fmt, ...) Serial.printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
  #define LOG_DEBUG(fmt, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_TRACE
  #define LOG_TRACE(fmt, ...) Serial.printf("[TRACE] " fmt "\n", ##__VA_ARGS__)
#else
  #define LOG_TRACE(fmt, ...)
#endif

#endif // CONFIG_H

