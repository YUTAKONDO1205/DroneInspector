#pragma once
#include "../../core/types.h"

void initBle();
bool isBleReady();
bool notifyBle(const EventRecord& rec);
bool bleWriteLine(const char* line);
bool bleWriteRaw(const uint8_t* data, size_t len);
void blePoll();
