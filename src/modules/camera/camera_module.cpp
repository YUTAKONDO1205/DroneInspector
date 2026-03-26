#include "camera_module.h"

#include <Arduino.h>
#include <Camera.h>
#include <stdlib.h>
#include <string.h>

#include "../../core/config.h"

// ============================================================
// Camera module for Spresense HDR Camera Board
// Current policy:
// - Use still JPEG capture first
// - Keep implementation simple and stable
// - Copy camera buffer into heap memory so the caller can own it
// ============================================================

namespace {
  bool g_cameraReady = false;

  // Start conservative.
  // QVGA is light enough for early integration and debugging.
  constexpr int kStillWidth  = CAM_IMGSIZE_QVGA_H;   // 320
  constexpr int kStillHeight = CAM_IMGSIZE_QVGA_V;   // 240

  void printCamError(const char* label, CamErr err) {
#if ENABLE_SERIAL_DEBUG
    Serial.print("[camera] ");
    Serial.print(label);
    Serial.print(" failed. err=");
    Serial.println(static_cast<int>(err));
#else
    (void)label;
    (void)err;
#endif
  }
}

void initCamera() {
  if (g_cameraReady) {
    return;
  }

  CamErr err = theCamera.begin();
  if (err != CAM_ERR_SUCCESS) {
    printCamError("begin", err);
    g_cameraReady = false;
    return;
  }

  // JPEG still image format
  err = theCamera.setStillPictureImageFormat(
    kStillWidth,
    kStillHeight,
    CAM_IMAGE_PIX_FMT_JPG
  );

  if (err != CAM_ERR_SUCCESS) {
    printCamError("setStillPictureImageFormat", err);
    theCamera.end();
    g_cameraReady = false;
    return;
  }

#if ENABLE_SERIAL_DEBUG
  Serial.print("[camera] ready. still=");
  Serial.print(kStillWidth);
  Serial.print("x");
  Serial.println(kStillHeight);
#endif

  g_cameraReady = true;
}

bool isCameraReady() {
  return g_cameraReady;
}

ImageBuffer captureImage() {
  ImageBuffer out = {};

  if (!g_cameraReady) {
#if ENABLE_SERIAL_DEBUG
    Serial.println("[camera] capture skipped: camera not ready");
#endif
    return out;
  }

  CamImage img = theCamera.takePicture();

  if (!img.isAvailable()) {
#if ENABLE_SERIAL_DEBUG
    Serial.println("[camera] takePicture returned unavailable image");
#endif
    return out;
  }

  const uint8_t* src = reinterpret_cast<const uint8_t*>(img.getImgBuff());
  const size_t srcSize = img.getImgSize();

  if (src == nullptr || srcSize == 0) {
#if ENABLE_SERIAL_DEBUG
    Serial.println("[camera] invalid image buffer");
#endif
    return out;
  }

  uint8_t* copied = static_cast<uint8_t*>(malloc(srcSize));
  if (copied == nullptr) {
#if ENABLE_SERIAL_DEBUG
    Serial.println("[camera] malloc failed");
#endif
    return out;
  }

  memcpy(copied, src, srcSize);

  out.data = copied;
  out.size = srcSize;
  out.width = kStillWidth;
  out.height = kStillHeight;

#if ENABLE_SERIAL_DEBUG
  Serial.print("[camera] captured JPEG bytes=");
  Serial.println(static_cast<unsigned long>(out.size));
#endif

  return out;
}

void releaseImage(ImageBuffer& img) {
  if (img.data != nullptr) {
    free(img.data);
    img.data = nullptr;
  }

  img.size = 0;
  img.width = 0;
  img.height = 0;
}

void endCamera() {
  if (!g_cameraReady) {
    return;
  }

  theCamera.end();
  g_cameraReady = false;

#if ENABLE_SERIAL_DEBUG
  Serial.println("[camera] ended");
#endif
}
