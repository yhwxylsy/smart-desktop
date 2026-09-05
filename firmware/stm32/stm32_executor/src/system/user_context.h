#pragma once
#include <Arduino.h>
#include "../../config.h"

// 用户上下文（原 sketch L276-278、L2332-2346 原样搬运）。
extern String currentUserId;
extern String currentCardUid;
extern String currentUserMode;

bool handleUserContextCommand(const String &payload);
