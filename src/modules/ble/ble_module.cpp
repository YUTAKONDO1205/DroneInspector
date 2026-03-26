#include "ble_module.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "../../core/config.h"

// ============================================================
// BLE1507 (NUS firmware) over UART2
// ============================================================
// Assumption:
// - BLE1507 is already flashed with NUS firmware
// - Spresense communicates with BLE1507 via Serial2
// - UART setting: 115200, 8N1
//
// Event format example:
// DET,RUN01,12,15324,0.87,IMG_000012.JPG,0.99,0.02
// ============================================================

static bool g_bleInitialized = false;

void initBle() {
  if (!ENABLE_BLE) {
    return;
  }

  Serial2.begin(115200, SERIAL_8N1);
  delay(100);

  g_bleInitialized = true;

#if ENABLE_SERIAL_DEBUG
  Serial.println("[ble] BLE1507 NUS via Serial2 initialized");
#endif
}

bool bleWriteRaw(const uint8_t* data, size_t len) {
  if (!ENABLE_BLE || !g_bleInitialized || data == nullptr || len == 0) {
    return false;
  }

  size_t written = Serial2.write(data, len);
  Serial2.flush();

  return (written == len);
}

bool bleWriteLine(const char* line) {
  if (!ENABLE_BLE || !g_bleInitialized || line == nullptr) {
    return false;
  }

  size_t len = strlen(line);
  size_t written1 = Serial2.write(reinterpret_cast<const uint8_t*>(line), len);
  size_t written2 = Serial2.write('\n');
  Serial2.flush();

  return (written1 == len && written2 == 1);
}

bool notifyBle(const EventRecord& rec) {
  if (!ENABLE_BLE || !g_bleInitialized) {
    return false;
  }

  char msg[160];

  // DET,RUN01,12,15324,0.87,IMG_000012.JPG,0.99,0.02
  snprintf(
    msg,
    sizeof(msg),
    "DET,%s,%lu,%lu,%.4f,%s,%.4f,%.4f",
    rec.run_id,
    static_cast<unsigned long>(rec.frame_id),
    static_cast<unsigned long>(rec.ts_ms),
    rec.confidence,
    rec.image_name,
    rec.imu.acc_norm,
    rec.imu.gyro_norm
  );

#if ENABLE_SERIAL_DEBUG
  Serial.print("[ble] tx: ");
  Serial.println(msg);
#endif

  return bleWriteLine(msg);
}

void blePoll() {
  if (!ENABLE_BLE || !g_bleInitialized) {
    return;
  }

  // Optional:
  // Forward received data from BLE side to USB serial monitor.
  // Useful for debugging smartphone/app messages.
  while (Serial2.available() > 0) {
    int c = Serial2.read();

#if ENABLE_SERIAL_DEBUG
    Serial.write(c);
#endif
  }
}
