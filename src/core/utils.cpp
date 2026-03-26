#include "utils.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "config.h"

void makeImageName(char* out, size_t out_size, uint32_t frame_id) {
  if (out == nullptr || out_size == 0) {
    return;
  }

  snprintf(
    out,
    out_size,
    "%s%06lu%s",
    IMAGE_NAME_PREFIX,
    static_cast<unsigned long>(frame_id),
    IMAGE_NAME_EXT
  );
}

void safeCopyRunId(char* dst, size_t dst_size, const char* src) {
  if (dst == nullptr || dst_size == 0) {
    return;
  }

  if (src == nullptr) {
    dst[0] = '\0';
    return;
  }

  strncpy(dst, src, dst_size - 1);
  dst[dst_size - 1] = '\0';
}

float calcNorm3(float x, float y, float z) {
  return sqrtf(x * x + y * y + z * z);
}
