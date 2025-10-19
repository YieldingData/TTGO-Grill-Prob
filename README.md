# TTGO Grill Thermometer (Meater+ Edition)

## Overview
The **TTGO Grill Thermometer** is a Wi‑Fi + Bluetooth LE BBQ monitor built on the ESP32‑based **TTGO T‑Display**.  
It reads temperature from a **Meater+ wireless probe** over BLE, shows live status on the TFT, and syncs time/weather for an easy, stand‑alone cook station.

**Highlights**
- Meater+ probe over **BLE (NimBLE‑Arduino)**
- Clean **Top Bar**: City • Bat XX% • Time • Weather icon
- Filtered battery % and stable UI refresh
- Modular `.ino` files (clock, weather, top bar, probe)
- Designed to run off a single Li‑ion/LiPo cell

---

## Hardware

| Component | Description | Notes |
|---|---|---|
| **TTGO T‑Display ESP32** | ESP32 + 1.14″ TFT display | Main MCU, Wi‑Fi, BLE |
| **Meater+ probe** | Wireless meat + ambient sensor | BLE to ESP32 (no phone required) |
| **18650/LiPo + TP4056** | Power + charging | 3.7 V nominal |
| **Voltage Divider** | Battery monitor to ADC | For % readout |
| **(Optional) Buck 5 V** | Auxiliary 5 V rail | Only if needed for extras |
| **Connectors/Wire** | JST/headers | As needed |

> The Meater+ “+” model includes a BLE repeater in the charger block for extended range. The ESP32 reads the probe via BLE GATT.

---

## Repo Layout / Modules

```
TTGO_Grill_Thermometer/
├─ user_interface_v3.ino   # Main sketch (app glue + screens)
├─ TopBar.ino              # Time • City • Weather • Bat XX%
├─ ClockModule.ino         # NTP + timezone handling
├─ WeatherModule.ino       # OpenWeather fetch + icon
├─ GrillProb.ino           # Meater+ BLE probe reader (temps)
├─ Config.h                # Pins, Wi‑Fi, API keys, constants
└─ README.md
```

**Probe module note**: `GrillProb.ino` is the Meater+ interface. It uses NimBLE for scanning, connecting, and reading temps from the probe’s GATT characteristics.

---

## Features

- **Meater+ BLE**: Connects directly from ESP32 to Meater+ (charger relay) and parses probe temps (meat & ambient).  
- **Top Bar**: City, filtered **Bat XX%**, time, weather icon (rain/clear/etc).  
- **Time & Weather**: OpenWeather API for timezone + current weather; NTP for wall‑clock.  
- **Battery Smoothing**: Configurable exponential or moving average to avoid jumpy % readouts.  
- **Buttons**: Debounced click/long‑press for UI navigation/refresh.  
- **Modular**: Each concern in its own file for easier hacking.

---

## Prerequisites

**Arduino IDE / ESP32 core**
- ESP32 board package (tested with `esp32 2.0.x`)
- Board: `ESP32 Dev Module` or `TTGO T‑Display`

**Libraries**
- **TFT_eSPI** (Bodmer) — configured for TTGO T‑Display
- **ArduinoJson**
- **HTTPClient** (bundled with ESP32 core)
- **WiFi** (bundled with ESP32 core)
- **NTPClient**
- **NimBLE-Arduino** (for Meater+ BLE)

> Tip: Prefer **NimBLE‑Arduino** over the classic `BLEDevice` to reduce RAM/flash usage.

---

## Configuration (`Config.h`)

```cpp
// Wi‑Fi / Weather
#define WIFI_SSID            "YourNetwork"
#define WIFI_PASSWORD        "YourPassword"
#define OPENWEATHER_API_KEY  "YourKey"
#define ZIP_CODE             "12345"
#define COUNTRY_CODE         "us"

// Battery / Pins
#define BATTERY_PIN          34          // ADC pin for voltage divider
#define BAT_VDIV_R1          200000.0f   // ohms (to battery+)
#define BAT_VDIV_R2          100000.0f   // ohms (to GND)
#define BAT_SAMPLES          8
#define BAT_ALPHA            0.2f        // 0..1, higher = snappier

// Buttons / UI
#define DEBOUNCE_MS          25
#define LONGPRESS_MS         600

// Meater+ / BLE
#define MEATER_TARGET_MAC    ""          // Optional: lock to a known probe MAC
#define MEATER_CONNECT_TIMEOUT_MS  8000
#define MEATER_READ_PERIOD_MS      1000  // GATT read/notify cadence
```

**How to get your Meater+ MAC**  
- Boot once with `MEATER_TARGET_MAC` empty to **scan**.  
- Watch the Serial Monitor for the detected **MAC address** and paste it here to bond to that device next time.

---

## Building & Uploading

1. **Clone**
   ```bash
   git clone https://github.com/YieldingData/TTGO_Grill_Thermometer.git
   ```
2. **Select board**: *ESP32 Dev Module* (or *TTGO T‑Display*).  
3. **Install libs** listed above.  
4. **Edit `Config.h`** with your Wi‑Fi, API key, and (optionally) the Meater+ MAC.  
5. **Upload** and open **Serial Monitor** (115200) for logs.

---

## Using the Meater+ Probe

1. Wake the Meater+ (remove from charger; ensure charger block has power for extended range).  
2. The ESP32 scans for the probe. If `MEATER_TARGET_MAC` is set, it will connect to that one.  
3. On success, the **Top Bar** shows normal status; your main UI shows **meat** and **ambient** temps from the probe.  
4. If the probe sleeps or goes out of range, the module will drop and re‑scan per its retry logic.

> The ESP32 reads the GATT characteristics exposed by the Meater+ relay (charger block). Exact UUIDs vary by firmware; the module handles discovery and value parsing internally.

---

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---|---|---|
| **math.h “unterminated argument list invoking macro ‘GRILL_LOG’”** | Name collision | Rename/`#undef GRILL_LOG` before including `<math.h>` or move your log macro after system headers. |
| **`invalid conversion from 'const NimBLEAdvertisedDevice*'`** | NimBLE API types differ | Use `const NimBLEAdvertisedDevice* dev = results.getDevice(i);` or avoid storing a non‑const pointer. |
| **Probe not found** | MAC mismatch / charger sleeping | Verify charger has power, scan with empty `MEATER_TARGET_MAC`, move closer (BLE range), re‑seat probe. |
| **Temps stale** | GATT notifications disabled or long interval | Ensure read/notify timer (`MEATER_READ_PERIOD_MS`) is active; check log for connect/notify status. |
| **Battery % jumps** | ADC noise | Increase `BAT_SAMPLES` or lower/higher `BAT_ALPHA` (0.1–0.3 works well). |
| **Time/weather wrong** | ZIP or API key issue | Confirm `ZIP_CODE`, `COUNTRY_CODE`, and `OPENWEATHER_API_KEY`; verify Wi‑Fi. |

---

## Roadmap

- Data logging (SPIFFS/SD) for cook sessions  
- BLE reconnection back‑off + on‑screen status toast  
- On‑device alarm thresholds (buzzer/VIB/LED)  
- Simple web UI (ESP32 captive/soft‑AP) for setup  
- Cloud dashboard exporter

---

## License
MIT — do what you like, just keep the notice.

## Credits
- Hardware & build: **Tristin Skinner (Ingot Works)**  
- BLE stack: **NimBLE‑Arduino**  
- Display: **TFT_eSPI**  
- Weather: **OpenWeatherMap**


# TTGO Grill Thermometer (Meater+ • ESP32 Bluedroid Edition)

## Important Note
Some TTGO T‑Display builds (ESP32 board package 2.0.x + TFT stack) can be finicky with **NimBLE‑Arduino**.  
This edition uses the **ESP32 BLE Arduino (Bluedroid)** stack instead (`#include <BLEDevice.h>`), which is known to be more forgiving with this board/display combo.

---

## Overview
The **TTGO Grill Thermometer** is a Wi‑Fi + BLE BBQ monitor built on the **TTGO T‑Display (ESP32)**.  
It connects to a **Meater+** probe via BLE GATT (using Bluedroid), shows live temps on the TFT, and syncs time & weather.

**Highlights**
- Meater+ probe via **ESP32 BLE Arduino (Bluedroid)**
- Clean **Top Bar**: City • Bat XX% • Time • Weather icon
- Filtered battery %
- Modular `.ino` files (clock, weather, top bar, probe)

---

## Hardware

| Component | Description | Notes |
|---|---|---|
| **TTGO T‑Display ESP32** | ESP32 + 1.14″ TFT display | Main MCU, Wi‑Fi, BLE |
| **Meater+ probe** | Wireless meat + ambient | BLE to ESP32 (via the Meater+ charger relay) |
| **18650/LiPo + TP4056** | Power + charging | 3.7 V nominal |
| **Voltage Divider** | Battery monitor → ADC | For % readout |
| **Connectors/Wire** | JST/headers | As needed |

> The Meater+ charger acts as a BLE relay with better range. The ESP32 connects to that relay over BLE GATT.

---

## Repo Layout / Modules

```
TTGO_Grill_Thermometer/
├─ user_interface_v3.ino   # Main sketch (screens + glue)
├─ TopBar.ino              # Time • City • Weather • Bat XX%
├─ ClockModule.ino         # NTP + timezone handling
├─ WeatherModule.ino       # OpenWeather fetch + icon
├─ GrillProb.ino           # Meater+ BLE probe (Bluedroid)
├─ Config.h                # Pins, Wi‑Fi, API keys, constants
├─ examples/
│   └─ MeaterProbe_BLE.ino # Minimal scan/connect/notify skeleton
└─ README.md
```

---

## Prerequisites

**Arduino IDE / ESP32 core**
- **ESP32 board package 2.0.x**
- Board: `ESP32 Dev Module` or `TTGO T‑Display`

**Libraries**
- **TFT_eSPI** (Bodmer) — configured for TTGO T‑Display
- **ArduinoJson**
- **HTTPClient** (bundled)
- **WiFi** (bundled)
- **NTPClient**
- **ESP32 BLE Arduino (Bluedroid)** ← *use this instead of NimBLE*

> In the Arduino Library Manager, the package is usually called **ESP32 BLE Arduino**.  
> It provides `<BLEDevice.h>`, `<BLEUtils.h>`, `<BLEClient.h>`, etc.

---

## Configuration (`Config.h`)

```cpp
// Wi‑Fi / Weather
#define WIFI_SSID            "YourNetwork"
#define WIFI_PASSWORD        "YourPassword"
#define OPENWEATHER_API_KEY  "YourKey"
#define ZIP_CODE             "12345"
#define COUNTRY_CODE         "us"

// Battery / Pins
#define BATTERY_PIN          34          // ADC pin for voltage divider
#define BAT_VDIV_R1          200000.0f   // ohms (to battery+)
#define BAT_VDIV_R2          100000.0f   // ohms (to GND)
#define BAT_SAMPLES          8
#define BAT_ALPHA            0.2f        // 0..1, higher = snappier

// Buttons / UI
#define DEBOUNCE_MS          25
#define LONGPRESS_MS         600

// Meater+ / BLE
#define MEATER_TARGET_MAC    ""          // Optional: lock to a known probe MAC
#define MEATER_CONNECT_TIMEOUT_MS  8000
#define MEATER_READ_PERIOD_MS      1000  // Notify/read cadence
```

**Finding the Meater+ MAC**  
- Use `examples/MeaterProbe_BLE.ino` to scan once and print MACs that look like the Meater relay.  
- Paste your MAC into `MEATER_TARGET_MAC` for faster connection.

---

## Using the Meater+ Probe

1. Make sure the **Meater+ charger** has power (relay active).  
2. Remove the probe from the charger to wake it.  
3. The ESP32 scans and connects (or uses your pinned MAC).  
4. The UI shows **meat** and **ambient** temps; the Top Bar continues to show city/battery/time/weather.

> GATT UUIDs can change with firmware. The probe module discovers services/characteristics at runtime and subscribes to temperature notifications where available. If notify isn’t exposed, it falls back to timed reads.

---

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---|---|---|
| **math.h ‘unterminated argument list invoking macro GRILL_LOG’** | Macro name collision | Rename/`#undef GRILL_LOG` before including `<math.h>` or include headers first, then define macros. |
| **Meater not found** | Charger asleep / MAC mismatch | Power the charger, rescan, move closer, paste the correct MAC. |
| **Temps don’t update** | No notify on that FW | Ensure the read timer is running; lower `MEATER_READ_PERIOD_MS` (e.g., 500–1000 ms). |
| **Battery % jumpy** | ADC noise | Raise `BAT_SAMPLES` or tweak `BAT_ALPHA` (0.1–0.3). |
| **UI only refreshes on button** | Partial screen refresh strategy | Add a timed refresh hook for the top bar / sections needing periodic updates. |

---

## Roadmap

- SPIFFS/SD cook logs  
- Alarm thresholds (buzzer/LED)  
- Simple captive-portal setup page  
- Export to web dashboard

---

## License
MIT

## Credits
- Hardware & build: **Tristin Skinner (Ingot Works)**
- BLE (Bluedroid): **ESP32 BLE Arduino**
- Display: **TFT_eSPI**
- Weather: **OpenWeatherMap**
