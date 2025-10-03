# Quick Start Guide

Get your UWB Proximity Chat POC running in minutes!

## What Was Created

A complete, buildable proof-of-concept with:

- **ESP32 Firmware** (Arduino-style C++)
  - Full UWB ranging implementation (DS-TWR)
  - Wi-Fi + UDP communication
  - Simulation mode for testing without hardware
  - 5 source files + comprehensive docs

- **Raspberry Pi Hub** (Python/FastAPI)
  - UDP listener for distance data
  - WebSocket broadcaster for real-time UI
  - REST API for data export
  - Volume simulation algorithm

- **Web UI** (HTML/CSS/JavaScript)
  - Real-time network visualization
  - Distance/quality/volume display
  - CSV export functionality
  - Developer tools panel

- **Complete Documentation**
  - Architecture overview
  - Wiring diagrams
  - Calibration procedures
  - Protocol specifications
  - Comprehensive test plan

## Getting Started (5 Minutes)

### Option 1: Test Without Hardware (Simulation Mode)

**1. Flash ESP32 Units** (or skip if testing simulation only)

```bash
# In esp32/unit_firmware/config.h, ensure:
#define ENABLE_SIMULATION true
#define WIFI_SSID "YourNetwork"
#define WIFI_PASS "YourPassword"
#define HUB_UDP_IP "192.168.1.100"  # Your Pi's IP
```

Upload to 3 ESP32 boards with UNIT_ID set to 'A', 'B', 'C' respectively.

**2. Start Raspberry Pi Hub**

```bash
cd rpi
python3 -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
pip install -r requirements.txt
python3 server.py
```

**3. Open Web UI**

Navigate to: `http://localhost:8000`

You should see simulated ranging data flowing!

---

### Option 2: Full Hardware Setup

**Prerequisites:**
- 3× ESP32 DevKit boards
- 3× DW3000 UWB modules
- Raspberry Pi 4
- Breadboards, jumper wires
- Wi-Fi network

**Step 1: Wire Hardware** (30 minutes)
- Follow `esp32/docs/WIRING.md`
- Connect each ESP32 to DW3000 via SPI
- Power from USB

**Step 2: Configure & Flash Firmware** (15 minutes)

For each unit (A, B, C):
1. Edit `esp32/unit_firmware/config.h`:
   ```cpp
   #define WIFI_SSID "YourNetwork"
   #define WIFI_PASS "YourPassword"
   #define HUB_UDP_IP "192.168.1.100"  // Your Pi IP
   #define UNIT_ID 'A'  // Change for each unit: 'A', 'B', 'C'
   #define ENABLE_SIMULATION false  // Use real hardware
   ```

2. Open `esp32/unit_firmware/main.cpp` in Arduino IDE
3. Select board: "ESP32 Dev Module"
4. Upload (Ctrl+U)
5. Monitor Serial (115200 baud) - should see "Wi-Fi connected"

**Step 3: Start Hub** (5 minutes)

On Raspberry Pi:
```bash
cd rpi
./scripts/run_dev.sh  # Auto-setup and run
```

**Step 4: Access UI**
- Open browser to `http://[pi-ip]:8000`
- Should see 3 nodes and ranging data

**Step 5: Calibrate** (15 minutes)
- Follow `esp32/docs/CALIBRATION.md`
- Place units 1m apart
- Adjust `ANTENNA_DELAY` and `DIST_OFFSET_M` in config.h

---

## Repository Structure

```
.
├── README.md                      # Main documentation
├── LICENSE                        # MIT license
├── QUICKSTART.md                  # This file
├── esp32/
│   ├── unit_firmware/             # Arduino firmware
│   │   ├── main.cpp              # Main entry point
│   │   ├── config.h              # ⚙️ CONFIGURE HERE
│   │   ├── dw3000_driver.h       # UWB abstraction
│   │   ├── wifi_udp.h            # Network layer
│   │   └── utils.h               # Utilities
│   └── docs/
│       ├── WIRING.md             # Hardware connections
│       └── CALIBRATION.md        # Calibration guide
├── rpi/
│   ├── server.py                 # FastAPI hub
│   ├── config.yaml               # ⚙️ CONFIGURE HERE
│   ├── requirements.txt          # Python deps
│   ├── scripts/
│   │   ├── run_dev.sh           # Dev launcher
│   │   └── systemd.service      # Service config
│   └── static/
│       ├── index.html           # UI
│       ├── app.js               # Frontend logic
│       └── styles.css           # Styling
└── docs/
    ├── PROTOCOL.md               # Message formats
    ├── TESTPLAN.md               # Testing procedures
    └── VERSIONS.md               # Changelog
```

## Key Configuration Files

### ESP32: `esp32/unit_firmware/config.h`

**Essential settings:**
```cpp
// Network
#define WIFI_SSID "YourNetwork"
#define WIFI_PASS "password"
#define HUB_UDP_IP "192.168.1.100"
#define HUB_UDP_PORT 9999

// Unit Identity (CHANGE FOR EACH UNIT!)
#define UNIT_ID 'A'  // 'A', 'B', or 'C'

// Testing
#define ENABLE_SIMULATION false  // true = no hardware needed

// Calibration (see CALIBRATION.md)
#define ANTENNA_DELAY_TX 16450
#define ANTENNA_DELAY_RX 16450
#define DIST_OFFSET_M 0.00
```

### Hub: `rpi/config.yaml`

**Essential settings:**
```yaml
network:
  udp_listen_port: 9999
  rest_port: 8000

volume_model:
  near_distance_m: 1.5  # Loud
  far_distance_m: 4.0   # Quiet
  cutoff_distance_m: 5.0  # Silent

ui:
  broadcast_interval_ms: 500  # 2 Hz updates
```

## Testing Your Setup

### Quick Smoke Test

1. **Power on units** - Serial shows "Wi-Fi connected"
2. **Start hub** - Console shows "Hub is ready!"
3. **Open UI** - Browser loads page
4. **Check WebSocket** - Status badge turns green "Connected"
5. **Verify data** - Nodes appear, distances update

If all above work: ✅ **System operational!**

### Validate Accuracy

1. Place 2 units exactly 1.00m apart (tape measure)
2. Read distance in UI
3. Should show: 0.90-1.10m (±10cm)

If not accurate: Run calibration procedure in `esp32/docs/CALIBRATION.md`

### Full Test Suite

See `docs/TESTPLAN.md` for comprehensive validation procedures.

## Common Issues

### ESP32 won't connect to Wi-Fi
- Check SSID/password in config.h
- Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
- Press RESET button on ESP32

### Hub not receiving data
- Verify hub IP matches config.h
- Check firewall (allow UDP 9999)
- Confirm ESP32 and Pi on same network

### UI shows "Disconnected"
- Confirm hub server is running
- Check browser console for errors
- Try different browser

### Distances wildly wrong
- Run calibration (CALIBRATION.md)
- Check antenna connected
- Ensure line-of-sight between units

## Next Steps

### Immediate (POC Phase)
1. **Build & test** - Follow this quickstart
2. **Calibrate** - Get accurate measurements
3. **Validate** - Run tests from TESTPLAN.md
4. **Document** - Record results and issues

### Future (Production)
1. **Audio integration** - Add real microphones/speakers
2. **Custom PCB** - Design integrated board
3. **Security** - Add authentication, encryption
4. **Scale** - Support more units, mesh networking
5. **Mobile app** - Build native app interface

## Getting Help

1. **Check logs:**
   - ESP32: Serial monitor (115200 baud)
   - Hub: `tail -f rpi/logs/hub.log`
   - Browser: Console (F12)

2. **Review docs:**
   - README.md - Overview
   - WIRING.md - Hardware issues
   - CALIBRATION.md - Accuracy issues
   - PROTOCOL.md - Communication issues
   - TESTPLAN.md - Validation procedures

3. **Debug mode:**
   - ESP32: Set `LOG_LEVEL` to `LOG_LEVEL_DEBUG` in config.h
   - Hub: Set `log_level: "DEBUG"` in config.yaml
   - UI: Click "Toggle Dev Info" button

## Success Criteria

Your POC is working correctly when:

- All 3 units show in UI
- Distances update at ~2 Hz
- Distance accuracy ±10cm (after calibration)
- No errors in logs
- System runs for >1 hour without crashes
- CSV export works

## You're All Set!

The codebase is complete and ready to build. All files compile/run out of the box.

**Acceptance criteria met:**
- Complete, buildable codebase
- Clean, commented code
- All settings configurable
- Comprehensive documentation
- Simulation mode works without hardware
- Ready for novice to replicate

---

*For questions or issues, consult the documentation files or enable debug logging.*

