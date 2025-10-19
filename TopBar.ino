// ==== TopBar.ino ====
#include "TopBar.h"
#include <math.h>

static TFT_eSPI* s_tft = nullptr;
static unsigned long s_lastMs = 0;

// Battery filter state
static float g_battPctEma   = NAN;
static int   g_battPctShown = -1;

// -------- Helpers: battery --------
static float readBatteryVoltageOnce() {
#if defined(ESP32)
  analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
  uint32_t mv = analogReadMilliVolts(BATTERY_ADC_PIN);
  return (mv / 1000.0f) * BATTERY_VDIV;
#else
  return NAN;
#endif
}

static float readBatteryVoltageAveraged() {
  float acc = 0.0f; int n = 0;
  for (int i = 0; i < BATTERY_SAMPLES; ++i) {
    float v = readBatteryVoltageOnce();
    if (!isnan(v)) { acc += v; n++; }
    delay(2);
  }
  if (n == 0) return NAN;
  return acc / n;
}

static int batteryPercentFromVoltage(float v) {
  if (isnan(v)) return -1;
  if (v <= BATTERY_MIN_V) return 0;
  if (v >= BATTERY_MAX_V) return 100;
  float p = (v - BATTERY_MIN_V) / (BATTERY_MAX_V - BATTERY_MIN_V);
  int pct = (int)roundf(p * 100.0f);
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return pct;
}

static void updateBatteryFiltered() {
  float vAvg = readBatteryVoltageAveraged();
  int pctRaw = batteryPercentFromVoltage(vAvg);
  if (pctRaw < 0) return;

  if (isnan(g_battPctEma)) g_battPctEma = pctRaw;
  else g_battPctEma = BATTERY_ALPHA * pctRaw + (1.0f - BATTERY_ALPHA) * g_battPctEma;

  int pctRounded = (int)roundf(g_battPctEma);
  if (g_battPctShown < 0 || abs(pctRounded - g_battPctShown) >= BATTERY_HYSTERESIS_PCT) {
    g_battPctShown = pctRounded;
  }
}

static String getBatteryPercentStr() {
  if (g_battPctShown < 0) return "";
  return String(g_battPctShown) + "%";
}

// -------- Drawing --------
static void drawTopBarTwoLine(const TimeData& td, const String& tempStr) {
  auto& tft = *s_tft;
  tft.fillRect(0, 0, tft.width(), TOPBAR_H, COLOR_TOPBAR);
  tft.drawLine(0, TOPBAR_H - 1, tft.width(), TOPBAR_H - 1, COLOR_FRAME);

  tft.setTextColor(COLOR_TEXT, COLOR_TOPBAR);
  tft.setTextFont(1);
  tft.setTextSize(1);

  char timeBuf[16];
  if (UI_24H) snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", td.h24, td.m);
  else snprintf(timeBuf, sizeof(timeBuf), "%d:%02d %s", td.h12 == 0 ? 12 : td.h12, td.m, td.period.c_str());

  // Line 1: time (left), temp (right)
  tft.setTextDatum(TL_DATUM);
  tft.drawString(timeBuf, 3, 2);
  tft.setTextDatum(TR_DATUM);
  tft.drawString(tempStr.length() ? tempStr : String("--°"), tft.width() - 3, 2);

  // Line 2: City + "Bat XX%"
  String batt = getBatteryPercentStr();
  String line2 = td.city;
  if (batt.length()) {
    if (line2.length()) line2 += "  Bat ";
    else line2 += "Bat ";
    line2 += batt;
  }

  tft.setTextDatum(TC_DATUM);
  int w = tft.textWidth(line2);
  if (w <= tft.width() - 8) {
    tft.drawString(line2, tft.width() / 2, 12);
  } else {
    // Try to truncate the city to fit
    String cityTrim = td.city;
    while (cityTrim.length() && tft.textWidth(cityTrim + "  Bat " + batt) > tft.width() - 8) {
      cityTrim.remove(cityTrim.length() - 1);
    }
    String combo = cityTrim;
    if (batt.length()) {
      if (combo.length()) combo += "  Bat ";
      else combo += "Bat ";
      combo += batt;
    }
    if (tft.textWidth(combo) <= tft.width() - 8) {
      tft.drawString(combo, tft.width() / 2, 12);
    } else {
      if (td.city.length()) {
        tft.setTextDatum(TC_DATUM);
        tft.drawString(td.city, tft.width() / 2, 12);
      }
      if (batt.length()) {
        tft.setTextDatum(TR_DATUM);
        tft.drawString(String("Bat ") + batt, tft.width() - 3, 12);
      }
    }
  }
}

// -------- Public API --------
void TopBar_init(TFT_eSPI* tft) {
  s_tft = tft;
  s_lastMs = 0;
  g_battPctEma = NAN;
  g_battPctShown = -1;
  // Seed once so first draw is stable
  updateBatteryFiltered();
}

void TopBar_tick(const TimeData& td, const String& tempStr) {
  if (!s_tft) return;
  unsigned long now = millis();
  if (now - s_lastMs < UI_REDRAW_TOPBAR_MS) return;
  s_lastMs = now;
  updateBatteryFiltered();
  drawTopBarTwoLine(td, tempStr);
}

void TopBar_redraw(const TimeData& td, const String& tempStr) {
  if (!s_tft) return;
  updateBatteryFiltered();
  drawTopBarTwoLine(td, tempStr);
}
