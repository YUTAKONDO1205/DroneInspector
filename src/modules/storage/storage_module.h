#pragma once

#include "../../core/types.h"

void initStorage();
bool isStorageReady();
void writeCsvHeaderIfNeeded();

bool saveJpeg(const char* image_name, const ImageBuffer& img);
bool appendEventCsv(const EventRecord& rec);

void endStorage();
