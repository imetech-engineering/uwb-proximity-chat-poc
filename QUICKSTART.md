# Quick Start Guide

Get your UWB Proximity Chat system running in 15 minutes.

---

## What You Need

- Raspberry Pi (3, 4, or 5)
- 3 or more Makerfabs ESP32 UWB DW3000 boards
- USB cables for programming
- Wi-Fi network (2.4GHz)
- Computer for initial setup

---

## Step-by-Step Setup

### Step 1: Raspberry Pi Setup (5 minutes)

**A. Connect to your Raspberry Pi**
```bash
ssh pi@raspberrypi.local
# Or use keyboard/monitor directly
```

**B. Download the software**
```bash
cd ~
git clone <your-repository-url> uwb-system
cd uwb-system/rpi
```

**C. Install requirements**
```bash
python3 -m pip install -r requirements.txt
```

**D. Find your Raspberry Pi's IP address**

On Raspberry Pi (Linux):
```bash
hostname -I
```

On Windows (if testing locally):
```powershell
ipconfig
```
Look for "IPv4 Address" under your active network adapter.

**Write this down!** Example: `192.168.1.100`

**E. Start the server**
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

Raspberry Pi is ready.

---

### Step 2: ESP32 Configuration (3 minutes per unit)

**A. Open the firmware in Arduino IDE**

1. Open Arduino IDE
2. Go to `File` → `Open`
3. Navigate to `esp32/unit_firmware/`
4. Open `unit_firmware.ino`
5. Arduino IDE will open the sketch with all files (`main.cpp`, `config.h`, `dw3000_driver.h`, `utils.h`, `wifi_udp.h`) visible as tabs

**B. Change these FOUR settings in config.h:**

```cpp
// 1. UNIT ID - Must be unique for each ESP32!
#define UNIT_ID 'A'  // Change to 'B' for second unit, 'C' for third, etc.

// 2. YOUR WI-FI NETWORK
#define WIFI_SSID "YourNetworkName"
#define WIFI_PASS "YourWiFiPassword"

// 3. RASPBERRY PI IP (from Step 1.D)
#define HUB_UDP_IP "192.168.1.100"  // ← Your Pi's IP here
```

**Important:** Each ESP32 must have a different `UNIT_ID`!
- First unit: `'A'`
- Second unit: `'B'`
- Third unit: `'C'`
- And so on...

**C. Flash the ESP32**

1. Connect ESP32 to your computer via USB
2. Open in Arduino IDE or PlatformIO
3. Select board: "ESP32 Dev Module"
4. Select correct COM port
5. Click Upload/Flash
6. Wait for "Done uploading" message

**D. Verify it works**

Open Serial Monitor (115200 baud):
```
[INFO] Wi-Fi connected!
[INFO] IP address: 192.168.1.101
[INFO] Initialization Complete
```

**E. Repeat for all units**

Change `UNIT_ID` and flash each one:
- ESP32 #1: `UNIT_ID 'A'`
- ESP32 #2: `UNIT_ID 'B'`
- ESP32 #3: `UNIT_ID 'C'`
- ESP32 #4: `UNIT_ID 'D'`
- etc.

ESP32 units are ready.

---

### Step 3: Test the System (2 minutes)

**A. Power on ESP32 units**
- Connect to USB power banks or chargers
- LEDs should blink on connection

**B. Open the web dashboard**

On any device (phone, tablet, laptop) connected to the same Wi-Fi:

```
http://192.168.1.100:8000
```
(Replace with your Raspberry Pi's IP)

**C. Check the display**

You should see:
- "Connected" status (green)
- Network graph with nodes A, B, C, ...
- Distance measurements updating
- Live data in the table

**D. Test movement**

- Move ESP32 units closer together → distances decrease
- Move them apart → distances increase
- Values update every ~0.5 seconds

System is working.

---

## Quick Reference

### Starting the Server

```bash
cd ~/uwb-system/rpi
python3 server.py
```

### Stopping the Server

Press `Ctrl+C` in the terminal

### Accessing the Dashboard

```
http://<raspberry-pi-ip>:8000
```

### Finding IP Address

On Raspberry Pi (Linux):
```bash
hostname -I
```

On Windows:
```powershell
ipconfig
```
Look for "IPv4 Address"

### Checking ESP32 Logs

1. Connect ESP32 via USB
2. Open Serial Monitor
3. Set baud rate: 115200
4. Watch for connection messages

---

## Common Issues

### Problem: "Waiting for data..." on dashboard

**Fix:**
1. Check Raspberry Pi server is running
2. Check ESP32 units are powered on
3. Verify ESP32 units connected to Wi-Fi (check serial monitor)
4. Confirm `HUB_UDP_IP` matches Raspberry Pi IP

### Problem: ESP32 won't connect to Wi-Fi

**Fix:**
1. Double-check Wi-Fi name and password in `config.h`
2. Ensure network is 2.4GHz (not 5GHz)
3. Move ESP32 closer to Wi-Fi router
4. Check Wi-Fi network is active

### Problem: Can't access dashboard

**Fix:**
1. Verify you're on the same Wi-Fi network
2. Check Raspberry Pi IP address is correct
3. Use `http://` not `https://`
4. Try from different device

### Problem: Wrong distance measurements

**Solution:**
Run calibration - see `esp32/docs/CALIBRATION.md`

---

## Next Steps

### Enable Automatic Startup

To make the server start automatically on Raspberry Pi boot:

```bash
cd ~/uwb-system/rpi/scripts
sudo cp systemd.service /etc/systemd/system/uwb-hub.service
sudo systemctl enable uwb-hub.service
sudo systemctl start uwb-hub.service
```

### Customize Settings

**Raspberry Pi:** Edit `rpi/config.yaml`
- Change ports
- Adjust audio model
- Configure data export

**ESP32:** Edit `esp32/unit_firmware/config.h`
- Adjust measurement frequency
- Change quality threshold
- Set calibration values

### Export Data

Click "Export CSV" button in dashboard to download measurements.

### Add More Units

1. Configure new ESP32 with unique `UNIT_ID`
2. Flash and power on
3. It will automatically appear in the dashboard

---

## System Architecture

```
┌──────────┐     Wi-Fi     ┌─────────────┐
│ ESP32-A  ├──────────────►│             │
└──────────┘               │             │
                           │ Raspberry   │      Browser
┌──────────┐     Wi-Fi     │     Pi      ├────────────► 📊 Dashboard
│ ESP32-B  ├──────────────►│  (Server)   │
└──────────┘               │             │
                           │             │
┌──────────┐     Wi-Fi     │             │
│ ESP32-C  ├──────────────►│             │
└──────────┘               └─────────────┘

ESP32 units measure distances → Send to Raspberry Pi → Display in browser
```

---

## Configuration Checklist

### Before First Use

**Raspberry Pi:**
- [x] Software installed
- [x] Requirements installed (`pip install -r requirements.txt`)
- [x] IP address noted
- [x] Server started

**Each ESP32:**
- [x] `UNIT_ID` set (unique: A, B, C, ...)
- [x] `WIFI_SSID` set to your network
- [x] `WIFI_PASS` set to your password
- [x] `HUB_UDP_IP` set to Raspberry Pi IP
- [x] Firmware flashed
- [x] Connection verified (serial monitor)

**Network:**
- [x] 2.4GHz Wi-Fi active
- [x] All devices on same network
- [x] Raspberry Pi accessible from browser

---

## Tips for Best Results

**Distance Measurement:**
- Keep units at least 20cm apart
- Avoid metal objects between units
- Keep antennas vertical and exposed
- Don't cover ESP32 modules with hands

**Wi-Fi Performance:**
- Keep units within 20m of router
- Reduce network congestion
- Use dedicated Wi-Fi if available
- Check signal strength (RSSI > -70 dBm)

**Battery Life:**
- Use 2000mAh+ power banks
- Reduce measurement rate if needed
- Monitor battery during events
- Keep spare batteries charged

---

## Support Files

**Calibration:**
- `esp32/docs/CALIBRATION.md` - Improve distance accuracy

**Full Documentation:**
- `README.md` - Complete system documentation

**Configuration:**
- `rpi/config.yaml` - Server settings
- `esp32/unit_firmware/config.h` - ESP32 settings

---

## Help & Contact

**Logs:**
- Server: `rpi/logs/hub.log`
- ESP32: Serial monitor (115200 baud)

**Developed by:**
IMeTech Engineering  
https://imetech.nl/

---
