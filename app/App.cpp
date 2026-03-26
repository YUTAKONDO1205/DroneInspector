#include "App.h"

#include <Arduino.h>

// #include "../src/core/config.h"
// #include "../src/core/types.h"
// #include "../src/core/utils.h"

// #include "../src/modules/camera/camera_module.h"
// #include "../src/modules/imu/imu_module.h"
// #include "../src/modules/classifier/classifier_module.h"
// #include "../src/modules/storage/storage_module.h"
// #include "../src/modules/ble/ble_module.h"
// #include "../src/modules/logger/logger_module.h"

#include "../src/core/config.h"
#include "../src/core/types.h"
#include "../src/core/utils.h"

#include "../src/modules/camera/camera_module.h"
#include "../src/modules/imu/imu_module.h"
#include "../src/modules/classifier/classifier_module.h"
#include "../src/modules/storage/storage_module.h"
#include "../src/modules/ble/ble_module.h"
#include "../src/modules/logger/logger_module.h"

// ============================================================
// Global state
// ============================================================
static unsigned long g_lastCaptureMs = 0;
static unsigned long g_lastImuMs = 0;
static uint32_t g_frameId = 0;

static ImuSample g_latestImu = {};
static char g_runId[16] = "RUN01";

// ============================================================
// Internal helper
// ============================================================
static EventRecord makeEventRecord(const InferenceResult& result, unsigned long nowMs) {
  EventRecord rec = {};

  rec.frame_id = g_frameId++;
  rec.ts_ms = nowMs;
  rec.detected = result.detected;
  rec.confidence = result.confidence;
  rec.imu = g_latestImu;

  safeCopyRunId(rec.run_id, sizeof(rec.run_id), g_runId);
  makeImageName(rec.image_name, sizeof(rec.image_name), rec.frame_id);

  return rec;
}

static bool shouldHandleAsEvent(const InferenceResult& result) {
  if (ALWAYS_SAVE_DEBUG) {
    return true;
  }
  return (result.confidence >= CONF_THRESHOLD);
}

// ============================================================
// Public API
// ============================================================
void appSetup() {
  Serial.begin(115200);

  while (!Serial && millis() < 3000) {
    ;
  }

  Serial.println("==================================");
  Serial.println(" DroneInspector boot");
  Serial.println("==================================");

  loggerInfo("Initializing storage...");
  initStorage();
  writeCsvHeaderIfNeeded();

  loggerInfo("Initializing camera...");
  initCamera();

  loggerInfo("Initializing IMU...");
  initImu();

  loggerInfo("Initializing BLE...");
  initBle();

  loggerInfo("Initializing classifier...");
  initClassifier();

  loggerInfo("Setup completed");
}

void appLoop() {
  const unsigned long now = millis();

  // ----------------------------------------------------------
  // 1. IMU periodic update
  // ----------------------------------------------------------
  if (ENABLE_IMU && (now - g_lastImuMs >= IMU_INTERVAL_MS)) {
    g_latestImu = readImuSample();
    g_lastImuMs = now;
  }

  // ----------------------------------------------------------
  // 2. Capture / inference periodic update
  // ----------------------------------------------------------
  if (now - g_lastCaptureMs < CAPTURE_INTERVAL_MS) {
    return;
  }
  g_lastCaptureMs = now;

  loggerDebug("Capture start");

  ImageBuffer img = captureImage();

  // 撮影失敗時のガード
  if (img.data == nullptr || img.size == 0) {
    loggerDebug("Capture failed or empty image");
    releaseImage(img);
    return;
  }

  // 推論
  InferenceResult result = runInference(img);

  // イベントレコード生成
  EventRecord rec = makeEventRecord(result, now);

  // ----------------------------------------------------------
  // 3. Event decision
  // ----------------------------------------------------------
  const bool eventTriggered = shouldHandleAsEvent(result);

  if (!eventTriggered) {
    loggerDebug("No event");
    releaseImage(img);
    return;
  }

  // ----------------------------------------------------------
  // 4. Save / log / notify
  // ----------------------------------------------------------
  if (SAVE_JPEG) {
    saveJpeg(rec.image_name, img);
  }

  if (ENABLE_LOG) {
    appendEventCsv(rec);
  }

  // BLE通知は「検出時」のみに限定
  if (ENABLE_BLE && result.confidence >= CONF_THRESHOLD) {
    notifyBle(rec);
  }

  loggerEvent(rec);

  // ----------------------------------------------------------
  // 5. Release
  // ----------------------------------------------------------
  releaseImage(img);
}
