#include "hex_util.h"
#include <Arduino.h>

String utf8Hex(const String &text) {
  static const char *HEX_DIGITS = "0123456789ABCDEF";
  String encoded;
  encoded.reserve(text.length() * 2);
  for (unsigned int i = 0; i < text.length(); i++) {
    uint8_t value = (uint8_t)text.charAt(i);
    encoded += HEX_DIGITS[(value >> 4) & 0x0F];
    encoded += HEX_DIGITS[value & 0x0F];
  }
  return encoded;
}
