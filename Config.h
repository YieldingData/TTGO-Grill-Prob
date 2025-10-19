#pragma once
/*
  Config.h — TTGO Meater+ Display (ESP32 + T-Display)
  Centralized configuration & feature toggles.
  Edit safely; no application logic here.
*/

/* ==============================
 *  Firmware / Build
 * ============================== */
#define FW_VERSION                 "0.1.0"
#define FEATURE_DEVICE_SCREEN      true     // true = include Scan/Pair screen
#define SAVE_ON_CHANGE             true     // true = persist immediately; false = only on "Save & Exit"
#define NVS_NAMESPACE              "meater_ui"


/* ==============================
 *  Hardware Pins (TTGO T-Display v1.1 typical)
 * ============================== */
#define PIN_BTN_LEFT               0       // Left button (input-only)
#define PIN_BTN_RIGHT              35        // Right/BOOT button
#define PIN_BL                     4        // TFT backlight (PWM capable)
// Optional buzzer for alarms; set to -1 to disable
#define PIN_BUZZER                 -1


/* ==============================
 *  UI Preferences
 * ============================== */
#define UI_24H                     false    // true = 24h clock; false = 12h AM/PM
#define UI_TEMP_FAHREN             true     // true = display °F; false = °C
#define DEFAULT_TARGET_F           135.0f   // initial cook target
#define UI_BACKLIGHT_DEFAULT       255      // 0–255 initial brightness
#define UI_BACKLIGHT_TIMEOUT_S     0        // 0 = never auto-off
#define UI_STATUS_HEARTBEAT_MS     1000UL   // UI refresh cadence


/* ==============================
 *  Wi‑Fi & Weather
 * ============================== */
#define WIFI_ENABLED               true
#define WIFI_SSID                  "*****"
#define WIFI_PASSWORD              "*****"

#define OPENWEATHER_ENABLED        true
#define OPENWEATHER_API_KEY        "*******"
#define ZIP_CODE                   "*****"   // e.g., "80301,US" (ZIP[,CC])
// If you prefer coordinates, set USE_ZIP_OVER_LATLON to false and fill LAT/LON
#define USE_ZIP_OVER_LATLON        true
#define LATITUDE                   "40.0000"
#define LONGITUDE                  "-105.0000"

// Units for OpenWeather requests: "imperial" or "metric"
// (Recommend keeping this aligned with UI_TEMP_FAHREN in app code.)
#define OPENWEATHER_UNITS          "imperial"

// Polling & timeouts
#define WEATHER_POLL_MS            (20UL * 60UL * 1000UL)   // current conditions every 20 min
#define FORECAST_POLL_MS           (2UL  * 60UL * 60UL * 1000UL) // forecast every 2 hours
#define WEATHER_HTTP_TIMEOUT_MS    6000UL
#define WEATHER_STALE_MS           (45UL * 60UL * 1000UL)   // mark weather stale after 45 min


/* ==============================
 *  BLE / Meater+
 * ============================== */
// If you know the probe’s MAC, set it (uppercase, colon-separated). Empty "" if unknown.
#define MEATER_TARGET_MAC          "**:**:**:**:**:**"   // e.g., "AB:CD:EF:12:34:56"

// Name prefixes to match if MAC unknown (first match wins).
#define MEATER_NAME_PREFIX_1       "MEATER"
#define MEATER_NAME_PREFIX_2       ""            // optional
#define MEATER_NAME_PREFIX_3       ""            // optional

// BLE timings
#define BLE_SCAN_WINDOW_MS         8000UL        // scan duration when user triggers Scan
#define BLE_CONNECT_TIMEOUT_MS     5000UL
#define BLE_DATA_STALE_MS          15000UL       // mark Meater data stale after 15s with no update
#define BLE_RETRY_BACKOFF_MS       3000UL        // wait before retrying connect


/* ==============================
 *  Status / Alarm Behavior
 * ============================== */
#define ALARM_ENABLED              true
#define ALARM_LATCH_UNTIL_ACK      true          // keep banner until user acknowledges
#define ALARM_BEEP_MS              0             // 0 = silent; else beep length (requires PIN_BUZZER >= 0)
#define ALARM_REARM_DELTA_F        2.0f          // alarm re-arms when probe drops below target - delta


/* ==============================
 *  Developer / Debug
 * ============================== */
#define DEBUG_SERIAL               true
#define LOG_BLE_EVENTS             false
#define LOG_WEATHER_HTTP           false
