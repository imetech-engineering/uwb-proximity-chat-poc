# Distance Measurement Calibration Guide

This guide shows you how to make your UWB system measure distances accurately.

---

## Why Calibrate?

Your UWB system measures distance by timing radio signals. However, several factors can make measurements slightly off:

- Internal delays in the circuits
- Antenna characteristics
- Environmental effects

Without calibration: Distances might be off by 10-50 cm  
With calibration: You can achieve accuracy within 5-10 cm

---

## When to Calibrate

Calibrate in these situations:
- After first hardware setup
- If measurements seem consistently wrong
- For best accuracy in your specific environment

You can skip calibration if:
- You only need rough distances (±30cm is acceptable)
- You're just testing the system

---

## What You Need

Equipment:
- 2 Makerfabs ESP32 UWB boards (already programmed and working)
- Metal tape measure (at least 3 meters long)
- Flat, open space (hallway or large room)
- Tape or chalk to mark positions
- Laptop with Arduino IDE

Environment:
- Indoor location
- Clear line-of-sight (no obstacles between units)
- No large metal objects nearby
- Flat surface (floor or table)

Time needed: About 20 minutes

---

## Quick Calibration Procedure

### Step 1: Setup

1. Make sure both ESP32 units are working and sending data
2. Open Serial Monitor on one unit (115200 baud)
3. Make sure the Raspberry Pi is collecting data

### Step 2: Measure at 1 Meter

1. Place two units exactly **1.00 meter apart**
   - Measure carefully with your tape measure
   - Place on floor or table
   - Mark positions with tape

2. Let them measure for 60 seconds

3. Watch the serial monitor and write down the distances shown
   - Example readings: 0.95m, 0.94m, 0.96m, 0.95m...
   - Calculate average: (0.95+0.94+0.96+0.95)/4 = **0.95m**

4. Calculate the error:
   ```
   Error = True Distance - Measured Average
   Error = 1.00m - 0.95m = 0.05m (5cm too short)
   ```

### Step 3: Verify at 3 Meters

1. Place units exactly **3.00 meters apart**

2. Let them measure for 60 seconds

3. Write down the average measurement
   - Example: 2.88m measured

4. Calculate error:
   ```
   Error = 3.00m - 2.88m = 0.12m (12cm too short)
   ```

### Step 4: Apply Correction

Since both measurements are short, we need to add an offset.

1. Calculate average error:
   ```
   Average = (0.05m + 0.12m) / 2 = 0.085m
   ```

2. Open `esp32/unit_firmware/config.h`

3. Find this line:
   ```cpp
   #define DIST_OFFSET_M 0.00
   ```

4. Change it to:
   ```cpp
   #define DIST_OFFSET_M 0.085  // Adds 8.5cm to measurements
   ```

5. Re-flash all your ESP32 units with the updated code

### Step 5: Verify It Worked

1. Place units at 1 meter again
2. Check measurement - should now show close to 1.00m
3. Place units at 3 meters
4. Check measurement - should now show close to 3.00m

Done. Your system is now calibrated.

---

## Expected Results

### Before Calibration
| True Distance | Measured | Error |
|---------------|----------|-------|
| 1.00m | 0.95m | -5cm |
| 3.00m | 2.88m | -12cm |

### After Calibration
| True Distance | Measured | Error |
|---------------|----------|-------|
| 1.00m | 0.99m | -1cm (Good) |
| 3.00m | 2.98m | -2cm (Good) |

---

## Tips for Best Results

**Positioning:**
- Place units on same height (both on floor or both on table)
- Keep antennas vertical
- Point antennas in same direction
- Don't cover antennas with your hand

**Environment:**
- Use open space without obstacles
- Keep away from large metal objects
- No people walking between units during measurement
- Stable temperature (not near heating/cooling)

**Measurements:**
- Always use center-to-center distance
- Mark positions clearly
- Take at least 20-30 readings
- Ignore first few readings (warmup)

---

## Common Issues

### Problem: Measurements Jump Around a Lot

**Symptoms:** Values constantly changing by 10+ cm

**Solutions:**
- Move away from metal objects
- Ensure clear line-of-sight
- Check antennas are stable (not moving/vibrating)
- Reduce movement in the area

### Problem: Measurements Are Way Off

**Symptoms:** Shows 5m when actually 1m apart

**Solutions:**
- Check antenna is connected
- Verify units are properly configured
- Try different location
- Check hardware connections

### Problem: One Direction Different Than Other

**Symptoms:** A→B shows 1.0m but B→A shows 1.2m

**Solutions:**
- Individual units may need separate calibration
- Check both units have same settings
- Verify antenna orientations match

---

## Advanced: Per-Unit Calibration

If your units give different results, you can calibrate each one individually:

1. Label units clearly (A, B, C)

2. In `config.h`, use conditional settings:
   ```cpp
   #if UNIT_ID == 'A'
     #define DIST_OFFSET_M 0.08
   #elif UNIT_ID == 'B'
     #define DIST_OFFSET_M 0.10
   #elif UNIT_ID == 'C'
     #define DIST_OFFSET_M 0.09
   #endif
   ```

3. Flash each unit with its own calibration value

---

## Record Your Calibration

Keep a record for future reference:

```
CALIBRATION RECORD
==================
Date: _____________
Unit ID: __________

Before:
  @1.00m measured: _______m
  @3.00m measured: _______m

After:
  DIST_OFFSET_M = _______
  @1.00m measured: _______m ✅
  @3.00m measured: _______m ✅

Location: _________________
Notes: ____________________
```

---

## Still Having Issues?

1. Verify firmware uploaded correctly
2. Check serial monitor for error messages
3. Try different location
4. Test with simulation mode first
5. Verify USB cable and power supply

---

With calibration complete, your system should now give accurate distance measurements.
