#include "logger_module.h"

#include <Arduino.h>
#include <stdio.h>

#include "../../core/config.h"

// ============================================================
// Logger module
// ============================================================
// Policy:
// - Keep it simple
// - Use USB serial output only
// - Respect ENABLE_SERIAL_DEBUG for noisy logs
// ============================================================

namespace {
  void printPrefix(const char* level) {
    Serial.print("[");
    Serial.print(level);
    Serial.print("] ");
  }
}

void loggerInfo(const char* msg) {
  if (msg == nullptr) {
    return;
  }

  printPrefix("INFO");
  Serial.println(msg);
}

void loggerDebug(const char* msg) {
  if (!ENABLE_SERIAL_DEBUG || msg == nullptr) {
    return;
  }

  printPrefix("DEBUG");
  Serial.println(msg);
}

void loggerEvent(const EventRecord& rec) {
  char line[256];

  snprintf(
    line,
    sizeof(line),
    "run=%s frame=%lu ts=%lu detected=%d conf=%.6f img=%s "
    "ax=%.4f ay=%.4f az=%.4f gx=%.4f gy=%.4f gz=%.4f "
    "acc_norm=%.4f gyro_norm=%.4f",
    rec.run_id,
    static_cast<unsigned long>(rec.frame_id),
    static_cast<unsigned long>(rec.ts_ms),
    rec.detected ? 1 : 0,
    rec.confidence,
    rec.image_name,
    rec.imu.ax,
    rec.imu.ay,
    rec.imu.az,
    rec.imu.gx,
    rec.imu.gy,
    rec.imu.gz,
    rec.imu.acc_norm,
    rec.imu.gyro_norm
  );

  printPrefix("EVENT");
  Serial.println(line);
}
