#pragma once

// ============================================================
// システム動作
// ============================================================
#define CONF_THRESHOLD          0.30f
#define CAPTURE_INTERVAL_MS     1000UL
#define IMU_INTERVAL_MS         50UL

#define SAVE_JPEG               true
#define ENABLE_BLE              true
#define ENABLE_IMU              true
#define ENABLE_LOG              true

// ============================================================
// デバッグ / 開発
// ============================================================
#define ALWAYS_SAVE_DEBUG       false
#define ENABLE_SERIAL_DEBUG     true
#define ENABLE_CAPTURE_FALLBACK true

// ============================================================
// モデル入力
// ============================================================
// uav_model.py から出力する MobileNetV2 は 160x160 入力で学習している。
#define INPUT_WIDTH             160
#define INPUT_HEIGHT            160
#define INPUT_CHANNELS          3

// ============================================================
// ファイル / 保存設定
// ============================================================
#define EVENTS_CSV_NAME         "events.csv"
#define IMAGE_NAME_PREFIX       "IMG_"
#define IMAGE_NAME_EXT          ".JPG"
#define RUN_ID_DEFAULT          "RUN01"

// ============================================================
// BLE 設定
// ============================================================
#define BLE_DEVICE_NAME         "DroneInspector"

// ============================================================
// ロガー
// ============================================================
#define SERIAL_BAUDRATE         115200
