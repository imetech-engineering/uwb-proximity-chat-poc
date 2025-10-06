/**
 * dw3000_driver.h - DW3000 UWB Driver for ESP32
 * 
 * Complete driver implementation for DecaWave DW3000 UWB transceiver
 * Implements Double-Sided Two-Way Ranging (DS-TWR) protocol
 * 
 * Reference: Makerfabs ESP32 UWB DW3000
 * Hardware: ESP32 + DW3000 module
 * 
 * This is a simplified but functional implementation for Proof of Concept.
 * For production, consider using official Qorvo DW3000 libraries.
 */

#ifndef DW3000_DRIVER_H
#define DW3000_DRIVER_H

#include <Arduino.h>
#include <SPI.h>
#include "config.h"
#include "utils.h"

// =============================================================================
// DW3000 REGISTER MAP
// =============================================================================

// Core registers
#define DW3000_REG_DEV_ID           0x00    // Device ID (read-only)
#define DW3000_REG_SYS_CFG          0x04    // System configuration
#define DW3000_REG_SYS_STATUS       0x44    // System event status
#define DW3000_REG_SYS_CTRL         0x0D    // System control
#define DW3000_REG_TX_FCTRL         0x08    // TX frame control
#define DW3000_REG_TX_BUFFER        0x14    // TX data buffer
#define DW3000_REG_RX_FINFO         0x10    // RX frame info
#define DW3000_REG_RX_BUFFER        0x11    // RX data buffer
#define DW3000_REG_RX_TIME          0x15    // RX timestamp
#define DW3000_REG_TX_TIME          0x17    // TX timestamp
#define DW3000_REG_CHAN_CTRL        0x1F    // Channel control
#define DW3000_REG_SYS_ENABLE       0x06    // System event mask
#define DW3000_REG_RX_FWTO          0x0C    // RX frame wait timeout

// System control bits
#define DW3000_TXSTRT               0x00000001  // Start transmission
#define DW3000_RXENAB               0x00000100  // Enable receiver
#define DW3000_SFCST                0x00010000  // Suppress auto FCS

// System status bits
#define DW3000_TXFRS                0x00000080  // TX frame sent
#define DW3000_RXFCG                0x00004000  // RX frame CRC good
#define DW3000_RXFCE                0x00008000  // RX frame CRC error
#define DW3000_RXRFTO               0x00010000  // RX frame wait timeout
#define DW3000_RXPTO                0x00200000  // Preamble detect timeout
#define DW3000_RXFR                 0x00002000  // RX frame ready

// Device ID expected value
#define DW3000_DEVICE_ID            0xDECA0302

// Message types for ranging protocol
#define MSG_TYPE_POLL               0x61
#define MSG_TYPE_RESP               0x50
#define MSG_TYPE_FINAL              0x69
#define MSG_TYPE_REPORT             0x72

// =============================================================================
// DATA STRUCTURES
// =============================================================================

/**
 * Ranging result structure
 */
struct RangingResult {
    bool success;
    float distance;
    float quality;
    uint32_t timestamp;
    char peerID;
};

/**
 * UWB message frame structure
 */
struct __attribute__((packed)) UWBFrame {
    uint8_t frameCtrl[2];       // Frame control
    uint8_t sequence;           // Sequence number
    uint8_t panID[2];           // PAN ID
    uint8_t destAddr[2];        // Destination address
    uint8_t sourceAddr[2];      // Source address
    uint8_t msgType;            // Message type
    uint8_t payload[32];        // Payload data
};

/**
 * DS-TWR timestamp payload
 */
struct __attribute__((packed)) TWRPayload {
    uint64_t pollTxTime;
    uint64_t pollRxTime;
    uint64_t respTxTime;
    uint64_t respRxTime;
    uint64_t finalTxTime;
    uint64_t finalRxTime;
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
static uint8_t g_sequenceNum = 0;
static const uint16_t PAN_ID = 0xDECA;

// =============================================================================
// LOW-LEVEL SPI FUNCTIONS
// =============================================================================

/**
 * Write to DW3000 register
 */
inline void dw3000_write_reg(uint8_t reg, uint8_t* data, uint16_t len) {
#if !ENABLE_SIMULATION
    if (!g_spi) return;  // SPI not initialized
    
    digitalWrite(DW3000_CS_PIN, LOW);
    delayMicroseconds(1);
    
    g_spi->transfer(0x80 | reg);  // Write flag
    g_spi->transfer(data, len);
    
    digitalWrite(DW3000_CS_PIN, HIGH);
    delayMicroseconds(5);
#endif
}

/**
 * Write single byte to register
 */
inline void dw3000_write_reg8(uint8_t reg, uint8_t value) {
    dw3000_write_reg(reg, &value, 1);
}

/**
 * Write 32-bit value to register
 */
inline void dw3000_write_reg32(uint8_t reg, uint32_t value) {
    uint8_t data[4];
    memcpy(data, &value, 4);
    dw3000_write_reg(reg, data, 4);
}

/**
 * Read from DW3000 register
 */
inline void dw3000_read_reg(uint8_t reg, uint8_t* data, uint16_t len) {
#if !ENABLE_SIMULATION
    if (!g_spi) {
        memset(data, 0, len);  // SPI not initialized
        return;
    }
    
    digitalWrite(DW3000_CS_PIN, LOW);
    delayMicroseconds(1);
    
    g_spi->transfer(reg);  // Read (no write flag)
    for (uint16_t i = 0; i < len; i++) {
        data[i] = g_spi->transfer(0x00);
    }
    
    digitalWrite(DW3000_CS_PIN, HIGH);
    delayMicroseconds(5);
#else
    // Simulation: return dummy data
    if (reg == DW3000_REG_DEV_ID && len == 4) {
        memcpy(data, &(uint32_t){DW3000_DEVICE_ID}, 4);
    } else {
        memset(data, 0, len);
    }
#endif
}

/**
 * Read 32-bit value from register
 */
inline uint32_t dw3000_read_reg32(uint8_t reg) {
    uint8_t data[4];
    dw3000_read_reg(reg, data, 4);
    uint32_t value;
    memcpy(&value, data, 4);
    return value;
}

/**
 * Read 64-bit timestamp
 */
inline uint64_t dw3000_read_reg64(uint8_t reg) {
    uint8_t data[8];
    dw3000_read_reg(reg, data, 8);
    uint64_t value = 0;
    memcpy(&value, data, 5);  // DW3000 uses 40-bit timestamps
    return value;
}

// =============================================================================
// DW3000 CORE FUNCTIONS
// =============================================================================

/**
 * Hardware reset
 */
inline void dw3000_reset() {
#if !ENABLE_SIMULATION
    LOG_DEBUG("DW3000 hardware reset");
    digitalWrite(DW3000_RST_PIN, LOW);
    delay(10);
    digitalWrite(DW3000_RST_PIN, HIGH);
    delay(10);
#endif
}

/**
 * Initialize SPI interface
 */
inline bool dw3000_spi_init() {
#if !ENABLE_SIMULATION
    pinMode(DW3000_CS_PIN, OUTPUT);
    pinMode(DW3000_RST_PIN, OUTPUT);
    pinMode(DW3000_IRQ_PIN, INPUT);
    
    digitalWrite(DW3000_CS_PIN, HIGH);
    digitalWrite(DW3000_RST_PIN, HIGH);
    
    g_spi = new SPIClass(FSPI);
    g_spi->begin(DW3000_SCK_PIN, DW3000_MISO_PIN, DW3000_MOSI_PIN, DW3000_CS_PIN);
    g_spi->setFrequency(DW3000_SPI_SPEED);
    g_spi->setDataMode(SPI_MODE0);
    g_spi->setBitOrder(MSBFIRST);
    
    LOG_DEBUG("SPI initialized at %d Hz", DW3000_SPI_SPEED);
    return true;
#else
    LOG_INFO("Simulation mode: Skipping SPI init");
    return true;
#endif
}

/**
 * Configure DW3000 for ranging
 */
inline bool dw3000_configure() {
#if ENABLE_SIMULATION
    LOG_INFO("Simulation mode: Skipping radio config");
    return true;
#else
    LOG_DEBUG("Configuring DW3000 radio");
    
    // Configure channel (5 or 9)
    uint32_t chanCtrl = 0;
    if (UWB_CHANNEL == 5) {
        chanCtrl = 0x000D0500;  // Channel 5 (6.5 GHz)
    } else {
        chanCtrl = 0x000D0900;  // Channel 9 (8 GHz)
    }
    dw3000_write_reg32(DW3000_REG_CHAN_CTRL, chanCtrl);
    
    // Configure system
    uint32_t sysCfg = 0x00000000;
    dw3000_write_reg32(DW3000_REG_SYS_CFG, sysCfg);
    
    // Enable events: TX done, RX done, RX timeout
    uint32_t sysEnable = DW3000_TXFRS | DW3000_RXFCG | DW3000_RXFCE | DW3000_RXRFTO;
    dw3000_write_reg32(DW3000_REG_SYS_ENABLE, sysEnable);
    
    // Set RX timeout (for waiting for responses)
    uint32_t timeout = (RANGING_TIMEOUT_MS * 1000) / 16;  // Convert to DW3000 time units
    dw3000_write_reg32(DW3000_REG_RX_FWTO, timeout);
    
    LOG_INFO("Radio configured: CH%d, PRF%d, Rate%d", UWB_CHANNEL, UWB_PRF, UWB_DATA_RATE);
    return true;
#endif
}

/**
 * Send a frame
 */
inline bool dw3000_tx(uint8_t* data, uint16_t len) {
#if ENABLE_SIMULATION
    return true;  // Simulation: always succeed
#else
    // Write data to TX buffer
    dw3000_write_reg(DW3000_REG_TX_BUFFER, data, len);
    
    // Set frame length
    uint32_t txCtrl = len | 0x00001000;  // Length + flags
    dw3000_write_reg32(DW3000_REG_TX_FCTRL, txCtrl);
    
    // Start transmission
    dw3000_write_reg32(DW3000_REG_SYS_CTRL, DW3000_TXSTRT);
    
    // Wait for TX complete (with timeout)
    unsigned long start = millis();
    while (millis() - start < 100) {
        uint32_t status = dw3000_read_reg32(DW3000_REG_SYS_STATUS);
        if (status & DW3000_TXFRS) {
            // Clear TX done flag
            dw3000_write_reg32(DW3000_REG_SYS_STATUS, DW3000_TXFRS);
            return true;
        }
        delayMicroseconds(100);
    }
    
    LOG_ERROR("TX timeout");
    return false;
#endif
}

/**
 * Receive a frame
 */
inline bool dw3000_rx(uint8_t* data, uint16_t* len, uint32_t timeout_ms) {
#if ENABLE_SIMULATION
    return false;  // Simulation: no real RX
#else
    // Enable receiver
    dw3000_write_reg32(DW3000_REG_SYS_CTRL, DW3000_RXENAB);
    
    // Wait for RX complete or timeout
    unsigned long start = millis();
    while (millis() - start < timeout_ms) {
        uint32_t status = dw3000_read_reg32(DW3000_REG_SYS_STATUS);
        
        // Check for good frame
        if (status & DW3000_RXFCG) {
            // Read frame info to get length
            uint32_t rxInfo = dw3000_read_reg32(DW3000_REG_RX_FINFO);
            *len = (rxInfo & 0x3FF) - 2;  // Remove 2-byte CRC
            
            // Read data
            dw3000_read_reg(DW3000_REG_RX_BUFFER, data, *len);
            
            // Clear RX flags
            dw3000_write_reg32(DW3000_REG_SYS_STATUS, 
                DW3000_RXFCG | DW3000_RXFR);
    
    return true;
}
        
        // Check for errors/timeout
        if (status & (DW3000_RXFCE | DW3000_RXRFTO | DW3000_RXPTO)) {
            dw3000_write_reg32(DW3000_REG_SYS_STATUS, 
                DW3000_RXFCE | DW3000_RXRFTO | DW3000_RXPTO);
            break;
        }
        
        delayMicroseconds(100);
    }
    
    return false;
#endif
}

/**
 * Get TX timestamp
 */
inline uint64_t dw3000_get_tx_timestamp() {
    return dw3000_read_reg64(DW3000_REG_TX_TIME);
}

/**
 * Get RX timestamp
 */
inline uint64_t dw3000_get_rx_timestamp() {
    return dw3000_read_reg64(DW3000_REG_RX_TIME);
}

// =============================================================================
// HIGH-LEVEL API
// =============================================================================

/**
 * Initialize DW3000
 */
inline bool uwb_init() {
    if (g_uwbState != UWB_STATE_IDLE) {
        LOG_WARN("UWB already initialized");
        return true;
    }
    
#if ENABLE_SIMULATION
    // =========================================================================
    // SIMULATION MODE - No hardware needed
    // =========================================================================
    LOG_INFO("=== UWB SIMULATION MODE ===");
    g_uwbState = UWB_STATE_INIT;
    
    LOG_INFO("  ✓ No hardware required");
    LOG_INFO("  ✓ Simulated distance: %.1f m ± %.1f m", SIM_BASE_DISTANCE_M, SIM_AMPLITUDE_M);
    LOG_INFO("  ✓ Period: %d ms", SIM_PERIOD_MS);
    LOG_INFO("  ✓ Quality: %.2f", SIM_QUALITY);
    
    g_uwbState = UWB_STATE_READY;
    LOG_INFO("=== UWB READY (SIMULATION) ===");
    LOG_INFO("");
    return true;
    
#else
    // =========================================================================
    // HARDWARE MODE - DW3000 required
    // =========================================================================
    LOG_INFO("=== Initializing DW3000 Hardware ===");
    g_uwbState = UWB_STATE_INIT;
    
    // Step 1: Initialize SPI
    LOG_INFO("Step 1/4: Initializing SPI...");
    if (!dw3000_spi_init()) {
        LOG_ERROR("✗ SPI initialization failed!");
            g_uwbState = UWB_STATE_ERROR;
            return false;
        }
    LOG_INFO("  ✓ SPI ready at %d Hz", DW3000_SPI_SPEED);
    yield();  // Feed watchdog
    
    // Step 2: Hardware reset
    LOG_INFO("Step 2/4: Resetting DW3000...");
    dw3000_reset();
    delay(100);
    LOG_INFO("  ✓ Hardware reset complete");
    yield();  // Feed watchdog
    
    // Step 3: Read and verify device ID
    LOG_INFO("Step 3/4: Reading device ID...");
    uint32_t devID = dw3000_read_reg32(DW3000_REG_DEV_ID);
    
    if (devID == 0x00000000 || devID == 0xFFFFFFFF) {
        LOG_ERROR("✗ No SPI response! Got: 0x%08X", devID);
        LOG_ERROR("");
        LOG_ERROR("HARDWARE NOT DETECTED!");
        LOG_ERROR("Either:");
        LOG_ERROR("  1. DW3000 module is not connected");
        LOG_ERROR("  2. Wiring is incorrect");
        LOG_ERROR("  3. Power supply issue");
        LOG_ERROR("");
        LOG_ERROR("Wiring should be:");
        LOG_ERROR("  ESP32 Pin → DW3000 Pin");
        LOG_ERROR("  GPIO %2d   → CS", DW3000_CS_PIN);
        LOG_ERROR("  GPIO %2d   → RST", DW3000_RST_PIN);
        LOG_ERROR("  GPIO %2d   → IRQ", DW3000_IRQ_PIN);
        LOG_ERROR("  GPIO %2d   → SCK", DW3000_SCK_PIN);
        LOG_ERROR("  GPIO %2d   → MISO", DW3000_MISO_PIN);
        LOG_ERROR("  GPIO %2d   → MOSI", DW3000_MOSI_PIN);
        LOG_ERROR("  3.3V     → VCC");
        LOG_ERROR("  GND      → GND");
        LOG_ERROR("");
        LOG_ERROR("TIP: Set ENABLE_SIMULATION=true in config.h");
        LOG_ERROR("     to test without hardware");
        LOG_ERROR("");
        g_uwbState = UWB_STATE_ERROR;
        return false;
    }
    
    if (devID != DW3000_DEVICE_ID) {
        LOG_WARN("✗ Unexpected device ID: 0x%08X", devID);
        LOG_WARN("  Expected: 0x%08X", DW3000_DEVICE_ID);
        LOG_WARN("  Continuing anyway (might be compatible chip)");
    } else {
        LOG_INFO("  ✓ Device ID verified: 0x%08X", devID);
    }
    yield();  // Feed watchdog
    
    // Step 4: Configure radio
    LOG_INFO("Step 4/4: Configuring radio...");
    if (!dw3000_configure()) {
        LOG_ERROR("✗ Radio configuration failed");
        g_uwbState = UWB_STATE_ERROR;
        return false;
    }
    LOG_INFO("  ✓ Channel: %d", UWB_CHANNEL);
    LOG_INFO("  ✓ PRF: %d MHz", UWB_PRF == 1 ? 16 : 64);
    LOG_INFO("  ✓ Data rate: %d", UWB_DATA_RATE);
    yield();  // Feed watchdog
    
    g_uwbState = UWB_STATE_READY;
    LOG_INFO("=== DW3000 READY ===");
    LOG_INFO("");
    return true;
#endif
}

/**
 * Check if ready
 */
inline bool uwb_is_ready() {
    return g_uwbState == UWB_STATE_READY;
}

/**
 * Build UWB frame
 */
inline void build_frame(UWBFrame* frame, uint8_t msgType, char destID, uint8_t* payload, uint8_t payloadLen) {
    memset(frame, 0, sizeof(UWBFrame));
    
    // Frame control: data frame, short addressing
    frame->frameCtrl[0] = 0x41;  // Data frame
    frame->frameCtrl[1] = 0x88;  // Short address mode
    
    frame->sequence = g_sequenceNum++;
    
    // PAN ID
    memcpy(frame->panID, &PAN_ID, 2);
    
    // Addresses (use unit IDs as addresses)
    uint16_t destAddr = (uint16_t)destID;
    uint16_t srcAddr = (uint16_t)UNIT_ID;
    memcpy(frame->destAddr, &destAddr, 2);
    memcpy(frame->sourceAddr, &srcAddr, 2);
    
    frame->msgType = msgType;
    
    if (payload && payloadLen > 0) {
        memcpy(frame->payload, payload, min(payloadLen, (uint8_t)32));
    }
}

/**
 * Calculate distance from timestamps
 */
inline float calculate_distance_from_twr(TWRPayload* twr) {
    // Double-sided TWR algorithm
    int64_t Ra = (int64_t)(twr->respRxTime - twr->pollTxTime);
    int64_t Rb = (int64_t)(twr->finalRxTime - twr->respTxTime);
    int64_t Da = (int64_t)(twr->respTxTime - twr->pollRxTime);
    int64_t Db = (int64_t)(twr->finalTxTime - twr->respRxTime);
    
    int64_t tof_dtu = ((Ra * Rb) - (Da * Db)) / (Ra + Rb + Da + Db);
    
    // Convert DW3000 time units to distance
    // Time unit = 1/(499.2 MHz * 128) = ~15.65 ps
    // Distance = (time * speed_of_light) / 2
    const double TIME_UNIT = 1.0 / (499.2e6 * 128.0);  // seconds
    const double SPEED_OF_LIGHT = 299792458.0;  // m/s
    
    double tof_sec = tof_dtu * TIME_UNIT;
    double distance = (tof_sec * SPEED_OF_LIGHT) / 2.0;
    
    // Apply calibration
    distance += DIST_OFFSET_M;
    
    return (float)distance;
}

/**
 * Perform ranging (initiator side)
 */
inline bool uwb_range(char peerID, RangingResult& result) {
    if (g_uwbState != UWB_STATE_READY) {
        LOG_ERROR("UWB not ready");
        return false;
    }
    
    g_uwbState = UWB_STATE_RANGING;
    
#if ENABLE_SIMULATION
    // Simulation mode
    result.success = true;
    result.distance = generateSimulatedDistance(peerID);
    result.quality = SIM_QUALITY;
    result.timestamp = getTimestamp();
    result.peerID = peerID;
    
    LOG_DEBUG("Simulated range to %c: %.2f m", peerID, result.distance);
    
    g_lastResult = result;
    g_uwbState = UWB_STATE_READY;
    return true;
    
#else
    // Real ranging with DW3000
    LOG_DEBUG("Ranging with %c...", peerID);
    
    TWRPayload twr;
    memset(&twr, 0, sizeof(twr));
    
    // Step 1: Send POLL
    UWBFrame pollFrame;
    build_frame(&pollFrame, MSG_TYPE_POLL, peerID, nullptr, 0);
    
    if (!dw3000_tx((uint8_t*)&pollFrame, sizeof(pollFrame))) {
        LOG_ERROR("Failed to send POLL");
        g_uwbState = UWB_STATE_READY;
        return false;
    }
    
    twr.pollTxTime = dw3000_get_tx_timestamp();
    LOG_TRACE("POLL sent, TX time: %llu", twr.pollTxTime);
    
    // Step 2: Wait for RESP
    uint8_t rxBuffer[128];
    uint16_t rxLen = 0;
    
    if (!dw3000_rx(rxBuffer, &rxLen, RANGING_TIMEOUT_MS)) {
        LOG_WARN("RESP timeout from %c", peerID);
        g_uwbState = UWB_STATE_READY;
        return false;
    }
    
    twr.respRxTime = dw3000_get_rx_timestamp();
    UWBFrame* respFrame = (UWBFrame*)rxBuffer;
    
    // Verify RESP
    if (respFrame->msgType != MSG_TYPE_RESP) {
        LOG_WARN("Invalid response type: 0x%02X", respFrame->msgType);
        g_uwbState = UWB_STATE_READY;
        return false;
    }
    
    // Extract responder timestamps from payload
    TWRPayload* respTWR = (TWRPayload*)respFrame->payload;
    twr.pollRxTime = respTWR->pollRxTime;
    twr.respTxTime = respTWR->respTxTime;
    
    LOG_TRACE("RESP received, RX time: %llu", twr.respRxTime);
    
    // Step 3: Send FINAL with our timestamps
    UWBFrame finalFrame;
    TWRPayload finalPayload = twr;  // Include all timestamps we have
    build_frame(&finalFrame, MSG_TYPE_FINAL, peerID, (uint8_t*)&finalPayload, sizeof(finalPayload));
    
    if (!dw3000_tx((uint8_t*)&finalFrame, sizeof(finalFrame))) {
        LOG_ERROR("Failed to send FINAL");
        g_uwbState = UWB_STATE_READY;
        return false;
    }
    
    twr.finalTxTime = dw3000_get_tx_timestamp();
    LOG_TRACE("FINAL sent, TX time: %llu", twr.finalTxTime);
    
    // Step 4: Wait for REPORT with final RX timestamp
    if (!dw3000_rx(rxBuffer, &rxLen, RANGING_TIMEOUT_MS)) {
        LOG_WARN("REPORT timeout from %c", peerID);
        g_uwbState = UWB_STATE_READY;
        return false;
    }
    
    UWBFrame* reportFrame = (UWBFrame*)rxBuffer;
    if (reportFrame->msgType != MSG_TYPE_REPORT) {
        LOG_WARN("Invalid report type: 0x%02X", reportFrame->msgType);
        g_uwbState = UWB_STATE_READY;
        return false;
    }
    
    TWRPayload* reportTWR = (TWRPayload*)reportFrame->payload;
    twr.finalRxTime = reportTWR->finalRxTime;
    
    LOG_TRACE("REPORT received, final RX: %llu", twr.finalRxTime);
    
    // Calculate distance
    float distance = calculate_distance_from_twr(&twr);
    
    // Estimate quality (based on successful exchange)
    float quality = 0.9f;
    if (distance < 0 || distance > 100) {
        quality = 0.3f;  // Suspicious value
    }
    
    result.success = true;
    result.distance = distance;
    result.quality = quality;
    result.timestamp = getTimestamp();
    result.peerID = peerID;
    
    LOG_INFO("Range to %c: %.2f m (Q=%.2f)", peerID, distance, quality);
    
    g_lastResult = result;
    g_uwbState = UWB_STATE_READY;
    return true;
#endif
}

/**
 * Get last result
 */
inline RangingResult uwb_get_last_result() {
    return g_lastResult;
}

/**
 * Get last quality
 */
inline float uwb_get_last_quality() {
    return g_lastResult.quality;
}

/**
 * Print status
 */
inline void uwb_print_status() {
    const char* states[] = {"IDLE", "INIT", "READY", "RANGING", "ERROR"};
    LOG_INFO("UWB Status: %s", states[g_uwbState]);
    
#if ENABLE_SIMULATION
    LOG_INFO("  Mode: SIMULATION");
#else
    LOG_INFO("  Mode: HARDWARE");
    LOG_INFO("  Channel: %d", UWB_CHANNEL);
#endif
    
    if (g_lastResult.success) {
        LOG_INFO("  Last: %c @ %.2fm (Q=%.2f)", 
                 g_lastResult.peerID, g_lastResult.distance, g_lastResult.quality);
    }
}

/**
 * Reset module
 */
inline void uwb_reset() {
    LOG_WARN("Resetting UWB...");
    g_uwbState = UWB_STATE_IDLE;
    uwb_init();
}

/**
 * Act as responder - listen for POLL and respond
 * Call this when NOT in your time slot to allow other units to range with you
 * 
 * @param timeout_ms How long to listen for POLL
 * @return true if successfully handled a ranging request
 */
inline bool uwb_respond(uint32_t timeout_ms) {
#if ENABLE_SIMULATION
    // In simulation, responder does nothing (only initiator generates data)
    return false;
#else
    if (g_uwbState != UWB_STATE_READY) {
        return false;
    }
    
    // Listen for POLL
    uint8_t rxBuffer[128];
    uint16_t rxLen = 0;
    
    if (!dw3000_rx(rxBuffer, &rxLen, timeout_ms)) {
        return false;  // No POLL received, that's OK
    }
    
    UWBFrame* pollFrame = (UWBFrame*)rxBuffer;
    
    // Check if it's a POLL for us
    uint16_t destAddr;
    memcpy(&destAddr, pollFrame->destAddr, 2);
    if (destAddr != (uint16_t)UNIT_ID) {
        return false;  // Not for us
    }
    
    if (pollFrame->msgType != MSG_TYPE_POLL) {
        return false;  // Not a POLL
    }
    
    uint64_t pollRxTime = dw3000_get_rx_timestamp();
    
    // Get initiator ID
    uint16_t srcAddr;
    memcpy(&srcAddr, pollFrame->sourceAddr, 2);
    char initiatorID = (char)srcAddr;
    
    LOG_TRACE("POLL from %c, RX: %llu", initiatorID, pollRxTime);
    
    // Build and send RESP with our timestamps
    TWRPayload respPayload;
    respPayload.pollRxTime = pollRxTime;
    
    UWBFrame respFrame;
    build_frame(&respFrame, MSG_TYPE_RESP, initiatorID, (uint8_t*)&respPayload, sizeof(respPayload));
    
    // Small delay before responding
    delayMicroseconds(100);
    
    if (!dw3000_tx((uint8_t*)&respFrame, sizeof(respFrame))) {
        LOG_ERROR("Failed to send RESP");
        return false;
    }
    
    uint64_t respTxTime = dw3000_get_tx_timestamp();
    respPayload.respTxTime = respTxTime;
    
    LOG_TRACE("RESP sent, TX: %llu", respTxTime);
    
    // Wait for FINAL
    if (!dw3000_rx(rxBuffer, &rxLen, RANGING_TIMEOUT_MS)) {
        LOG_TRACE("FINAL timeout");
        return false;
    }
    
    UWBFrame* finalFrame = (UWBFrame*)rxBuffer;
    if (finalFrame->msgType != MSG_TYPE_FINAL) {
        LOG_WARN("Expected FINAL, got 0x%02X", finalFrame->msgType);
        return false;
    }
    
    uint64_t finalRxTime = dw3000_get_rx_timestamp();
    
    LOG_TRACE("FINAL received, RX: %llu", finalRxTime);
    
    // Send REPORT with final RX timestamp
    TWRPayload reportPayload;
    reportPayload.finalRxTime = finalRxTime;
    
    UWBFrame reportFrame;
    build_frame(&reportFrame, MSG_TYPE_REPORT, initiatorID, (uint8_t*)&reportPayload, sizeof(reportPayload));
    
    delayMicroseconds(100);
    
    if (!dw3000_tx((uint8_t*)&reportFrame, sizeof(reportFrame))) {
        LOG_ERROR("Failed to send REPORT");
        return false;
    }
    
    LOG_DEBUG("Responded to ranging from %c", initiatorID);
    
    return true;
#endif
}

#endif // DW3000_DRIVER_H
