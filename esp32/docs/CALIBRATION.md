# DW3000 Ranging Calibration Guide

This guide explains how to calibrate the UWB ranging system for accurate distance measurements.

## 📐 Why Calibration Is Needed

UWB ranging measures time-of-flight (ToF) of radio signals to calculate distance. However, several factors introduce systematic errors:

1. **Antenna delay**: Time for signal to travel through PCB traces and antenna
2. **Group delay**: Frequency-dependent delay in filters and amplifiers  
3. **Cable delay**: If using external antenna with cable
4. **Environment**: Reflections, temperature, humidity

**Without calibration**, distances may be off by 10-50cm or more.

**With proper calibration**, accuracy of ±5-10cm is achievable.

## 🎯 Calibration Goals

- **Primary goal**: Accurate distance measurements at 1-5m range
- **Acceptable error**: ±10cm in line-of-sight conditions
- **Method**: Two-point calibration (1m and 3m reference distances)

## 📋 Prerequisites

### Hardware Required

- 2× calibrated ESP32+DW3000 units (minimum)
- Tape measure (metal, ≥3m length, 1mm precision)
- Level surface (floor, table, hallway)
- Markers (tape or chalk to mark positions)
- Laptop with serial monitor

### Software Required

- Firmware uploaded to both units (different UNIT_IDs)
- Hub running (to receive measurements)
- Serial monitor (Arduino IDE, PuTTY, or similar)

### Environmental Requirements

- **Indoor**, large open space (5m × 5m minimum)
- **Line-of-sight** between units (no obstacles)
- **No metal objects** nearby (3m clearance)
- Stable temperature (avoid HVAC vents, direct sunlight)

## 🔧 Calibration Procedure

### Step 1: Initial Setup

1. **Flash firmware** with default settings:
   ```cpp
   // In config.h:
   #define ANTENNA_DELAY_TX  16450
   #define ANTENNA_DELAY_RX  16450
   #define DIST_OFFSET_M     0.00
   ```

2. **Label units**: Use Unit A and Unit B for calibration

3. **Start hub**: Ensure Raspberry Pi hub is running and receiving data

4. **Position units**:
   - Place both units on same surface (e.g., floor)
   - Align antennas horizontally
   - Orient antennas in same direction

### Step 2: Baseline Measurement (Close Range)

**Goal**: Measure error at very short distance to isolate antenna delay.

1. **Position units**: Place exactly **50cm apart** (measure center-to-center)
   - Use tape measure
   - Mark positions with tape
   - Ensure antennas are aligned

2. **Collect data**:
   - Open serial monitor on both units
   - Let ranging run for 30 seconds
   - Note distance readings from serial output

3. **Calculate average**:
   - Discard first 5 readings (warmup)
   - Average next 20 readings
   - Example: If readings are 0.48, 0.49, 0.48, 0.47... → Average = 0.48m

4. **Record results**:
   ```
   True distance:    0.50m
   Measured avg:     0.48m
   Error:            -0.02m (20mm too short)
   ```

### Step 3: Reference Measurement (1 meter)

**This is the primary calibration point.**

1. **Position units**: Place exactly **1.00m apart**
   - Measure carefully (±5mm accuracy)
   - Mark positions clearly
   - Use level surface

2. **Collect data**:
   - Let ranging run for 60 seconds
   - Monitor both serial outputs
   - Check for quality >0.7 (good signal)

3. **Calculate statistics**:
   - Mean distance
   - Standard deviation (should be <5cm)
   - Min/Max values

4. **Record results**:
   ```
   True distance:    1.00m
   Measured avg:     0.95m
   Std dev:          0.03m
   Error:            -0.05m (50mm too short)
   ```

### Step 4: Verification Measurement (3 meters)

**This verifies calibration across range.**

1. **Position units**: Place exactly **3.00m apart**
   - Use full tape measure length
   - Double-check measurement
   - Ensure line-of-sight

2. **Collect data**:
   - Let ranging run for 60 seconds
   - Note any quality degradation with distance

3. **Record results**:
   ```
   True distance:    3.00m
   Measured avg:     2.88m
   Std dev:          0.05m
   Error:            -0.12m (120mm too short)
   ```

### Step 5: Calculate Calibration Constants

Now we use the measurements to compute correction factors.

#### Method 1: Distance Offset (Simple)

If error is consistent across distances, use offset correction:

```
DIST_OFFSET_M = (True - Measured)
```

**Example**:
- At 1m: Error = -0.05m
- At 3m: Error = -0.12m
- Average error: -0.085m

**Update config.h**:
```cpp
#define DIST_OFFSET_M  0.085  // Adds 8.5cm to all measurements
```

#### Method 2: Antenna Delay (Advanced)

For more accurate calibration, adjust antenna delay. This affects the ToF calculation directly.

**Formula**:
```
Antenna Delay Adjustment = (Error in meters × 499.2e6 × 128) / 3e8
```

**Example** (error = -0.05m at 1m):
```
Adjustment = (-0.05 × 499.2e6 × 128) / 3e8
           = -10.65 time units
           ≈ -11 units
```

**Update config.h**:
```cpp
// If starting value was 16450:
#define ANTENNA_DELAY_TX  16439  // Decreased by 11
#define ANTENNA_DELAY_RX  16439  // Match TX value
```

**Note**: TX and RX delays should typically be the same for symmetric transceivers.

### Step 6: Apply Calibration

1. **Edit config.h** with new values
2. **Reflash both units**
3. **Repeat measurements** at 1m and 3m
4. **Verify improvement**:
   - Error should be <±2cm at 1m
   - Error should be <±5cm at 3m

### Step 7: Cross-Check

Test with different unit combinations:

1. **A ↔ B**: Already calibrated
2. **A ↔ C**: Apply same calibration to Unit C, test
3. **B ↔ C**: Should give consistent results

If results vary significantly between pairs, individual calibration per unit may be needed (advanced).

## 📊 Expected Results

### Before Calibration

| True Distance | Measured | Error | Std Dev |
|---------------|----------|-------|---------|
| 0.50m | 0.45m | -0.05m | 0.02m |
| 1.00m | 0.95m | -0.05m | 0.03m |
| 3.00m | 2.88m | -0.12m | 0.05m |

### After Calibration (Good)

| True Distance | Measured | Error | Std Dev |
|---------------|----------|-------|---------|
| 0.50m | 0.51m | +0.01m | 0.02m |
| 1.00m | 0.99m | -0.01m | 0.03m |
| 3.00m | 2.98m | -0.02m | 0.04m |

### After Calibration (Excellent)

| True Distance | Measured | Error | Std Dev |
|---------------|----------|-------|---------|
| 0.50m | 0.50m | 0.00m | 0.01m |
| 1.00m | 1.00m | 0.00m | 0.02m |
| 3.00m | 3.01m | +0.01m | 0.03m |

## 🔬 Advanced Calibration

### Multi-Point Calibration

For best accuracy across wide range:

1. Measure at: 0.5m, 1m, 2m, 3m, 4m, 5m
2. Plot measured vs. true distance
3. Fit linear regression: `measured = a × true + b`
4. Derive correction: `corrected = (measured - b) / a`
5. Implement correction in firmware

### Temperature Compensation

UWB ranging can drift with temperature changes.

**Procedure**:
1. Calibrate at room temperature (20-25°C)
2. Measure at cold temperature (10°C)
3. Measure at warm temperature (30°C)
4. Calculate temperature coefficient (cm/°C)
5. Implement compensation in firmware using onboard temperature sensor

**Typical drift**: ~1-2 cm per 10°C

### Per-Unit Calibration

If units show significant variation:

1. Create calibration table for each unit
2. Store in config.h or EEPROM:
   ```cpp
   #if UNIT_ID == 'A'
     #define ANTENNA_DELAY_TX 16440
   #elif UNIT_ID == 'B'
     #define ANTENNA_DELAY_TX 16455
   #elif UNIT_ID == 'C'
     #define ANTENNA_DELAY_TX 16448
   #endif
   ```

## 🐛 Troubleshooting

### Problem: Large Standard Deviation (>10cm)

**Symptoms**: Measurements jump around wildly

**Solutions**:
- Check for nearby metal objects (move them away)
- Verify antennas are stable (not vibrating)
- Ensure line-of-sight (no obstacles)
- Check power supply quality (add capacitors)
- Reduce environmental motion (people walking, doors closing)

### Problem: Non-Linear Error

**Symptoms**: Calibration works at 1m but not at 3m

**Solutions**:
- May indicate multipath interference (reflections)
- Try different location
- Adjust antenna orientation
- Use multi-point calibration
- Check for systematic obstacles

### Problem: Asymmetric Measurements

**Symptoms**: A→B distance ≠ B→A distance

**Solutions**:
- Units may need individual calibration
- Check antenna delay settings (should be symmetric)
- Verify both units have same UWB config (channel, PRF, etc.)
- Check for hardware differences (different antenna types)

### Problem: Cannot Achieve <10cm Accuracy

**Symptoms**: Error remains >10cm after calibration

**Possible causes**:
- Poor quality breadboard connections (use soldered protoboard)
- Antenna issues (damaged, wrong impedance, poor matching)
- Clock drift (DW3000 crystal oscillator tolerance)
- Environmental factors (metal, moisture, multipath)

**Solutions**:
- Try simulation mode to verify hub/network path
- Test with different DW3000 modules
- Use spectrum analyzer to check UWB signal quality
- Consider custom PCB with proper impedance matching

## 📝 Calibration Record Template

Keep a log of calibration for each unit:

```
CALIBRATION RECORD
==================
Date: 2025-10-03
Unit ID: A
Hardware: ESP32 DevKit v1 + DWM3000 EVB
Antenna: PCB trace antenna

Before Calibration:
  Config: ANTENNA_DELAY_TX=16450, RX=16450, OFFSET=0.00
  @1.00m: Measured=0.95m, Error=-0.05m, StdDev=0.03m
  @3.00m: Measured=2.88m, Error=-0.12m, StdDev=0.05m

After Calibration:
  Config: ANTENNA_DELAY_TX=16450, RX=16450, OFFSET=0.085
  @1.00m: Measured=1.00m, Error=0.00m, StdDev=0.03m
  @3.00m: Measured=3.01m, Error=+0.01m, StdDev=0.04m

Notes: Calibration performed in hallway, concrete floor.
       Temperature: 22°C, Humidity: 45%

Verified by: [Your Name]
```

## 🎓 Understanding the Numbers

### Antenna Delay Units

DW3000 uses internal time units of approximately **15.65 picoseconds** each.

- Time unit = 1 / (499.2 MHz × 128) ≈ 15.65 ps
- 1 meter = 6.67 ns (time for light to travel)
- 1 meter ≈ 426 time units

**Typical values**:
- Antenna delay: 16000-17000 units (corresponds to ~1.5m equivalent delay)
- This doesn't mean antenna adds 1.5m distance, it's internal circuit delay

### Quality Factor

Quality indicates confidence in measurement:
- **>0.9**: Excellent (strong signal, low noise)
- **0.7-0.9**: Good (reliable for calibration)
- **0.5-0.7**: Fair (usable but may have more variance)
- **<0.5**: Poor (consider discarding)

Low quality can result from:
- Weak signal (units too far apart)
- Interference (Wi-Fi, other UWB devices)
- Multipath (reflections)
- Antenna misalignment

## ✅ Calibration Checklist

- [ ] Hardware assembled and tested
- [ ] Firmware flashed with default calibration values
- [ ] Quiet environment with no obstacles
- [ ] Accurate tape measure (±5mm)
- [ ] Measurements at 0.5m, 1m, 3m recorded
- [ ] Calibration constants calculated
- [ ] config.h updated with new values
- [ ] Firmware reflashed to all units
- [ ] Verification measurements show <±5cm error
- [ ] Calibration record saved

## 📚 References

- DW3000 User Manual, Section 4.3 (Antenna Calibration)
- IEEE 802.15.4z Standard (UWB PHY)
- [Qorvo Application Note APS014](https://www.qorvo.com) - Antenna Delay Calibration

---

**Next steps**: After calibration, proceed to TESTPLAN.md for full system validation.

