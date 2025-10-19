// ==== ClockModule.ino (robust timezone + weather) ====
#include "ClockModule.h"
#include "Config.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// --------- State ---------
static WiFiUDP ntpUDP;
static NTPClient ntp(ntpUDP, "pool.ntp.org", 0 /* set later */, 60 * 1000);

static int timezoneOffsetSec = 0;
static String cityName = "";
static unsigned long lastTzFetchMs = 0;

// Weather cache
static String cachedTempStr = "--°";
static unsigned long lastWeatherFetchMs = 0;

// ---------- Internals ----------
static bool httpGet(const String& url, String& body, uint16_t timeoutMs = WEATHER_HTTP_TIMEOUT_MS) {
  HTTPClient http;
  http.setTimeout(timeoutMs);
  http.begin(url);
  int code = http.GET();
  if (code == HTTP_CODE_OK) {
    body = http.getString();
    http.end();
    return true;
  }
  #if DEBUG_SERIAL
    Serial.printf("[Clock] HTTP GET failed (%d) for %s\n", code, url.c_str());
  #endif
  http.end();
  return false;
}

static bool fetchTimezoneAndCity() {
  if (!OPENWEATHER_ENABLED) return false;

  // Build OpenWeather URL for current conditions (includes "timezone" + "name")
  String url = "http://api.openweathermap.org/data/2.5/weather?";
  if (USE_ZIP_OVER_LATLON) {
    // ZIP must include country code (e.g., 80301,US). We'll assume the user supplies it in Config.h
    url += "zip=" + String(ZIP_CODE);
  } else {
    url += "lat=" + String(LATITUDE) + "&lon=" + String(LONGITUDE);
  }
  url += "&appid=" + String(OPENWEATHER_API_KEY);
  url += "&units=" + String(OPENWEATHER_UNITS);

  String body;
  if (!httpGet(url, body)) return false;

  // Parse compactly
  StaticJsonDocument<1536> doc;  // plenty for the small payload
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    #if DEBUG_SERIAL
      Serial.printf("[Clock] JSON parse error: %s\n", err.c_str());
    #endif
    return false;
  }

  // timezone: shift in seconds from UTC (handles DST on their side)
  if (doc.containsKey("timezone")) {
    timezoneOffsetSec = doc["timezone"].as<int>();
  } else {
    #if DEBUG_SERIAL
      Serial.println("[Clock] 'timezone' missing in response; leaving offset unchanged");
    #endif
  }

  // city name (may be empty if coords)
  if (doc.containsKey("name")) {
    cityName = doc["name"].as<String>();
  }

  // Update NTP offset immediately
  ntp.setTimeOffset(timezoneOffsetSec);

  #if DEBUG_SERIAL
    Serial.printf("[Clock] City=%s, tzOffset=%d\n", cityName.c_str(), timezoneOffsetSec);
  #endif
  return true;
}

static bool fetchWeatherTemp() {
  if (!OPENWEATHER_ENABLED) return false;

  String url = "http://api.openweathermap.org/data/2.5/weather?";
  if (USE_ZIP_OVER_LATLON) {
    url += "zip=" + String(ZIP_CODE);
  } else {
    url += "lat=" + String(LATITUDE) + "&lon=" + String(LONGITUDE);
  }
  url += "&appid=" + String(OPENWEATHER_API_KEY);
  url += "&units=" + String(OPENWEATHER_UNITS);

  String body;
  if (!httpGet(url, body)) return false;

  StaticJsonDocument<1536> doc;
  if (deserializeJson(doc, body)) {
    return false;
  }

  // main.temp is double
  if (doc.containsKey("main") && doc["main"].containsKey("temp")) {
    float t = doc["main"]["temp"].as<float>();
    // Round to nearest whole for compact top-bar display
    int ti = (int)roundf(t);
    cachedTempStr = String(ti) + "°";
    return true;
  }
  return false;
}

// ---------- Public API ----------
void Clock_init() {
  #if DEBUG_SERIAL
    Serial.println("[Clock] init");
  #endif

  if (WIFI_ENABLED) {
    WiFi.mode(WIFI_STA);
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      #if DEBUG_SERIAL
        Serial.printf("[Clock] WiFi connecting to '%s'...\n", WIFI_SSID);
      #endif
      unsigned long start = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
        delay(200);
        #if DEBUG_SERIAL
          Serial.print(".");
        #endif
      }
      #if DEBUG_SERIAL
        Serial.println();
      #endif
    }
    if (WiFi.status() == WL_CONNECTED) {
      #if DEBUG_SERIAL
        Serial.printf("[Clock] WiFi OK, IP: %s\n", WiFi.localIP().toString().c_str());
      #endif
    } else {
      #if DEBUG_SERIAL
        Serial.println("[Clock] WiFi not connected; time will be UTC unless offset cached");
      #endif
    }
  }

  ntp.begin();
  ntp.setUpdateInterval(60 * 1000);

  // Try to get timezone on boot (if WiFi + API ready)
  if (WIFI_ENABLED && WiFi.status() == WL_CONNECTED && OPENWEATHER_ENABLED && strlen(OPENWEATHER_API_KEY) > 0) {
    if (fetchTimezoneAndCity()) {
      lastTzFetchMs = millis();
    }
    // Prime weather too
    if (fetchWeatherTemp()) {
      lastWeatherFetchMs = millis();
    }
  }
}

void Clock_update() {
  ntp.update();

  // Refresh timezone occasionally to keep DST correct; 12h is plenty
  const unsigned long TZ_REFRESH_MS = 12UL * 60UL * 60UL * 1000UL;
  unsigned long now = millis();
  if (WIFI_ENABLED && WiFi.status() == WL_CONNECTED && OPENWEATHER_ENABLED && strlen(OPENWEATHER_API_KEY) > 0) {
    if (now - lastTzFetchMs > TZ_REFRESH_MS || lastTzFetchMs == 0) {
      if (fetchTimezoneAndCity()) lastTzFetchMs = now;
    }
    if (now - lastWeatherFetchMs > WEATHER_POLL_MS || lastWeatherFetchMs == 0) {
      if (fetchWeatherTemp()) lastWeatherFetchMs = now;
    }
  }
}

TimeData Clock_get() {
  TimeData td{};
  // Epoch already adjusted by ntp timeOffset
  time_t epoch = ntp.getEpochTime();
  struct tm *tmn = gmtime(&epoch); // safe because we've applied offset to epoch

  td.h24 = tmn->tm_hour;
  td.m   = tmn->tm_min;
  td.s   = tmn->tm_sec;

  if (UI_24H) {
    td.h12 = 0;
    td.period = "";
  } else {
    int h = td.h24;
    td.period = (h < 12) ? "AM" : "PM";
    h = h % 12;
    td.h12 = (h == 0) ? 12 : h;
  }

  td.city = cityName;
  return td;
}

String Weather_getTempStr() {
  return cachedTempStr;
}
