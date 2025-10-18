# TTGO Grill Thermometer

## Overview
The **TTGO Grill Thermometer** is a Wi-Fi-enabled, battery-powered BBQ monitoring station built around the ESP32-based **TTGO T-Display** board.  
It provides real-time temperature readings from multiple probes, displays weather and time via the top bar interface, and syncs data to the web for remote monitoring.

The goal of this project is to create a **stand-alone smart thermometer** with:
- High-visibility TFT display (outdoor-readable)
- Live temperature graphs
- Wi-Fi weather/time sync
- Battery percentage indicator
- Configurable probe channels
- Modular code structure (each feature in its own `.ino`)

---

## Hardware

| Component | Description | Notes |
|------------|--------------|-------|
| **TTGO T-Display ESP32** | 1.14″ TFT screen + onboard ESP32 | Main controller and display |
| **Thermistor Probes (NTC 100K)** | Temperature sensors | One per grill/meat probe |
| **TP4056 Module** | 1-cell Li-ion charging | For 18650 or LiPo battery |
| **Battery (18650 or LiPo)** | Power source | 3.7 V nominal |
| **Voltage Divider** | Battery voltage monitor | Used for battery % readout |
| **Buck Converter (optional)** | For stable 5 V rail | Optional if powering external sensors |
| **Wires / Connectors** | JST or barrel | As needed |

---

## Software Structure

```
TTGO_Grill_Thermometer/
├── user_interface_v3.ino     # Main program entry
├── TopBar.ino                # Displays time, city, weather, and battery %
├── ClockModule.ino           # Handles NTP time + timezone sync
├── WeatherModule.ino         # Fetches weather from OpenWeather API
├── GrillProb.ino             # Reads thermistor probe data
├── Config.h                  # API keys, pins, constants
└── README.md
```

---

## Features

✅ **Top Bar Display**
- Shows current time, city, weather condition, and filtered battery percentage.
- Updates automatically every minute or when buttons are pressed.

✅ **Clock & Weather Sync**
- Fetches timezone and weather using **OpenWeatherMap API**.
- Adjusts for local timezone offset automatically based on ZIP code.

✅ **Battery Filtering**
- Smooths out ADC noise with exponential or moving average filtering.
- Displays “Bat XX%” to the right of city name.

✅ **Button Control**
- Three buttons for UI navigation and probe management.
- Long-press and debounce logic separated in constants.

✅ **Modular Design**
- Each function lives in its own `.ino` for easy development and reuse.

---

## Setup & Configuration

### 1. Clone the Repository
```bash
git clone https://github.com/YieldingData/TTGO_Grill_Thermometer.git
```

### 2. Install Dependencies
In Arduino IDE, install the following libraries:
- **TFT_eSPI** by Bodmer  
- **ArduinoJson**  
- **HTTPClient**  
- **WiFi** (ESP32 built-in)  
- **NTPClient**

### 3. Configure `Config.h`
Update these fields:
```cpp
#define WIFI_SSID      "YourNetworkName"
#define WIFI_PASSWORD  "YourPassword"
#define OPENWEATHER_API_KEY "Your_API_Key"
#define ZIP_CODE       "12345"
#define COUNTRY_CODE   "us"
```

You can also adjust:
- `BATTERY_PIN`  
- `DEBOUNCE_MS`  
- `LONGPRESS_MS`  
- `PROBE_COUNT`  

### 4. Upload to Board
- Select **Board:** ESP32 Dev Module (or TTGO T-Display)
- Upload via USB-C
- Open Serial Monitor for debug output

---

## Usage

1. Power on the unit or plug in via USB.  
2. It will automatically connect to Wi-Fi and fetch the correct time and weather.  
3. Top bar shows: `City | Bat XX% | Time | Weather Icon`.  
4. Insert probes into food and grill.  
5. View live temperatures on the display.  
6. Long-press a button to toggle probe channels or refresh data.

---

## Future Improvements

| Feature | Status | Notes |
|----------|---------|-------|
| Data logging to SD card | ☐ | Store cooking sessions |
| Bluetooth mobile app | ☐ | Companion control app |
| Web dashboard | ☐ | Live remote monitoring |
| Auto sleep mode | ☐ | Extend battery life |
| Alarm system | ☐ | Audible/visual temp alerts |

---

## Troubleshooting

| Symptom | Likely Cause | Fix |
|----------|---------------|-----|
| Compile error in math.h ("unterminated argument list") | Macro conflict with `GRILL_LOG` | Rename or undefine macro before including `math.h` |
| No time update | NTP or timezone fetch failed | Verify Wi-Fi and API key |
| Temperature reads 0 °C | Probe disconnected or ADC pin misconfigured | Check wiring and pin constants |
| Battery % jumps | ADC noise | Adjust filter constant or sampling rate |

---

## License
This project is released under the **MIT License**.  
Feel free to modify, fork, and share under the same terms.

---

## Credits
Created by **Tristin Skinner**  
Part of the **Ingot Works Electronics Series**  
Built with Arduino IDE and open-source libraries.
