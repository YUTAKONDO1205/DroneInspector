#include "storage_module.h"

#include <Arduino.h>
#include <SDHCI.h>
#include <stdio.h>
#include <string.h>

#include "../../core/config.h"

// ============================================================
// Spresense microSD storage module
//
// Policy:
// - mount SD once at startup
// - save JPEG bytes as-is
// - append event logs to CSV
// - keep implementation simple and robust first
// ============================================================

namespace {
  SDClass SD;
  bool g_storageReady = false;

  void logStorageError(const char* msg) {
#if ENABLE_SERIAL_DEBUG
    Serial.print("[storage] ");
    Serial.println(msg);
#else
    (void)msg;
#endif
  }

  bool ensureMounted() {
    if (g_storageReady) {
      return true;
    }

    if (!SD.begin()) {
      logStorageError("SD.begin() failed");
      return false;
    }

    g_storageReady = true;
#if ENABLE_SERIAL_DEBUG
    Serial.println("[storage] SD mounted");
#endif
    return true;
  }

  bool fileExists(const char* path) {
    File f = SD.open(path);
    if (!f) {
      return false;
    }
    f.close();
    return true;
  }
}

void initStorage() {
  g_storageReady = false;
  ensureMounted();
}

bool isStorageReady() {
  return g_storageReady;
}

void writeCsvHeaderIfNeeded() {
  if (!ensureMounted()) {
    return;
  }

  if (fileExists(EVENTS_CSV_NAME)) {
#if ENABLE_SERIAL_DEBUG
    Serial.println("[storage] events.csv already exists");
#endif
    return;
  }

  File f = SD.open(EVENTS_CSV_NAME, FILE_WRITE);
  if (!f) {
    logStorageError("failed to create events.csv");
    return;
  }

  f.println("run_id,frame_id,ts_ms,detected,confidence,image_name,ax,ay,az,gx,gy,gz,acc_norm,gyro_norm");
  f.close();

#if ENABLE_SERIAL_DEBUG
  Serial.println("[storage] events.csv header written");
#endif
}

bool saveJpeg(const char* image_name, const ImageBuffer& img) {
  if (!ensureMounted()) {
    return false;
  }

  if (image_name == nullptr || img.data == nullptr || img.size == 0) {
    logStorageError("saveJpeg invalid argument");
    return false;
  }

  File f = SD.open(image_name, FILE_WRITE);
  if (!f) {
#if ENABLE_SERIAL_DEBUG
    Serial.print("[storage] failed to open image file: ");
    Serial.println(image_name);
#endif
    return false;
  }

  size_t written = f.write(img.data, img.size);
  f.close();

  if (written != img.size) {
#if ENABLE_SERIAL_DEBUG
    Serial.print("[storage] JPEG write size mismatch. expected=");
    Serial.print((unsigned long)img.size);
    Serial.print(" actual=");
    Serial.println((unsigned long)written);
#endif
    return false;
  }

#if ENABLE_SERIAL_DEBUG
  Serial.print("[storage] JPEG saved: ");
  Serial.print(image_name);
  Serial.print(" bytes=");
  Serial.println((unsigned long)img.size);
#endif

  return true;
}

bool appendEventCsv(const EventRecord& rec) {
  if (!ensureMounted()) {
    return false;
  }

  File f = SD.open(EVENTS_CSV_NAME, FILE_WRITE);
  if (!f) {
    logStorageError("failed to open events.csv");
    return false;
  }

  char line[256];
  snprintf(
    line,
    sizeof(line),
    "%s,%lu,%lu,%d,%.6f,%s,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f",
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

  f.println(line);
  f.close();

#if ENABLE_SERIAL_DEBUG
  Serial.print("[storage] CSV appended frame=");
  Serial.println((unsigned long)rec.frame_id);
#endif

  return true;
}

void endStorage() {
  // SDClass has no special explicit shutdown path required here
  g_storageReady = false;

#if ENABLE_SERIAL_DEBUG
  Serial.println("[storage] ended");
#endif
}
