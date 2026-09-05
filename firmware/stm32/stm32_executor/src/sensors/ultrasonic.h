#pragma once
#include <Arduino.h>
#include "../../config.h"

// 超声波测距（原 sketch L1965-1983 原样搬运）。
extern bool ultrasonicEnabled;

bool readDistanceCm(float &distanceCm);
