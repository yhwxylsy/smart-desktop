#include "user_context.h"
#include "../../config.h"
#include "../core/text_util.h"
#include "../ui/oled.h"
#include "../ui/ui_state.h"

String currentUserId = "-";
String currentCardUid = "-";
String currentUserMode = "NONE";

bool handleUserContextCommand(const String &payload) {
  int offset = 0;
  currentUserId = nextColonField(payload, offset);
  currentCardUid = nextColonField(payload, offset);
  currentUserMode = nextColonField(payload, offset);
  currentUserMode.toUpperCase();
  if (currentUserId == "-" || currentUserMode == "NONE" || currentUserMode == "DENIED") {
    if (currentUserMode != "DENIED") {
      currentCardUid = "-";
    }
  }
  uiEvent.detail = String("USER ") + compactForDisplay(currentUserId, 14);
  oledRenderPending = true;
  return true;
}
