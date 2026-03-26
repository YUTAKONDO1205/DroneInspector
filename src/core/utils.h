#pragma once

#include <stddef.h>
#include <stdint.h>

void makeImageName(char* out, size_t out_size, uint32_t frame_id);
void safeCopyRunId(char* dst, size_t dst_size, const char* src);
float calcNorm3(float x, float y, float z);
float calcNorm3(float x, float y, float z);