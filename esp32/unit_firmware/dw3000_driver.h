/**
 * dw3000_driver.h - Hardware Abstraction Layer for DW3000 UWB Module
 * 
 * Provides clean interface to DW3000 ranging functionality.
 * Implements Double-Sided Two-Way Ranging (DS-TWR) protocol.
 * 
 * This abstraction allows the code to compile even without actual
 * DW3000 libraries installed (for testing/simulation).
 * 
 * IMPORTANT: In production, replace stub functions with actual
 * DW3000 library calls (e.g., from Qorvo's DW3000 Arduino library).
 */

#ifndef DW3000_DRIVER_H
#define DW3000_DRIVER_H

#include <Arduino.h>
#include <SPI.h>
#include "config.h"
#include "utils.h"

// =============================================================================
// DW3000 REGISTER DEFINITIONS (subset for POC)
// =============================================================================

// In production, include actual DW3000 headers:
// #include <DW3000.h>
// #include <DW3000Ranging.h>

// For POC, we define essential constants
#define DW3000_DEV_ID           0x00    // Device ID register
#define DW3000_SYS_CFG          0x04    // System configuration
#define DW3000_TX_FCTRL         0x08    // TX frame control
#define DW3000_DX_TIME          0x0A    // Delayed TX time
#define DW3000_RX_FWTO          0x0C    // RX frame wait timeout
#define DW3000_SYS_CTRL         0x0D    // System control
#define DW3000_SYS_STATUS       0x0F    // System status
#define DW3000_RX_TIME          0x15    // RX timestamp
#define DW3000_TX_TIME          0x17    // TX timestamp

// =============================================================================
// DATA STRUCTURES
// =============================================================================

/**
 * Structure to hold ranging result
 */
struct RangingResult {
    bool success;           // true if ranging succeeded
    float distance;         // Distance in meters
    float quality;          // Quality metric (0.0-1.0)
    uint32_t timestamp;     // Timestamp of measurement
    char peerID;            // Peer unit ID
};

/**
 * Structure to hold DS-TWR timestamps
 * DS-TWR requires 6 timestamps for accurate ranging
 */
struct TWRTimestamps {
    uint64_t poll_tx;       // Initiator sends poll
    uint64_t poll_rx;       // Responder receives poll
    uint64_t resp_tx;       // Responder sends response
    uint64_t resp_rx;       // Initiator receives response
    uint64_t final_tx;      // Initiator sends final
    uint64_t final_rx;      // Responder receives final
};

/**
 * UWB module state
 */
enum UWBState {
    UWB_STATE_IDLE,
    UWB_STATE_INIT,
    UWB_STATE_READY,
    UWB_STATE_RANGING,
    UWB_STATE_ERROR
};

// =============================================================================
// GLOBAL STATE
// =============================================================================

static UWBState g_uwbState = UWB_STATE_IDLE;
static RangingResult g_lastResult;
static SPIClass* g_spi = nullptr;

// =============================================================================
// LOW-LEVEL SPI FUNCTIONS (Hardware Abstraction)
// =============================================================================

/**
 * Initialize SPI interface for DW3000
 */
void uwb_spi_init() {
    // Initialize SPI pins
    pinMode(DW3000_CS_PIN, OUTPUT);
    digitalWrite(DW3000_CS_PIN, HIGH);  // Deselect
    
    pinMode(DW3000_RST_PIN, OUTPUT);
    digitalWrite(DW3000_RST_PIN, HIGH); // Not in reset
    
    pinMode(DW3000_IRQ_PIN, INPUT);
    
    // Initialize SPI bus (VSPI on ESP32)
    g_spi = new SPIClass(VSPI);
    g_spi->begin(DW3000_SCK_PIN, DW3000_MISO_PIN, DW3000_MOSI_PIN, DW3000_CS_PIN);
    
    LOG_DEBUG("SPI initialized: SCK=%d, MISO=%d, MOSI=%d, CS=%d", 
              DW3000_SCK_PIN, DW3000_MISO_PIN, DW3000_MOSI_PIN, DW3000_CS_PIN);
}

/**
 * Hardware reset of DW3000
 */
void uwb_hard_reset() {
    LOG_DEBUG("DW3000 hardware reset");
    digitalWrite(DW3000_RST_PIN, LOW);
    delay(10);
    digitalWrite(DW3000_RST_PIN, HIGH);
    delay(10);
}

/**
 * Read from DW3000 register (stub for POC)
 * @param reg Register address
 * @return Register value
 */
uint32_t uwb_read_reg(uint8_t reg) {
    // STUB: In production, implement actual SPI read
    // Example:
    // digitalWrite(DW3000_CS_PIN, LOW);
    // g_spi->transfer(reg);
    // uint32_t value = g_spi->transfer32(0);
    // digitalWrite(DW3000_CS_PIN, HIGH);
    // return value;
    
#if ENABLE_SIMULATION
    // Return dummy values for simulation
    if (reg == DW3000_DEV_ID) {
        return 0xDECA0302;  // DW3000 device ID
    }
#endif
    return 0;
}

/**
 * Write to DW3000 register (stub for POC)
 * @param reg Register address
 * @param value Value to write
 */
void uwb_write_reg(uint8_t reg, uint32_t value) {
    // STUB: In production, implement actual SPI write
    // Example:
    // digitalWrite(DW3000_CS_PIN, LOW);
    // g_spi->transfer(reg | 0x80);  // Write bit
    // g_spi->transfer32(value);
    // digitalWrite(DW3000_CS_PIN, HIGH);
    
    LOG_TRACE("Write reg 0x%02X = 0x%08X", reg, value);
}

// =============================================================================
// DW3000 INITIALIZATION
// =============================================================================

/**
 * Configure DW3000 radio parameters
 */
bool uwb_configure_radio() {
    LOG_DEBUG("Configuring UWB radio: CH=%d, PRF=%d, RATE=%d", 
              UWB_CHANNEL, UWB_PRF, UWB_DATA_RATE);
    
#if !ENABLE_SIMULATION
    // STUB: In production, use actual DW3000 library
    // Example:
    // DW3000.setChannel(UWB_CHANNEL);
    // DW3000.setPreambleLength(UWB_PREAMBLE_LEN);
    // DW3000.setDataRate(UWB_DATA_RATE);
    // DW3000.setPRF(UWB_PRF);
    // DW3000.setPreambleCode(UWB_PCODE);
    // DW3000.setAntennaDelay(ANTENNA_DELAY_TX, ANTENNA_DELAY_RX);
    
    // For POC without hardware, just log configuration
    LOG_WARN("Running without DW3000 library - using stubs");
#else
    LOG_INFO("Simulation mode - no hardware configuration needed");
#endif
    
    return true;
}

/**
 * Initialize DW3000 module
 * @return true if initialization succeeded
 */
bool uwb_init() {
    if (g_uwbState != UWB_STATE_IDLE) {
        LOG_WARN("UWB already initialized");
        return true;
    }
    
    LOG_INFO("Initializing DW3000...");
    g_uwbState = UWB_STATE_INIT;
    
    // Initialize SPI
    uwb_spi_init();
    
    // Hardware reset
    uwb_hard_reset();
    
#if !ENABLE_SIMULATION
    // Check device ID
    uint32_t devID = uwb_read_reg(DW3000_DEV_ID);
    LOG_DEBUG("Device ID: 0x%08X", devID);
    
    // Expected DW3000 ID: 0xDECA0302
    if ((devID & 0xFFFFFF00) != 0xDECA0300) {
        LOG_ERROR("Invalid device ID: 0x%08X (expected 0xDECA03xx)", devID);
        // In simulation mode or without hardware, continue anyway
        if (!ENABLE_SIMULATION) {
            g_uwbState = UWB_STATE_ERROR;
            return false;
        }
    }
#endif
    
    // Configure radio parameters
    if (!uwb_configure_radio()) {
        LOG_ERROR("Failed to configure radio");
        g_uwbState = UWB_STATE_ERROR;
        return false;
    }
    
    g_uwbState = UWB_STATE_READY;
    LOG_INFO("DW3000 initialized successfully");
    
    return true;
}

/**
 * Check if UWB module is ready
 * @return true if ready for ranging
 */
bool uwb_is_ready() {
    return g_uwbState == UWB_STATE_READY;
}

// =============================================================================
// DS-TWR RANGING IMPLEMENTATION
// =============================================================================

/**
 * Calculate time-of-flight from DS-TWR timestamps
 * @param ts TWR timestamps structure
 * @return Time of flight in DW3000 time units
 */
uint64_t twr_calculate_tof(const TWRTimestamps& ts) {
    // DS-TWR algorithm:
    // tof = ((resp_rx - poll_tx) * (final_rx - resp_tx) - (resp_tx - poll_rx) * (final_tx - resp_rx))
    //       / ((resp_rx - poll_tx) + (final_rx - resp_tx) + (resp_tx - poll_rx) + (final_tx - resp_rx))
    
    int64_t Ra = ts.resp_rx - ts.poll_tx;   // Round trip 1 at initiator
    int64_t Rb = ts.final_rx - ts.resp_tx;  // Round trip 2 at responder
    int64_t Da = ts.resp_tx - ts.poll_rx;   // Reply time at responder
    int64_t Db = ts.final_tx - ts.resp_rx;  // Reply time at initiator
    
    int64_t tof_num = Ra * Rb - Da * Db;
    int64_t tof_den = Ra + Rb + Da + Db;
    
    if (tof_den == 0) {
        LOG_ERROR("Division by zero in TOF calculation");
        return 0;
    }
    
    uint64_t tof = (uint64_t)(tof_num / tof_den);
    
    LOG_TRACE("TOF calculation: Ra=%lld, Rb=%lld, Da=%lld, Db=%lld, TOF=%llu",
              Ra, Rb, Da, Db, tof);
    
    return tof;
}

/**
 * Perform DS-TWR ranging with a peer
 * @param peerID Peer unit ID to range with
 * @param result Output structure for ranging result
 * @return true if ranging succeeded
 */
bool uwb_range(char peerID, RangingResult& result) {
    if (g_uwbState != UWB_STATE_READY) {
        LOG_ERROR("UWB not ready for ranging");
        return false;
    }
    
    LOG_DEBUG("Ranging with peer %c...", peerID);
    g_uwbState = UWB_STATE_RANGING;
    
#if ENABLE_SIMULATION
    // Simulation mode: generate synthetic data
    result.success = true;
    result.distance = generateSimulatedDistance(peerID);
    result.quality = SIM_QUALITY;
    result.timestamp = getTimestamp();
    result.peerID = peerID;
    
    LOG_DEBUG("Simulated range to %c: %.2f m (Q=%.2f)", 
              peerID, result.distance, result.quality);
    
#else
    // Real ranging implementation (STUB for POC)
    // In production, implement actual DS-TWR protocol:
    
    TWRTimestamps ts;
    memset(&ts, 0, sizeof(ts));
    
    // STUB: These would be actual UWB transactions
    // Step 1: Send POLL message
    // ts.poll_tx = DW3000.getTxTimestamp();
    
    // Step 2: Wait for RESP message
    // if (!DW3000.waitForResponse(RANGING_TIMEOUT_MS)) {
    //     LOG_WARN("Timeout waiting for RESP from %c", peerID);
    //     result.success = false;
    //     g_uwbState = UWB_STATE_READY;
    //     return false;
    // }
    // ts.resp_rx = DW3000.getRxTimestamp();
    // Parse RESP to get ts.poll_rx and ts.resp_tx from peer
    
    // Step 3: Send FINAL message with our timestamps
    // ts.final_tx = DW3000.getTxTimestamp();
    
    // Step 4: Wait for ACK with peer's final_rx timestamp
    // if (!DW3000.waitForResponse(RANGING_TIMEOUT_MS)) {
    //     LOG_WARN("Timeout waiting for ACK from %c", peerID);
    //     result.success = false;
    //     g_uwbState = UWB_STATE_READY;
    //     return false;
    // }
    // Parse ACK to get ts.final_rx from peer
    
    // For POC without library, generate dummy timestamps
    ts.poll_tx = 1000000;
    ts.poll_rx = 1001000;
    ts.resp_tx = 1002000;
    ts.resp_rx = 1003000;
    ts.final_tx = 1004000;
    ts.final_rx = 1005000;
    
    // Calculate time-of-flight
    uint64_t tof = twr_calculate_tof(ts);
    
    // Convert to distance
    float distance = tofToDistance(tof);
    
    // Calculate quality metric (based on signal strength, consistency, etc.)
    // STUB: In production, derive from actual DW3000 diagnostics
    float quality = 0.85f;  // Dummy value
    
    result.success = true;
    result.distance = distance;
    result.quality = quality;
    result.timestamp = getTimestamp();
    result.peerID = peerID;
    
    LOG_DEBUG("Range to %c: %.2f m (Q=%.2f)", 
              peerID, result.distance, result.quality);
#endif
    
    g_lastResult = result;
    g_uwbState = UWB_STATE_READY;
    
    return result.success;
}

/**
 * Get the last ranging result
 * @return Last ranging result structure
 */
RangingResult uwb_get_last_result() {
    return g_lastResult;
}

/**
 * Get quality metric of last ranging
 * @return Quality value (0.0-1.0)
 */
float uwb_get_last_quality() {
    return g_lastResult.quality;
}

// =============================================================================
// DIAGNOSTIC FUNCTIONS
// =============================================================================

/**
 * Print DW3000 status information
 */
void uwb_print_status() {
    const char* stateStr[] = {"IDLE", "INIT", "READY", "RANGING", "ERROR"};
    LOG_INFO("UWB Status: %s", stateStr[g_uwbState]);
    
    if (g_uwbState == UWB_STATE_READY || g_uwbState == UWB_STATE_RANGING) {
        LOG_INFO("  Channel: %d, PRF: %d, Data Rate: %d", 
                 UWB_CHANNEL, UWB_PRF, UWB_DATA_RATE);
        LOG_INFO("  Antenna Delay: TX=%d, RX=%d", 
                 ANTENNA_DELAY_TX, ANTENNA_DELAY_RX);
    }
    
    if (g_lastResult.success) {
        LOG_INFO("  Last Range: %c @ %.2f m (Q=%.2f)", 
                 g_lastResult.peerID, g_lastResult.distance, g_lastResult.quality);
    }
}

/**
 * Reset UWB module (for error recovery)
 */
void uwb_reset() {
    LOG_WARN("Resetting UWB module...");
    g_uwbState = UWB_STATE_IDLE;
    uwb_init();
}

// =============================================================================
// INTERRUPT HANDLERS (for production use)
// =============================================================================

/**
 * IRQ handler for DW3000 interrupts
 * In production, attach this to DW3000_IRQ_PIN
 */
void IRAM_ATTR uwb_irq_handler() {
    // STUB: Handle DW3000 interrupts
    // Example:
    // uint32_t status = uwb_read_reg(DW3000_SYS_STATUS);
    // if (status & TX_DONE_MASK) {
    //     // TX complete
    // }
    // if (status & RX_DONE_MASK) {
    //     // RX complete
    // }
}

/**
 * Attach IRQ handler
 */
void uwb_attach_irq() {
    attachInterrupt(digitalPinToInterrupt(DW3000_IRQ_PIN), uwb_irq_handler, RISING);
    LOG_DEBUG("IRQ handler attached to pin %d", DW3000_IRQ_PIN);
}

#endif // DW3000_DRIVER_H

