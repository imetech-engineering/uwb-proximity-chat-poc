# Test Plan & Validation Procedures

Comprehensive testing guide for the UWB Proximity Chat proof-of-concept system.

## 🎯 Test Objectives

1. **Functional Validation**: Verify all components work as designed
2. **Performance Validation**: Confirm system meets latency and accuracy targets
3. **Integration Validation**: Ensure components communicate correctly
4. **Reliability Validation**: Test error handling and recovery
5. **User Experience**: Validate UI usability and responsiveness

## ✅ Acceptance Criteria

The POC is considered successful if it meets these criteria:

### Primary Criteria (Must Pass)

| Criterion | Target | Measurement Method |
|-----------|--------|-------------------|
| Distance accuracy (LOS) | ±10cm at 1m-3m | Tape measure comparison |
| Distance precision | Std dev <5cm | Statistical analysis (30 samples) |
| UI update rate | ≥2 Hz | Browser dev tools, timestamp analysis |
| WebSocket latency | <500ms | Packet timestamp comparison |
| System uptime | >1 hour continuous | Runtime monitoring |
| Multi-unit support | 3 units simultaneous | Visual confirmation in UI |

### Secondary Criteria (Should Pass)

| Criterion | Target | Measurement Method |
|-----------|--------|-------------------|
| Wi-Fi reconnection | <10s recovery | Simulate disconnect |
| UDP packet loss handling | Graceful degradation | Packet capture analysis |
| Browser compatibility | Chrome, Firefox, Safari | Manual testing |
| CSV export | Valid data format | File inspection |
| Simulation mode | Works without hardware | Run without DW3000 |

## 📝 Test Phases

### Phase 1: Component Testing (Individual Units)
### Phase 2: Integration Testing (System-Level)
### Phase 3: Performance Testing (Under Load)
### Phase 4: Reliability Testing (Error Conditions)
### Phase 5: User Acceptance Testing (End-to-End)

---

## Phase 1: Component Testing

### 1.1 ESP32 Firmware Build Test

**Objective:** Verify firmware compiles and uploads successfully

**Prerequisites:**
- Arduino IDE or PlatformIO installed
- ESP32 board connected via USB

**Procedure:**
1. Open `esp32/unit_firmware/main.cpp` in Arduino IDE
2. Select board: "ESP32 Dev Module"
3. Select port: (your ESP32 port)
4. Click Upload (Ctrl+U)
5. Open Serial Monitor (115200 baud)

**Expected Results:**
- ✅ Compilation succeeds (no errors)
- ✅ Upload completes successfully
- ✅ Serial output shows:
  ```
  UWB Proximity Chat Unit
  Firmware v0.1.0
  Unit ID: [A/B/C]
  Connecting to Wi-Fi...
  Wi-Fi connected!
  IP address: [x.x.x.x]
  ```

**Pass Criteria:**
- Firmware compiles without errors
- Serial output confirms initialization
- Wi-Fi connects within 10 seconds

---

### 1.2 ESP32 Simulation Mode Test

**Objective:** Verify firmware runs in simulation mode (without DW3000 hardware)

**Prerequisites:**
- Firmware uploaded with `ENABLE_SIMULATION true` in `config.h`

**Procedure:**
1. Set in `config.h`: `#define ENABLE_SIMULATION true`
2. Upload firmware
3. Monitor serial output for 60 seconds

**Expected Results:**
- ✅ Serial shows: "Simulation mode - no hardware configuration needed"
- ✅ Simulated ranging messages appear every ~500ms:
  ```
  Range: A->B = 2.34 m (Q=0.95) [SENT]
  Range: A->C = 1.87 m (Q=0.95) [SENT]
  ```
- ✅ No DW3000 errors

**Pass Criteria:**
- Simulated distances generated
- UDP packets sent successfully
- No crashes or errors

---

### 1.3 Raspberry Pi Server Startup Test

**Objective:** Verify hub server starts and listens correctly

**Prerequisites:**
- Python 3.8+ installed
- Dependencies installed (`pip install -r requirements.txt`)

**Procedure:**
1. Navigate to `rpi/` directory
2. Run: `python3 server.py`
3. Observe console output

**Expected Results:**
- ✅ Server starts without errors
- ✅ Console shows:
  ```
  UWB Proximity Chat Hub Starting
  UDP port: 9999
  REST port: 8000
  Hub is ready!
  ```
- ✅ No Python exceptions

**Pass Criteria:**
- Server starts successfully
- UDP listener initializes on port 9999
- HTTP server listens on port 8000
- No critical errors in logs

---

### 1.4 Web UI Load Test

**Objective:** Verify UI loads and displays correctly

**Prerequisites:**
- Hub server running

**Procedure:**
1. Open browser (Chrome, Firefox, or Safari)
2. Navigate to `http://[hub-ip]:8000`
3. Inspect page elements

**Expected Results:**
- ✅ Page loads within 2 seconds
- ✅ Header shows "UWB Proximity Chat"
- ✅ Status badge shows "Disconnected" (if no WebSocket yet)
- ✅ Network graph placeholder visible
- ✅ Buttons render correctly
- ✅ No console errors (check browser dev tools)

**Pass Criteria:**
- Page renders without errors
- All UI elements visible
- CSS styles applied correctly
- JavaScript loads without errors

---

## Phase 2: Integration Testing

### 2.1 UDP Communication Test

**Objective:** Verify units can send data to hub

**Prerequisites:**
- 1 ESP32 unit flashed and powered
- Hub server running
- Both on same network

**Procedure:**
1. Configure ESP32 `config.h` with correct hub IP
2. Power on ESP32
3. Monitor hub logs: `tail -f logs/hub.log`
4. Monitor ESP32 serial output

**Expected Results:**
- ✅ ESP32 shows: "UDP sent ... [SENT]"
- ✅ Hub logs show: "Received: A->B 1.23m (Q=0.95)"
- ✅ No "UDP send failed" errors

**Pass Criteria:**
- UDP packets reach hub
- Hub parses JSON correctly
- Packet rate ~2 Hz
- No errors on either side

---

### 2.2 WebSocket Connection Test

**Objective:** Verify browser connects to WebSocket and receives updates

**Prerequisites:**
- Hub server running
- At least 1 ESP32 sending data

**Procedure:**
1. Open browser to `http://[hub-ip]:8000`
2. Open browser dev tools → Network tab → WS filter
3. Observe WebSocket messages

**Expected Results:**
- ✅ WebSocket connection established
- ✅ Status badge changes to "Connected" (green)
- ✅ Messages received every ~500ms
- ✅ Network tab shows JSON snapshots

**Pass Criteria:**
- WebSocket connects within 2 seconds
- Messages arrive at 2 Hz rate
- No disconnections
- JSON parses correctly

---

### 2.3 End-to-End Data Flow Test

**Objective:** Verify complete data flow from unit to UI

**Prerequisites:**
- 2 ESP32 units (A and B) powered and connected
- Hub server running
- UI open in browser

**Procedure:**
1. Place Unit A and Unit B exactly 1.00m apart
2. Observe UI for 60 seconds
3. Record distance readings

**Expected Results:**
- ✅ UI shows both nodes A and B
- ✅ Pair A-B appears in table
- ✅ Distance shown: 0.90-1.10m (±10cm)
- ✅ Quality >0.5
- ✅ Volume >0.0
- ✅ Network graph shows connection line
- ✅ Updates continuously

**Pass Criteria:**
- Data appears in UI within 5 seconds
- Distance within ±10cm of actual
- No stale data warnings
- Smooth, continuous updates

---

### 2.4 Three-Unit Integration Test

**Objective:** Verify system handles 3 units correctly

**Prerequisites:**
- 3 ESP32 units (A, B, C) flashed with different IDs
- Hub server running
- UI open

**Procedure:**
1. Power on all 3 units
2. Arrange in triangle: A-B = 1m, B-C = 1m, A-C = 1.4m
3. Observe UI for 60 seconds

**Expected Results:**
- ✅ UI shows nodes: A, B, C
- ✅ Pairs table shows 3 rows: A-B, A-C, B-C
- ✅ Network graph shows triangle topology
- ✅ Distances approximately correct:
  - A-B: ~1.0m
  - B-C: ~1.0m
  - A-C: ~1.4m (1.0 × √2)
- ✅ All pairs updating continuously

**Pass Criteria:**
- All 3 nodes visible
- All 3 pairs present
- No missing or stale data
- Distances reasonable (±20cm for 3-unit setup)

---

## Phase 3: Performance Testing

### 3.1 Distance Accuracy Test

**Objective:** Measure ranging accuracy at multiple distances

**Prerequisites:**
- 2 calibrated units
- Tape measure (3m+, 1mm precision)
- Flat, open area (LOS)

**Procedure:**
1. Place units at exactly 0.50m apart
2. Collect 30 measurements from serial/UI
3. Calculate mean and std dev
4. Repeat at: 1.00m, 2.00m, 3.00m, 4.00m

**Data Collection Template:**

| True Distance | Mean Measured | Std Dev | Min | Max | Error |
|---------------|---------------|---------|-----|-----|-------|
| 0.50m | _____ | _____ | _____ | _____ | _____ |
| 1.00m | _____ | _____ | _____ | _____ | _____ |
| 2.00m | _____ | _____ | _____ | _____ | _____ |
| 3.00m | _____ | _____ | _____ | _____ | _____ |
| 4.00m | _____ | _____ | _____ | _____ | _____ |

**Pass Criteria:**
- Error <10cm at 1m-3m
- Std dev <5cm at all distances
- No outliers >20cm

---

### 3.2 Update Latency Test

**Objective:** Measure end-to-end latency from ranging to UI display

**Prerequisites:**
- 2 units running
- Hub server running
- UI open with dev tools

**Procedure:**
1. Enable dev panel in UI (click "Toggle Dev Info")
2. Correlate ESP32 serial timestamps with UI update times
3. Measure 20 samples
4. Calculate average latency

**Measurement:**
```
Latency = UI_timestamp - ESP32_timestamp
```

**Expected Results:**
- ✅ Average latency: 50-200ms
- ✅ Maximum latency: <500ms
- ✅ No outliers >1000ms

**Pass Criteria:**
- Average latency <500ms
- 95th percentile <1000ms
- No complete update failures

---

### 3.3 Sustained Load Test

**Objective:** Verify system runs stably under continuous operation

**Prerequisites:**
- All 3 units powered
- Hub server running
- UI open

**Procedure:**
1. Start system
2. Let run continuously for 1 hour
3. Monitor CPU, memory, network usage
4. Record any errors or crashes

**Monitoring:**
- Hub CPU: `top -p $(pgrep -f server.py)`
- Hub memory: Check RSS in `top`
- ESP32 serial: Look for errors, resets

**Expected Results:**
- ✅ No crashes or restarts
- ✅ Hub memory stable (no leaks)
- ✅ Hub CPU <20% average
- ✅ No ESP32 watchdog resets
- ✅ WebSocket stays connected
- ✅ UI remains responsive

**Pass Criteria:**
- 1 hour continuous operation
- No crashes or freezes
- Memory usage stable (±10%)
- UI remains usable

---

### 3.4 Wi-Fi Resilience Test

**Objective:** Test recovery from Wi-Fi disruption

**Prerequisites:**
- 1 ESP32 unit running
- Hub server running
- Access to Wi-Fi router

**Procedure:**
1. System running normally
2. Disable Wi-Fi (unplug router or block MAC)
3. Wait 30 seconds
4. Re-enable Wi-Fi
5. Monitor recovery

**Expected Results:**
- ✅ ESP32 detects disconnect (serial logs)
- ✅ ESP32 attempts reconnection
- ✅ ESP32 reconnects within 10 seconds
- ✅ UDP data resumes
- ✅ UI updates resume
- ✅ No manual intervention needed

**Pass Criteria:**
- Automatic reconnection within 30 seconds
- Data flow resumes normally
- No persistent errors

---

## Phase 4: Reliability Testing

### 4.1 Invalid Packet Handling Test

**Objective:** Verify hub handles malformed packets gracefully

**Procedure:**

Send various invalid packets via `netcat`:

```bash
# Missing fields
echo '{"node":"A"}' | nc -u [hub-ip] 9999

# Invalid JSON
echo 'not json' | nc -u [hub-ip] 9999

# Out of range
echo '{"node":"A","peer":"B","distance":-5,"quality":0.9}' | nc -u [hub-ip] 9999

# Invalid types
echo '{"node":"A","peer":"B","distance":"hello","quality":0.9}' | nc -u [hub-ip] 9999
```

**Expected Results:**
- ✅ Hub logs warnings for each invalid packet
- ✅ Hub continues running (no crash)
- ✅ Invalid counter increments (`/api/stats`)
- ✅ Valid packets still processed correctly

**Pass Criteria:**
- No crashes from invalid input
- Proper logging of errors
- System remains operational

---

### 4.2 WebSocket Disconnect/Reconnect Test

**Objective:** Verify UI handles WebSocket disruptions

**Procedure:**
1. Open UI, confirm connected
2. Stop hub server
3. Wait 5 seconds
4. Restart hub server
5. Observe UI behavior

**Expected Results:**
- ✅ UI status changes to "Disconnected" (red)
- ✅ UI attempts reconnection (console logs)
- ✅ After hub restart, UI reconnects automatically
- ✅ Data flow resumes
- ✅ No page reload needed

**Pass Criteria:**
- Auto-reconnect within 10 seconds of hub restart
- No data loss (displays latest data)
- User not required to manually refresh

---

### 4.3 NLOS (Non-Line-of-Sight) Test

**Objective:** Observe system behavior with obstacles

**Procedure:**
1. Place 2 units 1m apart (LOS)
2. Record baseline measurements
3. Place obstacle between units (e.g., metal sheet, wall)
4. Observe distance and quality changes

**Expected Results:**
- ✅ Quality metric decreases (e.g., 0.9 → 0.4)
- ✅ Distance may increase or become erratic
- ✅ System continues to operate (no crashes)
- ✅ When quality <threshold, volume →0

**Pass Criteria:**
- System handles NLOS without crashing
- Quality metric reflects signal degradation
- Ranging continues (even if less accurate)

---

## Phase 5: User Acceptance Testing

### 5.1 UI Usability Test

**Objective:** Validate UI is intuitive and functional

**Test Participants:** 2-3 people unfamiliar with the system

**Tasks:**
1. Open UI and identify how many units are connected
2. Find the distance between Unit A and Unit B
3. Determine which pair has the highest volume
4. Export data as CSV
5. Toggle developer information panel

**Success Metrics:**
- ✅ All tasks completed without assistance
- ✅ No confusion about data interpretation
- ✅ Export works on first try
- ✅ Positive subjective feedback

**Pass Criteria:**
- ≥80% task completion rate
- Users can interpret data correctly
- No critical usability issues

---

### 5.2 CSV Export Validation Test

**Objective:** Verify exported data is complete and correct

**Procedure:**
1. Run system for 5 minutes
2. Click "Export CSV" button
3. Open CSV file in Excel/LibreOffice
4. Validate data

**Expected Results:**
- ✅ File downloads successfully
- ✅ Valid CSV format (opens in spreadsheet apps)
- ✅ Headers present: timestamp, node_a, node_b, distance_m, quality, volume
- ✅ Data rows present (>100 rows for 5 min)
- ✅ Timestamps in ISO format
- ✅ No missing values
- ✅ Values in expected ranges

**Pass Criteria:**
- CSV is well-formed
- All fields present and valid
- Data matches UI display

---

### 5.3 Documentation Completeness Test

**Objective:** Verify documentation enables replication

**Test Participant:** Someone unfamiliar with the project

**Tasks:**
1. Read README.md
2. Follow setup instructions for ESP32
3. Follow setup instructions for Raspberry Pi
4. Get system running

**Success Metrics:**
- ✅ All steps clear and unambiguous
- ✅ No missing information
- ✅ System works after following docs
- ✅ <30 minutes from docs to running system (excluding compile time)

**Pass Criteria:**
- Participant can replicate system independently
- No unanswered questions
- Docs are clear and complete

---

## 📊 Test Report Template

### Test Execution Summary

**Date:** ___________  
**Tester:** ___________  
**System Version:** 0.1.0

### Results Overview

| Phase | Tests | Passed | Failed | Skipped |
|-------|-------|--------|--------|---------|
| 1. Component | __ | __ | __ | __ |
| 2. Integration | __ | __ | __ | __ |
| 3. Performance | __ | __ | __ | __ |
| 4. Reliability | __ | __ | __ | __ |
| 5. Acceptance | __ | __ | __ | __ |
| **TOTAL** | **__** | **__** | **__** | **__** |

### Acceptance Criteria Status

| Criterion | Target | Achieved | Pass/Fail |
|-----------|--------|----------|-----------|
| Distance accuracy | ±10cm | _____ | ☐ |
| Distance precision | <5cm std dev | _____ | ☐ |
| UI update rate | ≥2 Hz | _____ | ☐ |
| WebSocket latency | <500ms | _____ | ☐ |
| System uptime | >1 hour | _____ | ☐ |
| Multi-unit support | 3 units | _____ | ☐ |

### Issues Found

| ID | Severity | Description | Status |
|----|----------|-------------|--------|
| 1 | _____ | _____ | _____ |
| 2 | _____ | _____ | _____ |

**Severity:** Critical / High / Medium / Low

### Recommendations

1. _____________________
2. _____________________
3. _____________________

### Overall Assessment

☐ **PASS** - System meets all acceptance criteria  
☐ **PASS WITH RESERVATIONS** - Minor issues, but acceptable  
☐ **FAIL** - Critical issues prevent acceptance

**Signature:** _____________________  
**Date:** _____________________

---

## 🔧 Troubleshooting Guide

### Issue: ESP32 won't connect to Wi-Fi

**Symptoms:** Serial shows "Wi-Fi connection timeout"

**Checks:**
1. Verify SSID and password in `config.h`
2. Confirm 2.4GHz network (ESP32 doesn't support 5GHz)
3. Check router allows new clients
4. Try resetting ESP32 (press RESET button)

**Solution:** Correct credentials, restart ESP32

---

### Issue: Hub not receiving UDP packets

**Symptoms:** Hub logs show no "Received" messages

**Checks:**
1. Verify ESP32 shows "UDP sent ... [SENT]"
2. Confirm hub IP matches `HUB_UDP_IP` in `config.h`
3. Check firewall (allow UDP port 9999)
4. Verify both on same network/subnet
5. Try `netcat` manual send to isolate issue

**Solution:** Fix network configuration, disable firewall temporarily

---

### Issue: UI shows "Disconnected"

**Symptoms:** Status badge red, no data updates

**Checks:**
1. Verify hub server is running
2. Check WebSocket URL in browser dev tools
3. Confirm browser supports WebSocket
4. Check for CORS errors in console

**Solution:** Start/restart hub server, try different browser

---

### Issue: Distance readings wildly inaccurate

**Symptoms:** Readings 10x too large/small

**Checks:**
1. Run calibration procedure (see `CALIBRATION.md`)
2. Verify antennas connected
3. Check for metal objects nearby
4. Ensure line-of-sight
5. Confirm both units using same UWB channel

**Solution:** Calibrate, improve environment

---

## ✅ Final Checklist

Before considering testing complete:

- [ ] All Phase 1 tests passed
- [ ] All Phase 2 tests passed
- [ ] All Phase 3 tests passed (≥80% pass rate acceptable)
- [ ] All Phase 4 tests passed
- [ ] All Phase 5 tests passed
- [ ] Test report completed
- [ ] Issues documented
- [ ] Acceptance criteria met (or exceptions documented)
- [ ] Documentation validated by independent user

---

**Test Plan Version:** 1.0  
**Last Updated:** 2025-10-03  
**Next Review:** After POC completion or major changes

