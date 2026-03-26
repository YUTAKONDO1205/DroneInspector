#include "classifier_module.h"

#include <Arduino.h>
#include <math.h>
#include <string.h>

#include "../../core/config.h"
#include "../../../models/model_data.h"

// ------------------------------------------------------------
// TensorFlow Lite Micro
// ------------------------------------------------------------
#include <TensorFlowLite.h>
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"

// ============================================================
// Notes
// ============================================================
// 1) This implementation currently expects RAW RGB888 input
//    of size INPUT_WIDTH * INPUT_HEIGHT * INPUT_CHANNELS.
//
// 2) If your current camera module returns JPEG bytes,
//    preprocess will fail safely and return confidence=0.
//
// 3) Once camera side is changed to preview/raw capture
//    or JPEG decode is added, this classifier can run as-is.
//
// 4) Depending on the model, tensor arena size may need tuning.
// ============================================================

namespace {
  bool g_classifierReady = false;

  constexpr size_t kExpectedInputBytes =
      INPUT_WIDTH * INPUT_HEIGHT * INPUT_CHANNELS;

  // Adjust if AllocateTensors() fails.
  // Start with a relatively safe value for early integration.
  constexpr size_t kTensorArenaSize = 512 * 1024;

  alignas(16) uint8_t g_tensorArena[kTensorArenaSize];

  const tflite::Model* g_model = nullptr;
  tflite::MicroInterpreter* g_interpreter = nullptr;
  TfLiteTensor* g_inputTensor = nullptr;
  TfLiteTensor* g_outputTensor = nullptr;

  // AllOpsResolver is easiest for now.
  // Later, if flash becomes tight, replace with MicroMutableOpResolver.
  tflite::AllOpsResolver g_resolver;

  // Use static storage to avoid dynamic lifetime issues
  tflite::MicroInterpreter* createInterpreter() {
    static tflite::MicroInterpreter staticInterpreter(
      g_model,
      g_resolver,
      g_tensorArena,
      kTensorArenaSize
    );
    return &staticInterpreter;
  }

  void logTensorInfo() {
#if ENABLE_SERIAL_DEBUG
    if (g_inputTensor != nullptr) {
      Serial.print("[classifier] input type=");
      Serial.println(g_inputTensor->type);

      Serial.print("[classifier] input dims=");
      for (int i = 0; i < g_inputTensor->dims->size; ++i) {
        Serial.print(g_inputTensor->dims->data[i]);
        if (i < g_inputTensor->dims->size - 1) {
          Serial.print("x");
        }
      }
      Serial.println();
    }

    if (g_outputTensor != nullptr) {
      Serial.print("[classifier] output type=");
      Serial.println(g_outputTensor->type);
    }
#endif
  }

  bool inputShapeLooksValid() {
    if (g_inputTensor == nullptr || g_inputTensor->dims == nullptr) {
      return false;
    }

    // Typically [1, H, W, C]
    if (g_inputTensor->dims->size != 4) {
      return false;
    }

    const int h = g_inputTensor->dims->data[1];
    const int w = g_inputTensor->dims->data[2];
    const int c = g_inputTensor->dims->data[3];

    return (h == INPUT_HEIGHT && w == INPUT_WIDTH && c == INPUT_CHANNELS);
  }

  bool fillInputTensorFromRawRgb(const ImageBuffer& img) {
    if (img.data == nullptr) {
      return false;
    }

    // For now: expect raw RGB888 exactly matching model input size
    if (img.size != kExpectedInputBytes) {
#if ENABLE_SERIAL_DEBUG
      Serial.print("[classifier] unsupported input size. expected raw RGB bytes=");
      Serial.print((unsigned long)kExpectedInputBytes);
      Serial.print(" actual=");
      Serial.println((unsigned long)img.size);
      Serial.println("[classifier] hint: current camera likely returns JPEG.");
#endif
      return false;
    }

    switch (g_inputTensor->type) {
      case kTfLiteFloat32: {
        float* dst = g_inputTensor->data.f;
        for (size_t i = 0; i < kExpectedInputBytes; ++i) {
          dst[i] = static_cast<float>(img.data[i]) / 255.0f;
        }
        return true;
      }

      case kTfLiteUInt8: {
        memcpy(g_inputTensor->data.uint8, img.data, kExpectedInputBytes);
        return true;
      }

      case kTfLiteInt8: {
        int8_t* dst = g_inputTensor->data.int8;
        const float scale = g_inputTensor->params.scale;
        const int zeroPoint = g_inputTensor->params.zero_point;

        if (scale == 0.0f) {
          return false;
        }

        for (size_t i = 0; i < kExpectedInputBytes; ++i) {
          const float x = static_cast<float>(img.data[i]) / 255.0f;
          int32_t q = static_cast<int32_t>(roundf(x / scale)) + zeroPoint;

          if (q < -128) q = -128;
          if (q > 127)  q = 127;

          dst[i] = static_cast<int8_t>(q);
        }
        return true;
      }

      default:
#if ENABLE_SERIAL_DEBUG
        Serial.print("[classifier] unsupported input tensor type=");
        Serial.println(g_inputTensor->type);
#endif
        return false;
    }
  }

  float readOutputConfidence() {
    if (g_outputTensor == nullptr) {
      return 0.0f;
    }

    switch (g_outputTensor->type) {
      case kTfLiteFloat32:
        return g_outputTensor->data.f[0];

      case kTfLiteUInt8: {
        const float scale = g_outputTensor->params.scale;
        const int zeroPoint = g_outputTensor->params.zero_point;
        const uint8_t q = g_outputTensor->data.uint8[0];
        return (static_cast<int>(q) - zeroPoint) * scale;
      }

      case kTfLiteInt8: {
        const float scale = g_outputTensor->params.scale;
        const int zeroPoint = g_outputTensor->params.zero_point;
        const int8_t q = g_outputTensor->data.int8[0];
        return (static_cast<int>(q) - zeroPoint) * scale;
      }

      default:
#if ENABLE_SERIAL_DEBUG
        Serial.print("[classifier] unsupported output tensor type=");
        Serial.println(g_outputTensor->type);
#endif
        return 0.0f;
    }
  }

  float clamp01(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
  }
}

void initClassifier() {
  if (g_classifierReady) {
    return;
  }

  if (g_model_data_len <= 1) {
#if ENABLE_SERIAL_DEBUG
    Serial.println("[classifier] model_data is placeholder. generate real model_data.cpp first.");
#endif
    g_classifierReady = false;
    return;
  }

  g_model = tflite::GetModel(g_model_data);
  if (g_model == nullptr) {
#if ENABLE_SERIAL_DEBUG
    Serial.println("[classifier] GetModel failed");
#endif
    g_classifierReady = false;
    return;
  }

  if (g_model->version() != TFLITE_SCHEMA_VERSION) {
#if ENABLE_SERIAL_DEBUG
    Serial.print("[classifier] schema mismatch. model=");
    Serial.print(g_model->version());
    Serial.print(" runtime=");
    Serial.println(TFLITE_SCHEMA_VERSION);
#endif
    g_classifierReady = false;
    return;
  }

  g_interpreter = createInterpreter();
  if (g_interpreter == nullptr) {
#if ENABLE_SERIAL_DEBUG
    Serial.println("[classifier] interpreter creation failed");
#endif
    g_classifierReady = false;
    return;
  }

  TfLiteStatus allocStatus = g_interpreter->AllocateTensors();
  if (allocStatus != kTfLiteOk) {
#if ENABLE_SERIAL_DEBUG
    Serial.println("[classifier] AllocateTensors failed");
    Serial.print("[classifier] try increasing tensor arena size. current=");
    Serial.println((unsigned long)kTensorArenaSize);
#endif
    g_classifierReady = false;
    return;
  }

  g_inputTensor = g_interpreter->input(0);
  g_outputTensor = g_interpreter->output(0);

  if (g_inputTensor == nullptr || g_outputTensor == nullptr) {
#if ENABLE_SERIAL_DEBUG
    Serial.println("[classifier] input/output tensor acquisition failed");
#endif
    g_classifierReady = false;
    return;
  }

  if (!inputShapeLooksValid()) {
#if ENABLE_SERIAL_DEBUG
    Serial.println("[classifier] input shape does not match config.");
#endif
    logTensorInfo();
    g_classifierReady = false;
    return;
  }

  logTensorInfo();

#if ENABLE_SERIAL_DEBUG
  Serial.println("[classifier] ready");
#endif

  g_classifierReady = true;
}

bool isClassifierReady() {
  return g_classifierReady;
}

InferenceResult runInference(const ImageBuffer& img) {
  InferenceResult result = {};
  result.ts_ms = millis();
  result.confidence = 0.0f;
  result.detected = false;

  if (!g_classifierReady) {
#if ENABLE_SERIAL_DEBUG
    Serial.println("[classifier] not ready");
#endif
    return result;
  }

  if (!fillInputTensorFromRawRgb(img)) {
#if ENABLE_SERIAL_DEBUG
    Serial.println("[classifier] preprocess failed");
#endif
    return result;
  }

  TfLiteStatus invokeStatus = g_interpreter->Invoke();
  if (invokeStatus != kTfLiteOk) {
#if ENABLE_SERIAL_DEBUG
    Serial.println("[classifier] Invoke failed");
#endif
    return result;
  }

  float confidence = readOutputConfidence();
  confidence = clamp01(confidence);

  result.confidence = confidence;
  result.detected = (confidence >= CONF_THRESHOLD);

#if ENABLE_SERIAL_DEBUG
  Serial.print("[classifier] confidence=");
  Serial.println(result.confidence, 6);
#endif

  return result;
}

void endClassifier() {
  // TFLM objects are static in this implementation.
  // Nothing explicit to free here.
  g_classifierReady = false;
  g_inputTensor = nullptr;
  g_outputTensor = nullptr;
}
