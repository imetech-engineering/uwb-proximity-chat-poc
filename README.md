# UWB Proximity Chat System

**Real-time distance tracking and audio volume simulation for events**

---

## What is this?

This system uses Ultra-Wideband (UWB) radio technology to accurately measure distances between people at events. It then simulates how audio volume should change based on proximity - the closer people are, the louder they hear each other.

Perfect for:
- Event planning and audio system setup
- Testing proximity-based audio concepts
- Visualizing crowd dynamics in real-time

---

## System Components

### Hardware Required

1. **Raspberry Pi** (one per system)
   - Acts as the central hub
   - Runs the web interface
   - Collects and displays data
   - Any Raspberry Pi 3/4/5 works

2. **Makerfabs ESP32 UWB DW3000 boards** (3 or more)
   - Integrated ESP32 and DW3000 on single board
   - No wiring or assembly required
   - Worn by participants
   - Measure distances between each other
   - Send data wirelessly to the Raspberry Pi
   - Battery powered for mobility
   - Product info: https://github.com/Makerfabs/Makerfabs-ESP32-UWB-DW3000

3. **Wi-Fi Network**
   - All devices must be on the same network
   - 2.4GHz network required (ESP32 limitation)

### Software Included

- **Raspberry Pi Hub Server** - Web interface and data collector
- **ESP32 Firmware** - Distance measurement software
- **Web Dashboard** - Real-time visualization

---

## Quick Overview

```
┌─────────────┐         ┌─────────────┐         ┌─────────────┐
│  Person A   │         │  Person B   │         │  Person C   │
│  (ESP32-A)  │◄────────┤  (ESP32-B)  ├────────►│  (ESP32-C)  │
└──────┬──────┘   UWB   └──────┬──────┘   UWB   └──────┬──────┘
       │                       │                        │
       │         Wi-Fi         │          Wi-Fi         │
       └───────────┬───────────┴────────────┬───────────┘
                   │                        │
                   ▼                        ▼
              ┌─────────────────────────────────┐
              │    Raspberry Pi Hub Server      │
              │  ● Collects distance data       │
              │  ● Calculates audio volumes     │
              │  ● Hosts web dashboard          │
              └─────────────┬───────────────────┘
                            │
                            │ Browser
                            ▼
                   ┌─────────────────┐
                   │  Web Dashboard  │
                   │  ● Network graph │
                   │  ● Live distances│
                   │  ● CSV export    │
                   └─────────────────┘
```

---

## Getting Started

### Step 1: Set Up the Raspberry Pi

1. **Install the software:**
   ```bash
   # On your Raspberry Pi
   cd ~/
   git clone <repository-url>
   cd Software/rpi
   ```

2. **Install dependencies:**
   ```bash
   python3 -m pip install -r requirements.txt
   ```

3. **Note your Raspberry Pi's IP address:**
   
   On Raspberry Pi (Linux):
   ```bash
   hostname -I
   ```
   
   On Windows (if testing locally):
   ```powershell
   ipconfig
   ```
   Look for "IPv4 Address" under your active network adapter.
   
   Write this down - you'll need it for the ESP32 configuration!

4. **Start the server:**
   ```bash
   python3 server.py
   ```

   You should see:
   ```
   ============================================================
     UWB Proximity Chat - Hub Server
     IMeTech Engineering
   ============================================================
   Starting server on port 8000...
   UDP listener on port 9999
   Simulation mode: OFF
   ============================================================
   ```

### Step 2: Configure the ESP32 Units

1. **Open the firmware configuration file:**
   ```
   esp32/unit_firmware/config.h
   ```

2. **Edit these settings for EACH unit:**

   ```cpp
   // UNIQUE FOR EACH UNIT! Change this for every ESP32
   #define UNIT_ID 'A'  // First unit = 'A', second = 'B', third = 'C', etc.
   
   // YOUR WI-FI NETWORK
   #define WIFI_SSID "YourNetworkName"     // Your Wi-Fi name
   #define WIFI_PASS "YourPassword"        // Your Wi-Fi password
   
   // YOUR RASPBERRY PI IP ADDRESS (from Step 1.3)
   #define HUB_UDP_IP "192.168.1.100"      // Replace with your Pi's IP
   ```

3. **Flash each ESP32:**
   - Connect ESP32 to computer via USB
   - Open in Arduino IDE or PlatformIO
   - Upload the firmware
   - Repeat for each unit with different `UNIT_ID`

### Step 3: Test the System

1. **Power on all ESP32 units**
   - They should connect to Wi-Fi automatically
   - LEDs will blink when connected

2. **Open the web dashboard**
   - On any computer/tablet/phone on the same network
   - Go to: `http://<raspberry-pi-ip>:8000`
   - Example: `http://192.168.1.100:8000`

3. **You should see:**
   - Network graph showing all units
   - Real-time distance measurements
   - Simulated audio volumes

---

## Using the System

### Web Dashboard Features

**Network Graph**
- Shows all units as circles
- Lines connect units that can measure each other
- Distance labels show current separation
- Updates in real-time

**Distance Pairs Table**
- Lists all unit pairs
- Shows exact distances
- Displays measurement quality
- Calculates simulated audio volume

**Controls**
- **Export CSV**: Download all measurement data
- **Refresh**: Manual data refresh
- **Clear**: Reset the display

### Understanding the Display

**Distance**: Physical separation in meters between two units

**Quality**: Measurement confidence (0.0 = unreliable, 1.0 = perfect)

**Volume**: Simulated audio loudness (0.0 = silent, 1.0 = maximum)
- Closer distance = higher volume
- Quality affects volume stability

---

## Configuration

### Raspberry Pi Settings

Edit `rpi/config.yaml` to customize:

```yaml
# Network ports
network:
  udp_listen_port: 9999      # Port to receive data from ESP32 units
  websocket_port: 8000       # Port for web dashboard

# Audio simulation model
audio:
  model: "inverse_square"    # How volume decreases with distance
  max_distance: 10.0         # Maximum audible distance (meters)
  min_volume: 0.0            # Minimum volume
  max_volume: 1.0            # Maximum volume

# Data export
persistence:
  csv_export_enabled: true   # Save data to CSV file
  csv_export_path: "./data/ranging_data.csv"
```

### ESP32 Settings

Edit `esp32/unit_firmware/config.h`:

```cpp
// How often to measure distances (milliseconds)
#define RANGING_INTERVAL_MS 500  // Default: 2 measurements per second

// Minimum quality to accept measurement
#define QUALITY_THRESHOLD 0.5    // Default: 0.5 (50% confidence)
```

---

## Calibration

For accurate distance measurements, you may need to calibrate the system:

1. Place two units at a known distance (e.g., exactly 2.0 meters)
2. Note the measured distance on the dashboard
3. Calculate the error: `error = measured - actual`
4. Edit `esp32/unit_firmware/config.h`:
   ```cpp
   #define DIST_OFFSET_M -0.15  // If measured is 2.15m, set offset to -0.15
   ```
5. Re-flash the ESP32 units

For more detailed calibration, see `esp32/docs/CALIBRATION.md`

---

## Troubleshooting

### Problem: Web dashboard shows "Waiting for data..."

**Check:**
1. Is the Raspberry Pi server running? (`python3 server.py`)
2. Are ESP32 units powered on?
3. Did ESP32 units connect to Wi-Fi? (check serial monitor)
4. Is the ESP32 configured with correct Raspberry Pi IP?

### Problem: ESP32 won't connect to Wi-Fi

**Check:**
1. Is the Wi-Fi name and password correct in `config.h`?
2. Is it a 2.4GHz network? (ESP32 doesn't support 5GHz)
3. Is the network reachable from the ESP32 location?

### Problem: Distances seem incorrect

**Solutions:**
1. Run the calibration procedure (see above)
2. Ensure units have clear line-of-sight
3. Keep units away from metal objects
4. Check antenna orientation (should be vertical)

### Problem: Measurements are unstable/jumping

**Solutions:**
1. Increase `QUALITY_THRESHOLD` in ESP32 config.h
2. Check Wi-Fi signal strength
3. Reduce network congestion
4. Keep units stationary during measurement

### Problem: Can't access web dashboard

**Check:**
1. Are you on the same network as the Raspberry Pi?
2. Is the IP address correct?
3. Is port 8000 being used by another program?
4. Try: `http://<pi-ip>:8000` not `https://`

---

## Data Export

The system automatically logs all measurements (when not in simulation mode).

**To export data:**
1. Click "Export CSV" button in the dashboard
2. File downloads with format:
   ```csv
   timestamp,node,peer,distance_m,quality,volume
   2025-10-06T14:23:45,A,B,2.45,0.95,0.85
   2025-10-06T14:23:45,A,C,5.32,0.88,0.45
   ```

**CSV Fields:**
- `timestamp`: When measurement was taken (ISO format)
- `node`: Source unit (A, B, C, ...)
- `peer`: Target unit (A, B, C, ...)
- `distance_m`: Distance in meters
- `quality`: Measurement quality (0.0-1.0)
- `volume`: Simulated audio volume (0.0-1.0)

---

## Hardware Setup

### Hardware Assembly

The Makerfabs ESP32 UWB DW3000 is a ready-to-use development board with ESP32 and DW3000 already integrated. No wiring or soldering required.

**Setup:**
- Connect USB cable for programming and power
- Or connect battery pack for portable operation
- Keep antenna area clear of obstructions

**Mounting:**
- Attach to belt or worn on arm
- Keep antenna exposed and vertical
- Ensure USB port accessible for charging/programming

---

## Safety & Best Practices

**Electrical Safety:**
- Use proper 3.3V power supply
- Don't exceed voltage limits
- Avoid short circuits

**RF Safety:**
- UWB operates at very low power
- Safe for continuous use
- Complies with FCC/CE regulations

**Operational:**
- Keep units dry
- Avoid dropping or impacts
- Don't cover antennas
- Charge batteries between events

---

## Technical Specifications

**Range:**
- Indoor: up to 50 meters
- Outdoor: up to 100 meters
- Accuracy: ±10-30cm (after calibration)

**Update Rate:**
- Default: 2 measurements per second per pair
- Configurable: 0.5 - 10 Hz

**Battery Life:**
- Typical: 6-8 hours continuous use
- Depends on battery capacity and update rate

**Network Requirements:**
- Wi-Fi 2.4GHz (802.11 b/g/n)
- UDP port 9999 accessible
- Low bandwidth: ~2 KB/s per unit

---

## Support

**Documentation:**
- `QUICKSTART.md` - Quick setup guide
- `esp32/docs/CALIBRATION.md` - Distance calibration

**Configuration Files:**
- `rpi/config.yaml` - Server settings
- `esp32/unit_firmware/config.h` - ESP32 settings

**Logs:**
- Server logs: `rpi/logs/hub.log`
- ESP32 logs: Arduino serial monitor (115200 baud)

---

## License

See `LICENSE` file for details.

---

## Credits

Developed by **IMeTech Engineering**  
Website: https://imetech.nl/

For SLVN Events proximity-based audio project.

---

**Version:** 1.0  
**Last Updated:** October 2025
