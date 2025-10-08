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
│  (ESP32-A)  │<------->│  (ESP32-B)  │<------->│  (ESP32-C)  │
└──────┬──────┘   UWB   └──────┬──────┘   UWB   └──────┬──────┘
       │                       │                        │
       │         Wi-Fi         │          Wi-Fi         │
       └───────────┬───────────┴────────────┬───────────┘
                   │                        │
                   v                        v
              ┌─────────────────────────────────┐
              │    Raspberry Pi Hub Server      │
              │  - Collects distance data       │
              │  - Calculates audio volumes     │
              │  - Hosts web dashboard          │
              └─────────────┬───────────────────┘
                            │
                            │ Browser
                            v
                   ┌─────────────────┐
                   │  Web Dashboard  │
                   │  - Network graph │
                   │  - Live distances│
                   │  - CSV export    │
                   └─────────────────┘
```

---

## Getting Started

### Step 1: Set Up the Raspberry Pi

#### 1.1 Initial Raspberry Pi Setup

If you haven't set up your Raspberry Pi yet:

1. **Install Raspberry Pi OS:**
   - Download and install Raspberry Pi Imager from https://www.raspberrypi.com/software/
   - Insert your SD card (16GB or larger) into your computer
   - Open Raspberry Pi Imager

2. **Configure the OS (important!):**
   - Click "Choose OS" → Select "Raspberry Pi OS (32-bit)" or "(64-bit)"
   - Click "Choose Storage" → Select your SD card
   - **Before clicking "Write"**, click the **gear icon** in the bottom right corner
   
   **In the Advanced Options menu:**
   - **Enable SSH**
     - Check "Enable SSH"
     - Select "Use password authentication"
   
   - **Set username and password**
     - Username: `pi` (or your choice)
     - Password: Set a secure password (you'll need this!)
   
   - **Configure Wi-Fi**
     - Check "Configure wireless LAN"
     - SSID: Your Wi-Fi network name
     - Password: Your Wi-Fi password
     - **IMPORTANT:** Wi-Fi country: Select your country
     - **IMPORTANT:** Make sure it's a 2.4GHz network (ESP32 requirement)
   
   - **Set locale settings**
     - Set your timezone and keyboard layout
   
   - Click "Save" to return to the main screen
   - Click "Write" to flash the SD card

3. **Boot the Raspberry Pi:**
   - Remove the SD card from your computer
   - Insert it into your Raspberry Pi
   - Connect power to the Pi
   - Wait 1-2 minutes for first boot and Wi-Fi connection

4. **Connect to your Raspberry Pi via SSH:**
   
   **From Windows:**
   - Open Command Prompt or PowerShell
   - Type: `ssh pi@raspberrypi.local`
   - If that doesn't work, you'll need to find the Pi's IP address in your router's admin panel
   - Then use: `ssh pi@<ip-address>` (e.g., `ssh pi@192.168.1.50`)
   - Enter the password you set in step 2
   
   **From Mac/Linux:**
   - Open Terminal
   - Type: `ssh pi@raspberrypi.local`
   - If that doesn't work, use: `ssh pi@<ip-address>`
   - Enter the password you set in step 2
   
   **Alternative - Direct Connection:**
   - If SSH doesn't work, connect a monitor, keyboard, and mouse to the Pi
   - Login with your username and password
   - Open Terminal on the Pi desktop

5. **Update the system:**
   ```bash
   sudo apt update
   sudo apt upgrade -y
   ```
   This may take several minutes.

#### 1.2 Install the Hub Software

1. **Install required system packages:**
   ```bash
   sudo apt install -y python3 python3-pip git
   ```

2. **Download the software:**
   ```bash
   cd ~/
   git clone https://github.com/imetech-engineering/uwb-proximity-chat-poc
   cd uwb-proximity-chat-poc
   cd rpi
   ```

3. **Install Python dependencies:**
   ```bash
   python3 -m pip install -r requirements.txt
   ```

#### 1.3 Find Your Raspberry Pi's IP Address

You'll need this IP address to configure the ESP32 units.

   ```bash
hostname -I
```

Example output: `192.168.1.100 fe80::...`  
The first address (e.g., `192.168.1.100`) is what you need.

**Write this IP address down!**

#### 1.4 Start the Server

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

Leave this running. The server is now waiting for ESP32 units to connect.

### Step 2: Program the ESP32 Units

You need to flash firmware onto each ESP32 board. You'll do this once per board, giving each a unique ID.

#### 2.1 Prepare Your Computer

**Install Arduino IDE:**
1. Download from https://www.arduino.cc/en/software
2. Install and open Arduino IDE

**Add ESP32 Board Support:**
1. Go to `File` → `Preferences`
2. In "Additional Board Manager URLs", add:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Go to `Tools` → `Board` → `Boards Manager`
4. Search for "esp32" and install "esp32 by Espressif Systems"

**Install Required Libraries:**
1. Go to `Tools` → `Manage Libraries`
2. Search and install:
   - `ArduinoJson` by Benoit Blanchon

#### 2.2 Configure and Flash Each ESP32

**For each ESP32 board, repeat these steps:**

1. **Open the firmware:**
   - In Arduino IDE, go to `File` → `Open`
   - Navigate to `uwb-proximity-chat-poc/esp32/unit_firmware/`
   - Open `unit_firmware.ino`
   - Arduino IDE will open the sketch with all files (`main.cpp`, `config.h`, `dw3000_driver.h`, `utils.h`, `wifi_udp.h`) visible as tabs

2. **Edit the configuration:**
   - Open the file `config.h` (should appear as a tab in Arduino IDE)
   - Change these four settings:

   ```cpp
   // UNIQUE FOR EACH UNIT! 
   // First board = 'A', second = 'B', third = 'C', etc.
   #define UNIT_ID 'A'
   
   // YOUR WI-FI NETWORK (same 2.4GHz network as Raspberry Pi)
   #define WIFI_SSID "YourNetworkName"
   #define WIFI_PASS "YourPassword"
   
   // YOUR RASPBERRY PI IP ADDRESS (from Step 1.3)
   #define HUB_UDP_IP "192.168.1.100"  // Replace with your actual IP!
   ```

3. **Select the board:**
   - Connect the Makerfabs ESP32 UWB board to your computer via USB (ensure the ESP32 is not connected to any other power source)
   - Go to `Tools` → `Board` → `ESP32 Arduino`
   - Select `ESP32 Dev Module`
   - Go to `Tools` → `Port` and select the COM port that appeared when you plugged in the board

4. **Upload the firmware:**
   - Click the **Upload** button (right arrow icon) in Arduino IDE
   - Wait for "Done uploading" message
   - The board will restart automatically

5. **Verify it works:**
   - Open `Tools` → `Serial Monitor`
   - Set baud rate to `115200`
   - You should see messages about connecting to Wi-Fi and starting UWB

6. **Repeat for next board:**
   - Disconnect this ESP32
   - Connect the next ESP32
   - **IMPORTANT:** Change `UNIT_ID` to 'B', then 'C', etc.
   - Upload again

**You need at least 3 units for the system to work properly.**

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
