#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>

// ============================================================
// IMU sample
// ============================================================
struct ImuSample {
  float ax = 0.0f;
  float ay = 0.0f;
  float az = 0.0f;

  float gx = 0.0f;
  float gy = 0.0f;
  float gz = 0.0f;

  float acc_norm = 0.0f;
  float gyro_norm = 0.0f;

  unsigned long ts_ms = 0;
};

// ============================================================
// Inference result
// ============================================================
struct InferenceResult {
  bool detected = false;
  float confidence = 0.0f;
  unsigned long ts_ms = 0;
};

// ============================================================
// Image buffer
// ============================================================
struct ImageBuffer {
  uint8_t* data = nullptr;
  size_t size = 0;

  uint16_t width = 0;
  uint16_t height = 0;
};

// ============================================================
// Event record
// ============================================================
struct EventRecord {
  char run_id[16] = {0};

  uint32_t frame_id = 0;
  unsigned long ts_ms = 0;

  bool detected = false;
  float confidence = 0.0f;

  ImuSample imu = {};

  char image_name[32] = {0};
};
