#include "App.h"

#include <Arduino.h>

#include "../src/core/config.h"
#include "../src/core/types.h"
#include "../src/core/utils.h"

#include "../src/modules/ble/ble_module.h"
#include "../src/modules/camera/camera_module.h"
#include "../src/modules/classifier/classifier_module.h"
#include "../src/modules/imu/imu_module.h"
#include "../src/modules/logger/logger_module.h"
#include "../src/modules/storage/storage_module.h"

// ============================================================
// グローバル状態
// ============================================================
static unsigned long g_lastCaptureMs = 0;
static unsigned long g_lastImuMs = 0;
static uint32_t g_frameId = 0;

static ImuSample g_latestImu = {};
static char g_runId[16] = RUN_ID_DEFAULT;
static bool g_loggedCaptureFallback = false;

// ============================================================
// 内部ヘルパー
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

static bool shouldPersistCapture(const InferenceResult& result, bool hasInferenceFrame) {
  if (!hasInferenceFrame) {
    return ENABLE_CAPTURE_FALLBACK || ALWAYS_SAVE_DEBUG;
  }

  return shouldHandleAsEvent(result);
}

static void logModuleStatus(const char* name, bool ready) {
  Serial.print("[setup] ");
  Serial.print(name);
  Serial.print(": ");
  Serial.println(ready ? "ready" : "unavailable");
}

// ============================================================
// 公開 API
// ============================================================
void appSetup() {
  Serial.begin(SERIAL_BAUDRATE);

  while (!Serial && millis() < 3000) {
    ;
  }

  safeCopyRunId(g_runId, sizeof(g_runId), RUN_ID_DEFAULT);

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

  logModuleStatus("storage", isStorageReady());
  logModuleStatus("camera", isCameraReady());
  logModuleStatus("imu", isImuReady());
  logModuleStatus("ble", isBleReady());
  logModuleStatus("classifier", isClassifierReady());

  if (!isClassifierReady()) {
    loggerInfo("Classifier is not ready. Capture fallback mode keeps JPEG/IMU logging active.");
  }

  loggerInfo("Setup completed");
}

void appLoop() {
  const unsigned long now = millis();

  blePoll();

  // ----------------------------------------------------------
  // 1. IMU の定期更新
  // ----------------------------------------------------------
  if (ENABLE_IMU && (now - g_lastImuMs >= IMU_INTERVAL_MS)) {
    g_latestImu = readImuSample();
    g_lastImuMs = now;
  }

  // ----------------------------------------------------------
  // 2. 画像取得 / 推論の定期更新
  // ----------------------------------------------------------
  if (now - g_lastCaptureMs < CAPTURE_INTERVAL_MS) {
    return;
  }
  g_lastCaptureMs = now;

  loggerDebug("Capture start");

  ImageBuffer jpeg = captureJpeg();
  if (jpeg.data == nullptr || jpeg.size == 0) {
    loggerDebug("Capture failed or empty image");
    releaseImage(jpeg);
    return;
  }

  InferenceResult result = {};
  ImageBuffer inferenceFrame = captureInferenceFrame();
  const bool hasInferenceFrame =
    (inferenceFrame.data != nullptr && inferenceFrame.size > 0);

  if (hasInferenceFrame) {
    result = runInference(inferenceFrame);
  } else if (ENABLE_CAPTURE_FALLBACK && !g_loggedCaptureFallback) {
    loggerInfo("Inference frame is unavailable. Saving captures in fallback mode.");
    g_loggedCaptureFallback = true;
  }

  EventRecord rec = makeEventRecord(result, now);

  // ----------------------------------------------------------
  // 3. イベント判定
  // ----------------------------------------------------------
  const bool eventTriggered = shouldPersistCapture(result, hasInferenceFrame);

  if (!eventTriggered) {
    loggerDebug("No event");
    if (inferenceFrame.data != nullptr && inferenceFrame.data != jpeg.data) {
      releaseImage(inferenceFrame);
    }
    releaseImage(jpeg);
    return;
  }

  // ----------------------------------------------------------
  // 4. 保存 / ログ / 通知
  // ----------------------------------------------------------
  if (SAVE_JPEG && !saveJpeg(rec.image_name, jpeg)) {
    loggerDebug("saveJpeg failed");
  }

  if (ENABLE_LOG && !appendEventCsv(rec)) {
    loggerDebug("appendEventCsv failed");
  }

  if (ENABLE_BLE && hasInferenceFrame && result.confidence >= CONF_THRESHOLD) {
    if (!notifyBle(rec)) {
      loggerDebug("notifyBle failed");
    }
  }

  loggerEvent(rec);

  // ----------------------------------------------------------
  // 5. 解放
  // ----------------------------------------------------------
  if (inferenceFrame.data != nullptr && inferenceFrame.data != jpeg.data) {
    releaseImage(inferenceFrame);
  }
  releaseImage(jpeg);
}
