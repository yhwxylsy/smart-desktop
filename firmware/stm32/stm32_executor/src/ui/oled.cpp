#include "oled.h"
#include <string.h>
#include <Wire.h>
#include "../../config.h"
#include "../core/board.h"
#include "fonts/oled_font5x7.h"
#include "fonts/oled_cjk16.h"

uint8_t oledAddress = OLED_ADDR_PRIMARY;
uint8_t oledBuffer[OLED_WIDTH * OLED_PAGES];
uint8_t oledFlushPage = 0;
bool oledAvailable = false;
bool oledDirty = false;
bool oledRenderPending = false;
uint32_t lastUiFrameMs = 0;
uint32_t lastOledFlushMs = 0;
uint32_t oledFpsWindowMs = 0;
uint16_t oledFrameCounter = 0;
uint8_t oledFps = 0;

void clearOledBuffer() {
  memset(oledBuffer, 0, sizeof(oledBuffer));
}

void oledSetPixel(uint8_t x, uint8_t y, bool on) {
  if (x >= OLED_WIDTH || y >= OLED_HEIGHT) {
    return;
  }
  uint16_t index = x + (uint16_t)(y / 8) * OLED_WIDTH;
  uint8_t mask = 1 << (y & 0x07);
  if (on) {
    oledBuffer[index] |= mask;
  } else {
    oledBuffer[index] &= ~mask;
  }
}

void oledFillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool on) {
  for (uint8_t yy = y; yy < y + h && yy < OLED_HEIGHT; ++yy) {
    for (uint8_t xx = x; xx < x + w && xx < OLED_WIDTH; ++xx) {
      oledSetPixel(xx, yy, on);
    }
  }
}

void oledDrawChar(uint8_t x, uint8_t y, char ch, bool inverted) {
  if ((uint8_t)ch < 32 || (uint8_t)ch > 126) {
    ch = '?';
  }
  const uint8_t *glyph = OLED_FONT_5X7[(uint8_t)ch - 32];
  for (uint8_t col = 0; col < 6; ++col) {
    uint8_t bits = (col < 5) ? glyph[col] : 0x00;
    for (uint8_t row = 0; row < 8; ++row) {
      bool on = (row < 7) && ((bits & (1 << row)) != 0);
      if (inverted) {
        on = !on;
      }
      oledSetPixel(x + col, y + row, on);
    }
  }
}

void oledDrawText(uint8_t x, uint8_t y, const String &text, uint8_t maxChars, bool inverted) {
  uint8_t cursorX = x;
  for (unsigned int i = 0; i < text.length() && i < maxChars; ++i) {
    if (cursorX + 5 >= OLED_WIDTH) {
      break;
    }
    uint8_t ch = (uint8_t)text.charAt(i);
    oledDrawChar(cursorX, y, (ch >= 32 && ch <= 126) ? (char)ch : '?', inverted);
    cursorX += 6;
  }
}

void oledDrawTextCentered(const String &text, uint8_t y, uint8_t maxChars) {
  uint8_t count = text.length() > maxChars ? maxChars : text.length();
  uint8_t width = count * 6;
  oledDrawText((OLED_WIDTH - width) / 2, y, text, maxChars, false);
}

const OledCjkGlyph *oledFindCjkGlyph(uint16_t codepoint) {
  for (uint8_t i = 0; i < OLED_CJK16_GLYPH_COUNT; ++i) {
    if (OLED_CJK16_GLYPHS[i].codepoint == codepoint) {
      return &OLED_CJK16_GLYPHS[i];
    }
  }
  return nullptr;
}

bool oledNextUtf8Codepoint(const char *text, uint16_t &offset, uint16_t &codepoint) {
  uint8_t first = (uint8_t)text[offset];
  if (first == 0) {
    return false;
  }
  offset++;
  if (first < 0x80) {
    codepoint = first;
    return true;
  }
  uint8_t second = (uint8_t)text[offset];
  uint8_t third = (uint8_t)text[offset + 1];
  if ((first & 0xF0) == 0xE0 && (second & 0xC0) == 0x80 && (third & 0xC0) == 0x80) {
    offset += 2;
    codepoint = ((uint16_t)(first & 0x0F) << 12) | ((uint16_t)(second & 0x3F) << 6) | (third & 0x3F);
    return true;
  }
  codepoint = '?';
  return true;
}

void oledDrawCjkGlyph(uint8_t x, uint8_t y, uint16_t codepoint) {
  const OledCjkGlyph *glyph = oledFindCjkGlyph(codepoint);
  if (glyph == nullptr) {
    oledFillRect(x + 2, y + 2, 12, 12, true);
    oledFillRect(x + 4, y + 4, 8, 8, false);
    return;
  }
  for (uint8_t row = 0; row < 16; ++row) {
    uint16_t bits = ((uint16_t)glyph->bitmap[row * 2] << 8) | glyph->bitmap[row * 2 + 1];
    for (uint8_t col = 0; col < 16; ++col) {
      if ((bits & ((uint16_t)1 << (15 - col))) != 0) {
        oledSetPixel(x + col, y + row, true);
      }
    }
  }
}

void oledDrawCjkTextCentered(const char *text, uint8_t y) {
  uint16_t offset = 0;
  uint16_t codepoint = 0;
  uint8_t count = 0;
  while (oledNextUtf8Codepoint(text, offset, codepoint)) {
    count++;
  }
  uint8_t width = count > 8 ? OLED_WIDTH : count * 16;
  uint8_t x = (OLED_WIDTH - width) / 2;
  offset = 0;
  while (oledNextUtf8Codepoint(text, offset, codepoint) && x + 15 < OLED_WIDTH) {
    oledDrawCjkGlyph(x, y, codepoint);
    x += 16;
  }
}

void oledDrawLine(int x0, int y0, int x1, int y1, bool on) {
  int dx = abs(x1 - x0);
  int sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0);
  int sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;

  while (true) {
    if (x0 >= 0 && x0 < OLED_WIDTH && y0 >= 0 && y0 < OLED_HEIGHT) {
      oledSetPixel((uint8_t)x0, (uint8_t)y0, on);
    }
    if (x0 == x1 && y0 == y1) {
      break;
    }
    int e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

bool oledProbe(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

bool oledWriteCommand(uint8_t command) {
  Wire.beginTransmission(oledAddress);
  Wire.write(0x00);
  Wire.write(command);
  return Wire.endTransmission() == 0;
}

bool oledWriteDataChunk(uint16_t offset, uint8_t length) {
  Wire.beginTransmission(oledAddress);
  Wire.write(0x40);
  for (uint8_t i = 0; i < length; ++i) {
    Wire.write(oledBuffer[offset + i]);
  }
  return Wire.endTransmission() == 0;
}

bool initializeOled() {
  usbConsole.println("[OLED] probe 0x3D then 0x3C");
  if (oledProbe(OLED_ADDR_PRIMARY)) {
    oledAddress = OLED_ADDR_PRIMARY;
  } else if (oledProbe(OLED_ADDR_FALLBACK)) {
    oledAddress = OLED_ADDR_FALLBACK;
  } else {
    usbConsole.println("[OLED] SSD1306 not detected");
    return false;
  }

  delay(20);
  static const uint8_t initCommands[] = {
    0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
    0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
    0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
  };
  for (uint8_t i = 0; i < sizeof(initCommands); ++i) {
    if (!oledWriteCommand(initCommands[i])) {
      usbConsole.println("[OLED] SSD1306 init failed");
      return false;
    }
  }

  oledAvailable = true;
  clearOledBuffer();
  oledRenderPending = true;
  usbConsole.print("[OLED] SSD1306 addr=0x");
  usbConsole.println(oledAddress, HEX);
  return true;
}

void flushOledPage() {
  if (!oledAvailable || !oledDirty) {
    return;
  }
  uint32_t now = millis();
  if (now - lastOledFlushMs < OLED_FLUSH_INTERVAL_MS) {
    return;
  }
  lastOledFlushMs = now;

  if (!oledWriteCommand(0xB0 | oledFlushPage) ||
      !oledWriteCommand(0x00) ||
      !oledWriteCommand(0x10)) {
    oledAvailable = false;
    usbConsole.println("[OLED] flush command failed");
    return;
  }

  uint16_t pageOffset = (uint16_t)oledFlushPage * OLED_WIDTH;
  for (uint8_t col = 0; col < OLED_WIDTH; col += OLED_CHUNK_BYTES) {
    if (!oledWriteDataChunk(pageOffset + col, OLED_CHUNK_BYTES)) {
      oledAvailable = false;
      usbConsole.println("[OLED] flush data failed");
      return;
    }
  }

  oledFlushPage++;
  if (oledFlushPage >= OLED_PAGES) {
    oledFlushPage = 0;
    oledDirty = false;
  }
}
