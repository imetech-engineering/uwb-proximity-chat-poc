# Version History

All notable changes to the UWB Proximity Chat POC will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Planned
- Audio streaming integration (I2S microphone/speaker)
- Mobile app interface
- Multi-hub support with mesh networking
- Advanced 3D positioning (TDOA/PDOA)
- Persistent time-series database integration

---

## [0.1.0] - 2025-10-03

### Added - Initial POC Release

#### ESP32 Firmware
- Complete Arduino-based firmware for ESP32 + DW3000
- Double-sided two-way ranging (DS-TWR) implementation
- Configurable UWB parameters (channel, data rate, preamble, TX power)
- Wi-Fi STA mode with automatic reconnection
- UDP JSON packet transmission to hub
- Simulation mode for testing without hardware
- Comprehensive configuration via `config.h`
- Serial logging with adjustable log levels
- Peer scheduling to avoid ranging collisions
- Hardware abstraction layer for DW3000

#### Raspberry Pi Hub
- FastAPI-based backend server
- Asynchronous UDP listener for distance data ingestion
- Real-time WebSocket broadcast to connected clients
- Distance-based volume simulation (configurable model)
- RESTful API endpoints:
  - `/api/snapshot` - Current system state
  - `/api/export` - CSV data export
- YAML-based configuration (`config.yaml`)
- Structured logging with rotation support
- CORS support for web UI
- Static file serving for UI

#### Web UI
- Real-time distance visualization
- Network graph showing all nodes and connections
- Live distance, quality, and simulated volume display
- WebSocket client with auto-reconnect
- Settings panel with configuration display
- CSV export functionality
- Responsive design (mobile-friendly)
- Vanilla JavaScript (no framework dependencies)

#### Documentation
- Comprehensive README with architecture diagram
- Complete wiring guide (`WIRING.md`) with pinouts
- Calibration procedure (`CALIBRATION.md`)
- Protocol specification (`PROTOCOL.md`) with JSON schemas
- Test plan (`TESTPLAN.md`) with acceptance criteria
- Troubleshooting guides
- Quick start guides for Arduino IDE and PlatformIO

#### Configuration
- All ESP32 settings centralized in `config.h`:
  - Network credentials
  - Unit identification
  - UWB parameters
  - Calibration constants
  - Debug options
- All hub settings in `config.yaml`:
  - Network ports
  - Volume model parameters
  - UI preferences
  - Export settings
  - Security options

#### Development Tools
- `run_dev.sh` script for quick server startup
- systemd service file for production deployment
- Python requirements file with pinned versions
- Example PlatformIO configuration

### Technical Specifications

- **Ranging Method**: DS-TWR (Double-Sided Two-Way Ranging)
- **UWB Chip**: DW3000 (Qorvo/Decawave)
- **Microcontroller**: ESP32 (WROOM series)
- **Communication**: Wi-Fi 802.11 b/g/n (2.4GHz)
- **Protocol**: UDP (unit to hub), WebSocket (hub to UI)
- **Backend**: Python 3.8+, FastAPI, Uvicorn
- **Frontend**: HTML5, CSS3, ES6+ JavaScript
- **Data Format**: JSON
- **Accuracy Target**: ±10cm in line-of-sight
- **Update Rate**: 2+ Hz (configurable)
- **WebSocket Latency**: <500ms typical

### Known Limitations (POC Version)

- No actual audio processing (simulation only)
- Maximum 3 units (hardcoded peer list)
- Single hub architecture (no redundancy)
- No authentication/authorization
- In-memory state only (no persistence)
- Requires line-of-sight for best accuracy
- 2.4GHz Wi-Fi only (no 5GHz support)
- No NLOS compensation
- Basic collision avoidance (time-slotted)

### Security Considerations

- **POC-level security**: Not suitable for production deployment
- UDP packets are unauthenticated (open to spoofing)
- WebSocket connections are unencrypted (ws://, not wss://)
- No API authentication by default
- Configuration files may contain credentials in plaintext
- **Recommendation**: Deploy on isolated/trusted network only

---

## Version Numbering Scheme

- **MAJOR**: Incompatible API/protocol changes
- **MINOR**: New features, backward-compatible
- **PATCH**: Bug fixes, documentation updates

---

## Upgrade Notes

### From Nothing to 0.1.0
This is the initial release - no upgrade needed.

---

## Deprecation Notices

None for v0.1.0.

---

## Contributors

- IMeTech Engineering

---

## Release Checklist (for future versions)

- [ ] Update version number in all files
- [ ] Update VERSIONS.md with changes
- [ ] Run full test suite (see TESTPLAN.md)
- [ ] Verify builds on clean Arduino IDE and PlatformIO
- [ ] Test on fresh Raspberry Pi OS installation
- [ ] Update documentation screenshots
- [ ] Tag release in Git
- [ ] Generate release notes
- [ ] Update README badges (if any)

