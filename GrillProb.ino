// ==== GrillProb.ino ====
// BLE client using classic ESP32 BLE Arduino (NOT NimBLE-Arduino)
// Scans, connects to probe, subscribes to first notifiable characteristic,
// and tries to parse two temperatures from notifications.
//
// Requires: ESP32 BLE Arduino (nkolban/chegewara).

#include "GrillProb.h"
#include "Config.h"


// ---- Local fallback in case GrillProb.h wasn't picked up ----
#ifndef GRILLPROB_PROBEDATA_DEFINED
#define GRILLPROB_PROBEDATA_DEFINED
struct ProbeData {
  bool        connected = false;
  int         rssi      = 0;
  float       internalF = NAN;   // food probe (meat) temp
  float       ambientF  = NAN;   // ambient/grill temp
  uint32_t    ageMs     = UINT32_MAX; // ms since last update
  bool        stale     = true;       // true when ageMs > BLE_DATA_STALE_MS
};
#endif

#if !defined(ARDUINO_ARCH_ESP32)
  #error "This module targets ESP32 with the 'ESP32 BLE Arduino' library."
#endif

// ---- ESP32 BLE Arduino (legacy Bluedroid) ----
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLEAddress.h>

static BLEClient*      gClient   = nullptr;
static BLERemoteCharacteristic* gNotifyChar = nullptr;
static BLEScan*        gScan     = nullptr;

static String targetMac = MEATER_TARGET_MAC;   // from Config.h
static String namePref1 = MEATER_NAME_PREFIX_1;


static uint32_t lastUpdateMs = 0;
static int      lastRSSI     = 0;
static float    tempInternalF = NAN;
static float    tempAmbientF  = NAN;
static bool     isConnected   = false;
static unsigned long lastTryConnectMs = 0;
static unsigned long lastScanStartMs  = 0;

static String foundAddrStr; // keep as string to avoid type issues
static bool hasFoundTarget = false;

// ---- Helpers from Config.h (timers) ----
#ifndef BLE_SCAN_WINDOW_MS
  #define BLE_SCAN_WINDOW_MS 8000UL
#endif
#ifndef BLE_CONNECT_TIMEOUT_MS
  #define BLE_CONNECT_TIMEOUT_MS 5000UL
#endif
#ifndef BLE_DATA_STALE_MS
  #define BLE_DATA_STALE_MS 15000UL
#endif
#ifndef BLE_RETRY_BACKOFF_MS
  #define BLE_RETRY_BACKOFF_MS 3000UL
#endif
#ifndef DEBUG_SERIAL
  #define DEBUG_SERIAL true
#endif

// ---- Logging macro ----
#if DEBUG_SERIAL
  #define GRILL_LOG(...)  do { Serial.printf(__VA_ARGS__); } while(0)
#else
  #define GRILL_LOG(...)
#endif

// ---- Match helpers ----
static bool matchesNamePrefix(const std::string& name) {
  if (name.empty()) return false;
  auto s = String(name.c_str());
  s.trim();
  if (namePref1.length() && s.startsWith(namePref1)) return true;
  if (namePref2.length() && s.startsWith(namePref2)) return true;
  if (namePref3.length() && s.startsWith(namePref3)) return true;
  return false;
}

static bool matchesMacStr(const String& addrStr) {
  if (targetMac.length() == 0) return false;
  String a = addrStr; a.toUpperCase();
  String t = targetMac; t.toUpperCase();
  return a == t;
}

// ---- Notification callback & parsing ----
static void parseTempNotification(const uint8_t* data, size_t len) {
  // Heuristic decoders seen in many probes:
  // 1) [int16 tenths °C][int16 tenths °C] -> two temps
  // 2) [int16 °C x 100][int16 °C x 100]
  // 3) Fallback: scan for plausible ranges
  float inC = NAN, ambC = NAN;

  auto rd_s16 = [&](int off)->int16_t {
    if (off + 1 >= (int)len) return 0;
    return (int16_t)((data[off+1] << 8) | data[off]);
  };

  if (len >= 4) {
    int16_t a = rd_s16(0);
    int16_t b = rd_s16(2);

    // Guess scale (tenths °C typical)
    auto tryScale = [&](float scale)->bool {
      float x = a / scale, y = b / scale;
      if (x > -40 && x < 350 && y > -40 && y < 350) { inC = x; ambC = y; return true; }
      return false;
    };
    if (!tryScale(10.0f))  (void)tryScale(100.0f);
  }

  // Fallback: leave NANs; we’ll log raw
  if (isnan(inC) || isnan(ambC)) {
    GRILL_LOG("[BLE] Unparsed payload (%u bytes):", (unsigned)len);
    for (size_t i=0;i<len;i++) GRILL_LOG(" %02X", data[i]);
    GRILL_LOG("\n");
    return;
  }

  // Convert to F/°C per UI preference
  auto c2f = [](float c){ return c * 9.0f/5.0f + 32.0f; };
#if UI_TEMP_FAHREN
  tempInternalF = c2f(inC);
  tempAmbientF  = c2f(ambC);
#else
  tempInternalF = inC;
  tempAmbientF  = ambC;
#endif
  lastUpdateMs = millis();
  GRILL_LOG("[BLE] Temps: internal=%.1f, ambient=%.1f (%s)\n",
       tempInternalF, tempAmbientF, UI_TEMP_FAHREN ? "F" : "C");
}

static void notifyCB(
  BLERemoteCharacteristic* /*rc*/,
  uint8_t* data, size_t len, bool /*isNotify*/) {
  parseTempNotification(data, len);
}

// ---- Scan callbacks ----
class GrillScanCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice adv) override {
    // Track RSSI for info
    lastRSSI = adv.getRSSI();

    String addrStr = String(adv.getAddress().toString().c_str());
    bool macMatch  = matchesMacStr(addrStr);
    bool nameMatch = matchesNamePrefix(adv.haveName() ? adv.getName() : "");
    if (macMatch || nameMatch) {
      foundAddrStr = addrStr;
      hasFoundTarget = true;
      GRILL_LOG("[BLE] Found target: %s  name=\"%s\"  RSSI=%d\n",
           foundAddrStr.c_str(),
           adv.haveName() ? adv.getName().c_str() : "",
           adv.getRSSI());
      // Stop scan early; we have our candidate
      BLEDevice::getScan()->stop();
    }
  }
};

// ---- Connection routine ----
static bool connectToTarget() {
  if (!hasFoundTarget) return false;

  if (!gClient) gClient = BLEDevice::createClient();
  gClient->setClientCallbacks(nullptr);

  GRILL_LOG("[BLE] Connecting to %s ...\n", foundAddrStr.c_str());
  unsigned long start = millis();
  BLEAddress addr(foundAddrStr.c_str());
  if (!gClient->connect(addr)) {
    GRILL_LOG("[BLE] Connect failed\n");
    return false;
  }
  // Crude timeout guard
  if (millis() - start > BLE_CONNECT_TIMEOUT_MS) {
    GRILL_LOG("[BLE] Connect timed out\n");
    gClient->disconnect();
    return false;
  }

  isConnected = true;

  // Service/characteristic discovery: take first notifiable characteristic
  gNotifyChar = nullptr;
  try {
    std::map<std::string, BLERemoteService*>* svcs = gClient->getServices();
    GRILL_LOG("[BLE] Discovered %u services\n", (unsigned)svcs->size());
    for (auto& svcKV : *svcs) {
      auto* svc = svcKV.second;
      auto* chars = svc->getCharacteristics(); // map<string, BLERemoteCharacteristic*>*
      for (auto& chKV : *chars) {
        auto* ch = chKV.second;
        bool notifiable = ch->canNotify();
        bool indic      = ch->canIndicate();
        GRILL_LOG("  Char %s props: read=%d write=%d notify=%d indicate=%d\n",
             ch->getUUID().toString().c_str(),
             (int)ch->canRead(), (int)ch->canWrite(), (int)notifiable, (int)indic);
        if (!gNotifyChar && (notifiable || indic)) {
          gNotifyChar = ch;
        }
      }
    }
  } catch(...) {
    GRILL_LOG("[BLE] Exception while discovering services\n");
  }

  if (gNotifyChar) {
    GRILL_LOG("[BLE] Subscribing to %s\n", gNotifyChar->getUUID().toString().c_str());
    try {
      gNotifyChar->registerForNotify(notifyCB);
    } catch(...) {
      GRILL_LOG("[BLE] Failed to register notification\n");
    }
  } else {
    GRILL_LOG("[BLE] No notifiable characteristic found; will try polling readable ones.\n");
  }

  lastUpdateMs = millis();
  return true;
}

// ---- Public API ----
void GrillProb_begin() {
#if DEBUG_SERIAL
  Serial.begin(115200);
  delay(50);
#endif
  BLEDevice::init("TTGO-Grill"); // client name
  gScan = BLEDevice::getScan();
  gScan->setAdvertisedDeviceCallbacks(new GrillScanCallbacks());
  gScan->setActiveScan(true);
  gScan->setInterval(160); // ~100 ms
  gScan->setWindow(80);    // ~50 ms
  GRILL_LOG("[BLE] GrillProb_begin: targetMac=%s, namePrefixes=['%s','%s','%s']\n",
       targetMac.c_str(), namePref1.c_str();
}

void GrillProb_loop() {
  unsigned long now = millis();

  // Handle stale flagging
  if (lastUpdateMs == 0) {
    // nothing yet
  } else if ((now - lastUpdateMs) > BLE_DATA_STALE_MS) {
    // keep temps but mark as stale in getter
  }

  // Maintain connection
  if (isConnected) {
    if (!gClient || !gClient->isConnected()) {
      GRILL_LOG("[BLE] Lost connection\n");
      isConnected = false;
      gNotifyChar = nullptr;
      lastTryConnectMs = now;
    }
    return;
  }

  // Not connected
  if (!hasFoundTarget) {
    if (gScan && (now - lastScanStartMs) > BLE_RETRY_BACKOFF_MS) {
      lastScanStartMs = now;
      GRILL_LOG("[BLE] Scanning %lu ms...\n", BLE_SCAN_WINDOW_MS);
      gScan->start(BLE_SCAN_WINDOW_MS / 1000); // seconds
      // Result callbacks will mark hasFoundTarget and stop scan early
    }
    return;
  }

  // We have a candidate; throttle connection attempts
  if ((now - lastTryConnectMs) < BLE_RETRY_BACKOFF_MS) return;
  lastTryConnectMs = now;
  (void)connectToTarget();
}

ProbeData GrillProb_get() {
  ProbeData out;
  out.connected = isConnected && gClient && gClient->isConnected();
  out.rssi      = lastRSSI;
  out.internalF = tempInternalF;
  out.ambientF  = tempAmbientF;
  out.ageMs     = (lastUpdateMs == 0) ? UINT32_MAX : (millis() - lastUpdateMs);
  out.stale     = (out.ageMs > BLE_DATA_STALE_MS);
  return out;
}
