// ==== TopBar.h ====
// Modular top bar for TTGO T-Display
// Line 1: time (left) + temp (right)
// Line 2: city + "Bat XX%" (with truncation & fit logic)
// Handles battery sampling, smoothing, and timed redraws.
//
// Usage:
//   #include "TopBar.h"
//   TopBar_init(&tft);
//   ... in loop():
//     TopBar_tick(Clock_get(), Weather_getTempStr());  // auto-refresh every N ms
//   ... when you need to fully repaint after menu changes:
//     TopBar_redraw(Clock_get(), Weather_getTempStr());
//
#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "Config.h"
#include "ClockModule.h"

#ifndef UI_REDRAW_TOPBAR_MS
  #define UI_REDRAW_TOPBAR_MS 1000UL
#endif

#ifndef TOPBAR_H
  #define TOPBAR_H 22
#endif

#ifndef COLOR_TOPBAR
  #define COLOR_TOPBAR 0x2104
#endif
#ifndef COLOR_TEXT
  #define COLOR_TEXT 0xFFFF
#endif
#ifndef COLOR_FRAME
  #define COLOR_FRAME 0x39E7
#endif

// Battery config with sane defaults (override in Config.h)
#ifndef BATTERY_ADC_PIN
  #define BATTERY_ADC_PIN 34
#endif
#ifndef BATTERY_VDIV
  #define BATTERY_VDIV 2.00f
#endif
#ifndef BATTERY_MIN_V
  #define BATTERY_MIN_V 3.30f
#endif
#ifndef BATTERY_MAX_V
  #define BATTERY_MAX_V 4.20f
#endif
#ifndef BATTERY_SAMPLES
  #define BATTERY_SAMPLES 8
#endif
#ifndef BATTERY_ALPHA
  #define BATTERY_ALPHA 0.25f
#endif
#ifndef BATTERY_HYSTERESIS_PCT
  #define BATTERY_HYSTERESIS_PCT 2
#endif

void TopBar_init(TFT_eSPI* tft);
void TopBar_tick(const TimeData& td, const String& tempStr);
void TopBar_redraw(const TimeData& td, const String& tempStr);
