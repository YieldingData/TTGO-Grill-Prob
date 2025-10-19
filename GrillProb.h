// ==== GrillProb.h ====
#pragma once
#include <Arduino.h>

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

void GrillProb_begin();
void GrillProb_loop();             // call frequently from loop()
ProbeData GrillProb_get();         // snapshot of latest values
