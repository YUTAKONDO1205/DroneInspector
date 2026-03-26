#pragma once

#include "../../core/types.h"

void initClassifier();
bool isClassifierReady();
InferenceResult runInference(const ImageBuffer& img);
void endClassifier();
