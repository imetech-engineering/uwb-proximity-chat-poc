# UWB Proximity Chat System - Proof of Concept

A complete proof-of-concept demonstrating UWB-based proximity chat using ESP32 + DW3000 units and a Raspberry Pi hub. This POC focuses on reliable ranging and visualization without actual audio hardware.

## Project Goals

**Goals:**
- Demonstrate reliable UWB ranging (DW3000) between multiple units
- Central hub-driven logic for "who hears whom" based on distance
- Real-time web UI visualization of distances and simulated volume
- Clean, configurable, documented code ready for productization

**Non-goals (this POC):**
- Real microphones/speakers (hooks provided for future integration)
- Custom PCB design (breadboard/dev boards only)
- Audio processing/streaming

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      SYSTEM OVERVIEW                         │
└─────────────────────────────────────────────────────────────┘

   Unit A (ESP32)          Unit B (ESP32)          Unit C (ESP32)
   ┌──────────┐            ┌──────────┐            ┌──────────┐
   │ ESP32    │            │ ESP32    │            │ ESP32    │
   │ DevKit   │            │ DevKit   │            │ DevKit   │
   │          │            │          │            │          │
   │ DW3000◄──┼────UWB────►│ DW3000◄──┼────UWB────►│ DW3000   │
   │ (SPI)    │   Ranging  │ (SPI)    │   Ranging  │ (SPI)    │
   └────┬─────┘            └────┬─────┘            └────┬─────┘
        │                       │                       │
        │ Wi-Fi                 │ Wi-Fi                 │ Wi-Fi
        │ UDP JSON              │ UDP JSON              │ UDP JSON
        │ (distances)           │ (distances)           │ (distances)
        └───────────────┬───────┴───────────────────────┘
                        ▼
              ┌─────────────────────┐
              │  Raspberry Pi 4     │
              │  (Hub)              │
              │                     │
              │  ┌──────────────┐   │
              │  │ FastAPI      │   │
              │  │ Backend      │   │
              │  │              │   │
              │  │ • UDP Ingest │   │
              │  │ • Volume Sim │   │
              │  │ • WebSocket  │   │
              │  │ • REST API   │   │
              │  └──────┬───────┘   │
              └─────────┼───────────┘
                        │
                        │ WebSocket + HTTP
                        ▼
              ┌─────────────────────┐
              │   Web Browser       │
              │   (UI)              │
              │                     │
              │  • Distance Graph   │
              │  • Volume Display   │
              │  • Node Status      │
              │  • CSV Export       │
              └─────────────────────┘
```

### Data Flow

1. **Ranging**: Each ESP32 unit performs DS-TWR (Double-Sided Two-Way Ranging) with peers via UWB
2. **Upload**: Units send distance measurements as UDP JSON packets to hub
3. **Processing**: Hub computes simulated volume based on distance (closer = louder)
4. **Broadcast**: Hub streams updates via WebSocket to connected clients
5. **Visualization**: Web UI displays real-time network graph and metrics

## Repository Layout

```
.
├── README.md                      # This file
├── LICENSE                        # MIT license
├── esp32/                         # ESP32 firmware
│   ├── unit_firmware/
│   │   ├── main.cpp              # Arduino entrypoint
│   │   ├── config.h              # ⚙️ ALL ESP32 SETTINGS
│   │   ├── dw3000_driver.h       # UWB hardware abstraction
│   │   ├── wifi_udp.h            # Wi-Fi + UDP client
│   │   └── utils.h               # Helper utilities
│   └── docs/
│       ├── WIRING.md             # Pin mapping and connections
│       └── CALIBRATION.md        # Antenna calibration guide
├── rpi/                           # Raspberry Pi hub
│   ├── server.py                 # FastAPI application
│   ├── config.yaml               # ⚙️ ALL HUB SETTINGS
│   ├── requirements.txt          # Python dependencies
│   ├── static/
│   │   ├── index.html           # Web UI
│   │   ├── app.js               # Frontend logic
│   │   └── styles.css           # Styling
│   └── scripts/
│       ├── run_dev.sh           # Dev server launcher
│       └── systemd.service      # System service config
└── docs/                          # Documentation
    ├── TESTPLAN.md               # Validation procedures
    ├── PROTOCOL.md               # Message formats
    └── VERSIONS.md               # Changelog
```

## Quick Start

### Prerequisites

**Hardware:**
- 3× ESP32 DevKit boards (ESP32-WROOM or similar)
- 3× DW3000 UWB modules (Qorvo DWM3000 or compatible)
- 1× Raspberry Pi 4 (2GB+ recommended)
- Breadboards, jumper wires, USB cables
- Wi-Fi network or router

**Software:**
- Arduino IDE 2.x or PlatformIO
- Python 3.8+ on Raspberry Pi
- Modern web browser

### ESP32 Setup

#### Option 1: Arduino IDE

1. **Install ESP32 board support:**
   - Open Arduino IDE → Preferences
   - Add to "Additional Board Manager URLs": 
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Tools → Board Manager → Search "esp32" → Install

2. **Configure firmware:**
   ```bash
   # Edit esp32/unit_firmware/config.h
   # Set WIFI_SSID, WIFI_PASS, HUB_UDP_IP, UNIT_ID
   ```

3. **Upload:**
   - Open `esp32/unit_firmware/main.cpp` in Arduino IDE
   - Tools → Board → ESP32 Dev Module
   - Tools → Port → (select your ESP32)
   - Upload (Ctrl+U)
   - Repeat for all 3 units with different `UNIT_ID` ('A', 'B', 'C')

4. **Monitor:**
   - Tools → Serial Monitor (115200 baud)

#### Option 2: PlatformIO

1. **Create platformio.ini in esp32/unit_firmware/:**
   ```ini
   [env:esp32dev]
   platform = espressif32
   board = esp32dev
   framework = arduino
   monitor_speed = 115200
   lib_deps = 
       # Add DW3000 library when available
   ```

2. **Build and upload:**
   ```bash
   cd esp32/unit_firmware
   pio run --target upload
   pio device monitor
   ```

### Raspberry Pi Setup

1. **Clone repository:**
   ```bash
   cd ~
   git clone <your-repo-url> proximity-chat
   cd proximity-chat/rpi
   ```

2. **Create Python virtual environment:**
   ```bash
   python3 -m venv venv
   source venv/bin/activate
   pip install --upgrade pip
   pip install -r requirements.txt
   ```

3. **Configure hub:**
   ```bash
   # Edit config.yaml
   # Set udp_listen_port, volume thresholds, etc.
   nano config.yaml
   ```

4. **Run development server:**
   ```bash
   chmod +x scripts/run_dev.sh
   ./scripts/run_dev.sh
   ```
   Or manually:
   ```bash
   uvicorn server:app --host 0.0.0.0 --port 8000 --reload
   ```

5. **Access UI:**
   - Open browser: `http://<raspberry-pi-ip>:8000`

### Simulation Mode (No Hardware)

Test the system without UWB hardware:

1. **ESP32:** Set `ENABLE_SIMULATION true` in `config.h`
2. **Upload** firmware - units will generate synthetic distances
3. **Start hub** as normal
4. **View UI** - you'll see simulated ranging data

## Configuration

### ESP32 (`esp32/unit_firmware/config.h`)

All settings are at the top of `config.h`:
- **Network**: WIFI_SSID, WIFI_PASS, HUB_UDP_IP, HUB_UDP_PORT
- **Identity**: UNIT_ID ('A', 'B', or 'C')
- **UWB**: Channel, data rate, TX power, ranging interval
- **Calibration**: Antenna delay, distance offset
- **Debug**: LOG_LEVEL, simulation mode

### Raspberry Pi (`rpi/config.yaml`)

YAML configuration file:
- **Network**: UDP/WebSocket/REST ports
- **Volume Model**: Near/far thresholds, min/max volume
- **UI**: Refresh rate, developer tools
- **Data**: CSV export settings
- **Security**: API tokens, CORS origins

## Testing

See [`docs/TESTPLAN.md`](docs/TESTPLAN.md) for complete validation procedures.

**Quick smoke test:**
1. Power on all 3 ESP32 units, verify serial output shows "RANGING OK"
2. Start Raspberry Pi hub, check logs for "UDP listener started"
3. Open web UI, confirm 3 nodes appear
4. Move units closer/farther, observe distance changes in real-time
5. Export CSV data via UI button

**Acceptance criteria:**
- ✅ Distinguish 1m vs 3m reliably (±10cm in LOS)
- ✅ UI updates ≥2 Hz
- ✅ WebSocket latency <500ms
- ✅ Simulation mode works end-to-end

## Protocols

See [`docs/PROTOCOL.md`](docs/PROTOCOL.md) for detailed message formats.

**Unit → Hub (UDP JSON):**
```json
{"node":"A","peer":"B","distance":1.23,"quality":0.95,"ts":1730567890}
```

**Hub → UI (WebSocket JSON):**
```json
{
  "nodes":["A","B","C"],
  "pairs":[
    {"a":"A","b":"B","d":1.23,"q":0.95,"vol":0.82}
  ],
  "config":{"near_m":1.5,"far_m":4.0}
}
```

## Hardware Wiring

See [`esp32/docs/WIRING.md`](esp32/docs/WIRING.md) for complete pinout.

**Quick reference (ESP32 ↔ DW3000):**
- VSPI (default SPI bus)
- CS: GPIO 5
- IRQ: GPIO 17
- RST: GPIO 27
- 3.3V power rail

## Calibration

See [`esp32/docs/CALIBRATION.md`](esp32/docs/CALIBRATION.md) for tuning procedure.

1. Place two units exactly 1.00m apart (LOS)
2. Read distance from serial monitor
3. Adjust `ANTENNA_DELAY_TX/RX` and `DIST_OFFSET_M` in `config.h`
4. Repeat at 3.00m for verification

## Troubleshooting

**Units won't connect to Wi-Fi:**
- Verify SSID/password in `config.h`
- Check 2.4GHz network (ESP32 doesn't support 5GHz)
- Monitor serial output for error messages

**No ranging data:**
- Check DW3000 wiring (especially CS, IRQ, RST)
- Verify 3.3V power supply is stable
- Enable simulation mode to test network path

**UI shows no data:**
- Verify hub IP matches `HUB_UDP_IP` in ESP32 config
- Check firewall allows UDP on configured port
- Inspect browser console and hub logs

**Inaccurate distances:**
- Run calibration procedure
- Check for obstacles (UWB requires line-of-sight)
- Verify antenna orientation (keep flat, parallel)

## Future Extensions

**Audio Integration (hooks provided):**
- Replace simulated volume with real audio streaming
- Add I2S microphone/speaker support to ESP32
- Implement OPUS codec for compression
- Use volume coefficients from hub for mixing

**Production Hardening:**
- Custom PCB with integrated antenna
- Secure WebSocket (WSS) and TLS for REST
- Authentication and multi-hub support
- Persistent storage (PostgreSQL/InfluxDB)
- MQTT for IoT integration

**Advanced Features:**
- 3D position estimation (TDOA)
- Mesh networking (ESP-NOW)
- Mobile app (React Native)
- Analytics dashboard

## License

MIT License - see [`LICENSE`](LICENSE) file for details.

## Acknowledgments

- Qorvo/Decawave for DW3000 documentation
- ESP32 Arduino core contributors
- FastAPI and Uvicorn teams

## Support

For issues, questions, or contributions, please open an issue or pull request.

