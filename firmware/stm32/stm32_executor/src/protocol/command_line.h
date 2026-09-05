#pragma once
#include <Arduino.h>
#include "../../config.h"
#include "../ui/rgb.h"
#include "../ui/ui_state.h"
#include "protocol.h"

// 命令入口与粘包/前缀处理（原 sketch L593-681、L683-807、L2539-2617 原样搬运）。
// 去耦合阶段将把 classifyNetCommand/commandPreview/isKnownNetCommandStart 前缀表
// 与 dispatcher 的命令分支合并为单一 command_table。
bool stringMatchesAt(const String &text, int index, const char *prefix);
bool isKnownNetCommandStart(const String &text, int index);
int findKnownNetCommandStart(const String &text, int fromIndex);
UiEventType classifyNetCommand(const String &command, const String &actionId);
String commandPreview(const String &command);
void handleLine(String line, const char *source);
