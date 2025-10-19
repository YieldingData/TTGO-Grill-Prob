// ==== ClockModule.h ====
#pragma once
#include <Arduino.h>

struct TimeData {
  int h24, h12, m, s;
  String period;   // "AM"/"PM" or "" if 24h
  String city;     // may be empty
};

void Clock_init();
void Clock_update();
TimeData Clock_get();

// Optional helper for UI top bar
String Weather_getTempStr();   // e.g. "72°", "--°" if unknown
