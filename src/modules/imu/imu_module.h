#pragma once

#include "../../core/types.h"

void initImu();
bool isImuReady();
ImuSample readImuSample();
void endImu();
