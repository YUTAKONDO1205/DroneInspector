#pragma once
#include "../../core/types.h"

void initCamera();
bool isCameraReady();

ImageBuffer captureInferenceFrame(); // raw RGB888, 128x128x3
ImageBuffer captureJpeg();           // JPEG bytes for saving

void releaseImage(ImageBuffer& img);
void endCamera();
