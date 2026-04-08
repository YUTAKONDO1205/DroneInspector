#pragma once
#include "../../core/types.h"

void initCamera();
bool isCameraReady();

ImageBuffer captureJpeg();
ImageBuffer captureInferenceFrame();
ImageBuffer captureImage();  // 既存コード向けの JPEG 取得互換エイリアス。

void releaseImage(ImageBuffer& img);
void endCamera();
