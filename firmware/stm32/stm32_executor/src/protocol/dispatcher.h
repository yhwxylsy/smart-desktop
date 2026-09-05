#pragma once
#include <Arduino.h>
#include "../../config.h"

// 命令执行（原 sketch L2348-2537 原样搬运）。
bool executeNetCommand(const String &command);
