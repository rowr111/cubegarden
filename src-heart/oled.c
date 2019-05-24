#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "oled.h"
#include "shell.h"
#include "chprintf.h"

#include "gfx.h"

// 011_1100
#define SSD1306_ADDR  0x3C

#define NL SHELL_NEWLINE_STR

void oledStart(void) {
  osalMutexObjectInit(&orchard_gfxMutex);
}

// pass through to legacy functions
void oledGfxStart() {
  orchardGfxStart();
}

void oledGfxEnd() {
  orchardGfxEnd();
}

void oledBanner(void) {
  coord_t width, fontheight;
  font_t font;
  
  oledGfxStart();
  width = gdispGetWidth();
  font = gdispOpenFont("UI2");
  fontheight = gdispGetFontMetric(font, fontHeight);
  
  gdispClear(Black);
  gdispDrawStringBox(0, fontheight, width, fontheight * 2,
                     "BM 2017", font, White, justifyCenter);
  gdispDrawStringBox(0, fontheight * 2, width, fontheight * 3,
                     "EVT1", font, White, justifyCenter);
  gdispFlush();
  gdispCloseFont(font);
  oledGfxEnd();
}

void oledAcquireBus(void) {
  i2cAcquireBus(OLED_I2C);
}

void oledReleaseBus(void) {
  i2cReleaseBus(OLED_I2C);
}

void oledCmd(uint8_t cmd) {
  uint8_t tx[2];
  msg_t retval;

  tx[0] = 0;  // command, not continuation
  tx[1] = cmd;
  retval = i2cMasterTransmitTimeout(&I2CD2, SSD1306_ADDR, tx, 2, NULL, 0, TIME_INFINITE);

  if( retval != MSG_OK ) {
    chprintf((BaseSequentialStream *)&SD4, " OLED I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
  }
}

void oledData(uint8_t *data, uint16_t length) {
  uint8_t *message;
  msg_t retval;

  message = (uint8_t *) chHeapAlloc(NULL, length + 1);
  
  message[0] = 0x40;  // data, not continuation
  memcpy(&(message[1]), data, length);
  retval = i2cMasterTransmitTimeout(&I2CD2, SSD1306_ADDR, message, length + 1, NULL, 0, TIME_INFINITE);

  chHeapFree(message);
  
  if( retval != MSG_OK ) {
    chprintf((BaseSequentialStream *)&SD4, " OLED I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
  }
}

