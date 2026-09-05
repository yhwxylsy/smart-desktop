#include "text_util.h"
#include "../../config.h"

String upperCopy(const String &text) {
  String result = text;
  result.toUpperCase();
  return result;
}

bool containsUpperToken(const String &text, const char *token) {
  return upperCopy(text).indexOf(token) >= 0;
}

String compactForDisplay(const String &text, uint8_t maxLen) {
  String out;
  bool lastReplacement = false;
  for (unsigned int i = 0; i < text.length() && out.length() < maxLen; ++i) {
    uint8_t ch = (uint8_t)text.charAt(i);
    if (ch >= 32 && ch <= 126) {
      out += (char)ch;
      lastReplacement = false;
    } else if (!lastReplacement) {
      out += '?';
      lastReplacement = true;
    }
  }
  return out;
}

int hexNibble(char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  }
  if (ch >= 'a' && ch <= 'f') {
    return 10 + ch - 'a';
  }
  if (ch >= 'A' && ch <= 'F') {
    return 10 + ch - 'A';
  }
  return -1;
}

String nextColonField(const String &text, int &offset) {
  int next = text.indexOf(':', offset);
  String value = next >= 0 ? text.substring(offset, next) : text.substring(offset);
  offset = next >= 0 ? next + 1 : text.length();
  value.trim();
  return value.length() > 0 ? value : "-";
}
