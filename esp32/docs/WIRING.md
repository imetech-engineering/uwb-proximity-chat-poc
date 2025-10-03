# ESP32 + DW3000 Wiring Guide

Complete hardware connection guide for the UWB proximity chat unit.

## Safety and Prerequisites

**Before you begin:**
- Work on a non-conductive surface
- Disconnect all power before making connections
- Use a regulated 3.3V power supply (DO NOT use 5V on DW3000)
- Anti-static precautions recommended

**Required tools:**
- Solderless breadboard
- Jumper wires (male-to-male, male-to-female)
- USB cable for ESP32 programming
- Multimeter (for voltage verification)

## Hardware Bill of Materials (per unit)

| Item | Quantity | Notes |
|------|----------|-------|
| ESP32 DevKit (WROOM-32) | 1 | Any ESP32 dev board with VSPI pins |
| DW3000 UWB Module | 1 | Qorvo DWM3000 or compatible |
| Breadboard (400 or 830 point) | 1 | Half or full size |
| Jumper wires | 10+ | Various colors for clarity |
| USB cable (micro or USB-C) | 1 | Depends on ESP32 board |
| (Optional) Logic level analyzer | 1 | For debugging SPI |

## Pin Mapping

### DW3000 ↔ ESP32 Connections

The DW3000 communicates via SPI (VSPI on ESP32).

| DW3000 Pin | Signal | ESP32 Pin | Notes |
|------------|--------|-----------|-------|
| VDD | Power 3.3V | 3V3 | **CRITICAL: 3.3V only!** |
| GND | Ground | GND | Multiple GND connections recommended |
| SCLK | SPI Clock | GPIO 18 | VSPI SCK |
| MOSI | SPI Master Out | GPIO 23 | VSPI MOSI |
| MISO | SPI Master In | GPIO 19 | VSPI MISO |
| CS/SS | Chip Select | GPIO 5 | Configurable in config.h |
| IRQ | Interrupt Request | GPIO 17 | Configurable in config.h |
| RST | Reset | GPIO 27 | Configurable in config.h |
| WAKEUP | Wake from sleep | 3V3 | Tie HIGH (not using sleep) |
| EXTON | External TXCO | GND | Tie LOW (using internal oscillator) |

### ESP32 Pinout Reference

```
                      ESP32 DEVKIT V1
                    ┌─────────────────┐
                    │     USB Port    │
                    └─────────────────┘
                            │
        ┌───────────────────┴───────────────────┐
        │                                       │
    3V3 │●  (Power 3.3V)                    GND │● (Ground)
    EN  │●                                   23 │● (VSPI MOSI)
   VP36 │●                                   22 │●
   VN39 │●                                    1 │● (TX)
   IO34 │●                                    3 │● (RX)
   IO35 │●                                   21 │●
   IO32 │●                                   19 │● (VSPI MISO)
   IO33 │●                                   18 │● (VSPI SCK)
   IO25 │●                                    5 │● (VSPI CS) → DW3000 CS
   IO26 │●                                   17 │● → DW3000 IRQ
   IO27 │●  → DW3000 RST                    16 │●
   IO14 │●                                    4 │●
   IO12 │●                                    2 │● (Built-in LED)
        │                                    15 │●
    GND │●  (Ground)                        GND │● (Ground)
    VIN │●  (5V input - DO NOT use for DW)    │
        └───────────────────────────────────────┘
```

## Step-by-Step Wiring

### Step 1: Power Verification

**CRITICAL**: Verify 3.3V rail before connecting DW3000!

1. Connect ESP32 to USB power
2. Use multimeter to measure voltage between ESP32 3V3 and GND pins
3. Verify reading is **3.2-3.4V** (should be ~3.3V)
4. If voltage is outside range, DO NOT proceed

### Step 2: Ground Connections

Good grounding is essential for reliable operation.

1. Connect ESP32 GND to breadboard ground rail
2. Use multiple ground connections (DW3000 has multiple GND pins - use them all)
3. Keep ground paths short and direct

### Step 3: Power Connection

1. Connect ESP32 **3V3** pin to breadboard power rail
2. Connect DW3000 **VDD** to breadboard power rail (3.3V)
3. Connect DW3000 **GND** to breadboard ground rail
4. Add 10µF capacitor between VDD and GND near DW3000 (noise filtering)
5. Add 100nF ceramic capacitor between VDD and GND near DW3000 (decoupling)

### Step 4: SPI Connections

Connect SPI bus using short, direct wires:

1. ESP32 GPIO 18 → DW3000 SCLK (blue wire)
2. ESP32 GPIO 23 → DW3000 MOSI (green wire)
3. ESP32 GPIO 19 → DW3000 MISO (yellow wire)
4. ESP32 GPIO 5 → DW3000 CS (orange wire)

**Tips:**
- Use color-coded wires for easier debugging
- Keep wires short (<10cm if possible)
- Avoid running SPI wires parallel to power wires

### Step 5: Control Signals

1. ESP32 GPIO 17 → DW3000 IRQ (white wire)
2. ESP32 GPIO 27 → DW3000 RST (red wire)

### Step 6: DW3000 Configuration Pins

These pins configure the DW3000 operating mode:

1. DW3000 WAKEUP → 3.3V (tie HIGH - active mode)
2. DW3000 EXTON → GND (tie LOW - use internal oscillator)

### Step 7: Antenna Connection

**CRITICAL**: UWB module MUST have antenna connected before powering on!

1. If using external antenna: connect to DW3000 RF port
2. If using PCB antenna: ensure nothing metallic is within 5cm
3. Orient antenna away from ESP32 metal shield (if present)

## 📸 Breadboard Layout

```
                    Breadboard Layout (Top View)
    
    Power Rails         Main Area                    Ground Rails
    ═══════════    ═════════════════════════    ═══════════
    + (3.3V)                                    - (GND)
    ═══════════    ═════════════════════════    ═══════════
         │                                            │
         │         ┌───────────────────┐              │
         ├─────────┤ DW3000 Module     ├──────────────┤
         │         │                   │              │
         │         │  VDD ●      ● GND │              │
         │         │  SCK ●      ● IRQ │──┐           │
         │         │ MOSI ●      ● RST │──┤           │
         │         │ MISO ●      ● CS  │  │           │
         │         │   ...             │  │           │
         │         │  [Antenna]        │  │           │
         │         └───────────────────┘  │           │
         │               │ │ │ │          │ │         │
         │               │ │ │ │          │ │         │
         │         ┌─────┴─┴─┴─┴──────────┴─┴───┐     │
         │         │                             │     │
         │         │    ESP32 DevKit             │     │
         │         │                             │     │
         └─────────┤ 3V3              GPIO 5 ●───┘ (CS)│
                   │ GND ●────────────────────────────┼┘
                   │                GPIO 17 ●─────────┘ (IRQ)
                   │                GPIO 18 ●─────────── (SCK)
                   │                GPIO 19 ●─────────── (MISO)
                   │                GPIO 23 ●─────────── (MOSI)
                   │                GPIO 27 ●─────────── (RST)
                   │                                     │
                   │         [USB Port]                  │
                   └─────────────────────────────────────┘
```

## ⚡ Power Considerations

### Current Requirements

- ESP32: ~80-260mA (depending on Wi-Fi activity)
- DW3000: ~100-150mA (during TX burst)
- **Peak total**: ~400mA
- USB provides up to 500mA, so should be sufficient

### Power Supply Quality

**Important**: DW3000 is sensitive to power supply noise!

1. **Decoupling capacitors** (essential):
   - 10µF electrolytic near DW3000 VDD (bulk storage)
   - 100nF ceramic near DW3000 VDD (high-frequency noise)
   - 100nF ceramic near ESP32 3V3 pin

2. **Ground plane**: Use breadboard ground rail generously

3. **Avoid**: 
   - Long power wires
   - Sharing power rails with motors or relays
   - USB hubs with poor power regulation

## 🔧 Antenna Considerations

### Antenna Types

1. **PCB Trace Antenna** (built into some modules):
   - No external connection needed
   - Keep clear area around antenna (5cm+ radius)
   - Orient away from metal objects

2. **External Antenna** (SMA or U.FL connector):
   - Connect before applying power
   - 50Ω impedance matched
   - Keep cable <30cm for best performance

### Antenna Placement

For best ranging accuracy:
- Mount antenna horizontally (parallel to ground)
- Keep antenna at same height on all units
- Orient all antennas the same direction
- Maintain line-of-sight between units
- Avoid metal objects within 10cm

### Polarization

UWB antennas are typically linearly polarized. For best results:
- All units should have same antenna orientation
- Rotating a unit 90° can reduce signal strength significantly

## 🧪 Testing Your Connections

### Visual Inspection Checklist

Before applying power:
- [ ] All GND connections secure
- [ ] VDD connected to 3.3V (NOT 5V!)
- [ ] No loose wires
- [ ] No short circuits (check with multimeter continuity mode)
- [ ] Antenna connected (if using external antenna)
- [ ] Capacitors in place

### Power-On Test

1. **Connect ESP32 to USB**
2. **Check LED**: ESP32 power LED should illuminate
3. **Measure voltages** with multimeter:
   - ESP32 3V3 pin: 3.2-3.4V
   - DW3000 VDD: 3.2-3.4V
   - All GND pins: 0V
4. **Upload test sketch**: Use Arduino IDE to upload blink sketch
   - If upload fails, check USB cable and drivers
5. **Upload UWB firmware**: Flash main.cpp
6. **Open Serial Monitor** (115200 baud):
   - Should see initialization messages
   - Look for "DW3000 initialized" or simulation mode message

### SPI Bus Test (Advanced)

If you have logic analyzer:
1. Connect probes to SCK, MOSI, MISO, CS
2. Monitor during ESP32 boot
3. Should see SPI transactions when firmware tries to read DW3000 ID
4. Clock frequency should be ~8MHz
5. CS should pulse LOW during transactions

## 🐛 Troubleshooting

### Problem: ESP32 won't boot

**Symptoms**: No serial output, no LED

**Solutions**:
- Check USB cable (try different cable)
- Check power LED on ESP32
- Try different USB port
- Press BOOT button while connecting USB
- Check EN pin is not pulled LOW

### Problem: "DW3000 initialization failed"

**Symptoms**: Serial shows error message, firmware continues in simulation mode

**Solutions**:
1. Check power (3.3V on DW3000 VDD)
2. Verify SPI connections (especially CS pin)
3. Check RST pin connection
4. Verify DW3000 is not in reset (RST should be HIGH)
5. Try re-seating DW3000 in breadboard
6. Verify correct SPI pins (VSPI on ESP32)

### Problem: Ranging gives wildly incorrect distances

**Symptoms**: Distances are 10x too large/small, or random

**Solutions**:
- Run calibration procedure (see CALIBRATION.md)
- Check antenna is connected and properly oriented
- Verify all units have same UWB channel configured
- Check for metal objects near antennas
- Ensure line-of-sight between units
- Verify ANTENNA_DELAY values in config.h

### Problem: Intermittent ranging failures

**Symptoms**: Ranging works sometimes, fails other times

**Solutions**:
- Add/improve decoupling capacitors
- Shorten wire connections
- Check for loose connections
- Verify power supply stability (measure with oscilloscope if possible)
- Check Wi-Fi and UWB are not interfering (try different UWB channel)
- Reduce TX power if units are very close (<50cm)

### Problem: No Wi-Fi connection

**Symptoms**: "Wi-Fi connection timeout" in serial

**Solutions**:
- Verify SSID and password in config.h
- Check 2.4GHz network is available (ESP32 doesn't support 5GHz)
- Move closer to Wi-Fi router
- Check router allows new clients
- Try different Wi-Fi channel (avoid channel overlap with UWB)

## 📏 Multi-Unit Setup

When building 3 units:

1. **Label everything**: Mark each ESP32 and DW3000 as Unit A/B/C
2. **Flash separately**: Change UNIT_ID in config.h before flashing each
3. **Identical wiring**: Use same pin assignments on all units
4. **Antenna orientation**: Keep all antennas oriented the same way
5. **Spacing**: Start with units ~2m apart for initial testing

## 🔐 Best Practices

1. **Strain relief**: Secure USB cable to prevent tugging on ESP32
2. **Breadboard quality**: Use good quality breadboard with tight contacts
3. **Wire management**: Keep wires neat and labeled
4. **Documentation**: Take photos of your working setup
5. **Backup**: Keep spare ESP32 and DW3000 modules if possible

## 📚 Additional Resources

- [ESP32 Pinout Reference](https://randomnerdtutorials.com/esp32-pinout-reference-gpios/)
- [DW3000 Datasheet](https://www.qorvo.com/products/p/DW3000)
- [SPI Protocol Tutorial](https://learn.sparkfun.com/tutorials/serial-peripheral-interface-spi)

## ⚠️ Important Notes

1. **Never apply 5V to DW3000** - permanent damage will occur
2. **Always connect antenna before powering DW3000** - can damage RF frontend
3. **ESD sensitive** - DW3000 can be damaged by static electricity
4. **RF exposure** - UWB uses low power, but avoid prolonged exposure at close range

---

**Questions or issues?** Check TESTPLAN.md for validation procedures and troubleshooting.

