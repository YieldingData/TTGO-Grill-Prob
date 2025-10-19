// ==== user_interface_v3.ino ====
// Test UI using TopBar module + ClockModule
// - Top bar auto-refreshes each second via TopBar_tick()
// - Menu draws beneath and only refreshes on button action

#include <Arduino.h>
#include "Config.h"
#include "ClockModule.h"
#include "TopBar.h"
#include "GrillProb.h"


#include <TFT_eSPI.h>
#include <SPI.h>

// ---------------------------
// Globals
// ---------------------------
TFT_eSPI tft = TFT_eSPI();

// Backlight control (ESP32 LEDC PWM)
static const int BL_CHANNEL = 2;
static const int BL_FREQ    = 5000;
static const int BL_RES     = 8;   // 0..255

// Simple menu items to test navigation
const char* MENU_ITEMS[] = {
  "Scan for Probe",
  "Connect",
  "Start Cook",
  "Targets",
  "Settings",
  "About"
};
const int MENU_COUNT = sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]);
int selectedIndex = 0;

// Buttons
const uint32_t DEBOUNCE_MS  = 25;
const uint32_t LONGPRESS_MS = 600;
struct BtnState {
  int pin; bool hasPullup; bool pressed; bool last;
  uint32_t lastChangeMs; uint32_t pressStartMs;
};
BtnState btnRight = { PIN_BTN_RIGHT, false, false, false, 0, 0 };
BtnState btnLeft  = { PIN_BTN_LEFT,  true,  false, false, 0, 0 };

// Colors / metrics
#ifndef COLOR_BG
  #define COLOR_BG 0x0000
#endif
#ifndef COLOR_TEXT
  #define COLOR_TEXT 0xFFFF
#endif
#ifndef COLOR_SELECTED
  #define COLOR_SELECTED 0xFBE0
#endif

#ifndef LIST_TOP
  #define LIST_TOP (TOPBAR_H + 4)
#endif
const uint16_t LINE_H = 22;

// ---------------------------
// Buttons helpers
// ---------------------------
static inline bool readButtonPin(int pin, bool hasPullup) {
  int raw = digitalRead(pin);
  return hasPullup ? (raw == LOW) : (raw == HIGH);
}
void updateButton(BtnState& b) {
  bool now = readButtonPin(b.pin, b.hasPullup);
  uint32_t t = millis();
  if (now != b.last) {
    if (t - b.lastChangeMs > DEBOUNCE_MS) {
      b.lastChangeMs = t; b.last = now;
      if (now) { b.pressed = true; b.pressStartMs = t; }
      else { b.pressed = false; }
    }
  }
}
bool consumeShortPress(BtnState& b) {
  uint32_t t = millis();
  if (!b.pressed && b.lastChangeMs != 0) {
    uint32_t held = t - b.pressStartMs;
    if (held >= DEBOUNCE_MS && held < LONGPRESS_MS) { b.pressStartMs = 0; return true; }
  }
  return false;
}
bool isLongPress(BtnState& b) {
  if (b.pressed) { return (millis() - b.pressStartMs) >= LONGPRESS_MS; }
  return false;
}

// ---------------------------
// Drawing
// ---------------------------
void drawMenu() {
  tft.fillRect(0, LIST_TOP - 4, tft.width(), tft.height() - (LIST_TOP - 4), COLOR_BG);
  int y = LIST_TOP;
  for (int i = 0; i < MENU_COUNT; ++i) {
    bool isSel = (i == selectedIndex);
    uint16_t textColor = isSel ? COLOR_SELECTED : COLOR_TEXT;
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(textColor, COLOR_BG);
    String line = (isSel ? "> " : "  "); line += MENU_ITEMS[i];
    tft.drawString(line, 8, y, 2);
    y += LINE_H;
  }
}

// ---------------------------
// Setup / Loop
// ---------------------------
void setup() {
  #if DEBUG_SERIAL
    Serial.begin(115200); delay(100); Serial.println("\n[UI] Boot");
  #endif

  // Backlight PWM
  ledcSetup(BL_CHANNEL, BL_FREQ, BL_RES);
  ledcAttachPin(PIN_BL, BL_CHANNEL);
  ledcWrite(BL_CHANNEL, UI_BACKLIGHT_DEFAULT);

  // Display
  tft.init();
  #ifdef TFT_ROTATION
    tft.setRotation(TFT_ROTATION);
  #else
    tft.setRotation(1);
  #endif
  tft.fillScreen(COLOR_BG);
  tft.setTextFont(1);
  tft.setTextSize(1);
  tft.setSwapBytes(true);

  // Buttons
  pinMode(btnRight.pin, INPUT);
  pinMode(btnLeft.pin,  btnLeft.hasPullup ? INPUT_PULLUP : INPUT);

  // Clock + top bar
  Clock_init();
  TopBar_init(&tft);

  // Grill Prob
  GrillProb_begin();
  
  // First paint
  TopBar_redraw(Clock_get(), Weather_getTempStr());
  drawMenu();
}

void loop() {
  Clock_update();
  GrillProb_loop();

  // Auto-refresh top bar
  TopBar_tick(Clock_get(), Weather_getTempStr());

  // Buttons
  updateButton(btnRight);
  updateButton(btnLeft);

  if (consumeShortPress(btnRight)) {
    selectedIndex = (selectedIndex + 1) % MENU_COUNT;
    drawMenu();
  }
  if (consumeShortPress(btnLeft)) {
    selectedIndex = (selectedIndex - 1 + MENU_COUNT) % MENU_COUNT;
    drawMenu();
  }
  
  // debug print Serial Monitor
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 2000) {
  lastPrint = millis();
  auto pd = GrillProb_get();
  Serial.printf("[BLE] connected=%d RSSI=%d meat=%.1f pit=%.1f stale=%d age=%lu ms\n",
                pd.connected, pd.rssi, pd.internalF, pd.ambientF, pd.stale, (unsigned long)pd.ageMs);
}

  // Long press feedback
  static bool lastLongEdge = false;
  bool longNow = isLongPress(btnRight);
  if (longNow && !lastLongEdge) {
    tft.fillRect(0, LIST_TOP - 4 + selectedIndex * LINE_H, tft.width(), LINE_H, 0x07E0);
    tft.setTextDatum(TL_DATUM); tft.setTextColor(0x0000, 0x07E0);
    String line = String("> ") + MENU_ITEMS[selectedIndex];
    tft.drawString(line, 8, LIST_TOP + selectedIndex * LINE_H, 2);
  }
  if (!longNow && lastLongEdge) {
    drawMenu();
    #if DEBUG_SERIAL
      Serial.printf("[UI] Selected: %s\n", MENU_ITEMS[selectedIndex]);
    #endif
  }
  lastLongEdge = longNow;

  delay(8);
}
