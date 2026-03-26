#pragma once

// ============================================================
// System behavior
// ============================================================
#define CONF_THRESHOLD          0.30f
#define CAPTURE_INTERVAL_MS     1000UL
#define IMU_INTERVAL_MS         50UL

#define SAVE_JPEG               true
#define ENABLE_BLE              true
#define ENABLE_IMU              true
#define ENABLE_LOG              true

// ============================================================
// Debug / development
// ============================================================
#define ALWAYS_SAVE_DEBUG       false
#define ENABLE_SERIAL_DEBUG     true

// ============================================================
// Model input
// ============================================================
#define INPUT_WIDTH             128
#define INPUT_HEIGHT            128
#define INPUT_CHANNELS          3

// ============================================================
// File / storage settings
// ============================================================
#define EVENTS_CSV_NAME         "events.csv"
#define IMAGE_NAME_PREFIX       "IMG_"
#define IMAGE_NAME_EXT          ".JPG"
#define RUN_ID_DEFAULT          "RUN01"

// ============================================================
// BLE
// ============================================================
#define BLE_DEVICE_NAME         "DroneInspector"

// ============================================================
// Logger
// ============================================================
#define SERIAL_BAUDRATE         115200
