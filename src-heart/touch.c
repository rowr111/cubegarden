#include "hal.h"

#include "chprintf.h"
#include "shell.h"
#include "orchard-events.h"
#include "touch.h"
#include "orchard-ui.h"
#include "orchard-app.h"

#include "i2c.h"
#include "gfx.h"
#include "oled.h"

#include "orchard-test.h"
#include "test-audit.h"

event_source_t touch_event;

unsigned int touch_debounce = TOUCH_DEBOUNCE;

uint8_t touch_state = 0;
static uint32_t stuck_start = 0;
static uint32_t stuck_time = 0;

uint8_t captouchRead(void) {
  uint8_t tx[2], rx[1];

  tx[0] = (uint8_t) 0;
  tx[1] = (uint8_t) 0;
  
  i2cAcquireBus(&I2CD1);
  i2cMasterTransmitTimeout(&I2CD1, CAP1208_ADDR, tx, 2, rx, 0, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);
  
  tx[0] = 3;
  i2cAcquireBus(&I2CD1);
  i2cMasterTransmitTimeout(&I2CD1, CAP1208_ADDR, tx, 1, rx, 1, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);

  return rx[0];
}


// throw-away function for checking touch sensor status
void update_touch_ui(uint8_t state) {
  coord_t width, fontheight;
  font_t font;
  char str[32];
  int i;
  
  oledGfxStart();
  width = gdispGetWidth();
  font = gdispOpenFont("fixed_5x8");
  fontheight = gdispGetFontMetric(font, fontHeight);
  
  //  chsnprintf(str, sizeof(str), "%s", "pass  ");
  for( i = 0; i < 8; i++ ) {
    str[i] = state & 0x80 ? '#' : '.';
    state <<= 1;
  }
  str[i] = '\0';
  
  gdispClear(Black);
  gdispDrawStringBox(0, 0, width, fontheight,
                     str, font, White, justifyCenter);
  gdispDrawStringBox(0, fontheight, width, fontheight * 2,
                     "76543210", font, White, justifyCenter);
  gdispFlush();
  gdispCloseFont(font);
  oledGfxEnd();
}

void touch_force_cal(void) {
  uint8_t tx[2], rx[1];
  
  tx[0] = 0x26;
  tx[1] = 0xff;
  i2cAcquireBus(&I2CD1);
  i2cMasterTransmitTimeout(&I2CD1, CAP1208_ADDR, tx, 2, rx, 0, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);
}


void touchHandler(eventid_t id) {
  (void) id;
  uint8_t tx[2], rx[1];
  uint8_t touch;
  
  // read sensor input stataus
  tx[0] = 0x3;
  i2cAcquireBus(&I2CD1);
  i2cMasterTransmitTimeout(&I2CD1, CAP1208_ADDR, tx, 1, rx, 1, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);
  touch = rx[0];

#if 0  // used for debug only
  if( touch != touch_state )
    update_touch_ui(touch);
#endif

  if( (touch_state == 0) && (touch != 0) ) {
    stuck_start = chVTGetSystemTime();
    stuck_time = chVTGetSystemTime();
  }
  if( (touch & touch_state) ) {
    stuck_time = chVTGetSystemTime();
  }

  if( (stuck_time - stuck_start) > STUCK_TIMEOUT ) {
    touch_force_cal();
    stuck_start = chVTGetSystemTime();
    stuck_time = chVTGetSystemTime();
  }

  // clear any pending interrupts, so we can fire again when touch is detected
  tx[0] = 0;
  tx[1] = 0;
  i2cAcquireBus(&I2CD1);
  i2cMasterTransmitTimeout(&I2CD1, CAP1208_ADDR, tx, 2, rx, 0, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);

#if 0
  // update the UI touch records
  if( touch & (1 << 1) ) {
    if( chVTTimeElapsedSinceX(uiinput.left_last) > touch_debounce ) {
      touch_debounce = TOUCH_DEBOUNCE;
      uiinput.left = 1;
    }
    uiinput.left_last = chVTGetSystemTime();
  }
  if( touch & (1 << 4) ) {
    if( chVTTimeElapsedSinceX(uiinput.right_last) > touch_debounce ) {
      touch_debounce = TOUCH_DEBOUNCE;
      uiinput.right = 1;
    }
    uiinput.right_last = chVTGetSystemTime();
  }
  if( touch & (1 << 2) ) {
    if( chVTTimeElapsedSinceX(uiinput.up_last) > touch_debounce ) {
      touch_debounce = TOUCH_DEBOUNCE;
      uiinput.up = 1;
    }
    uiinput.up_last = chVTGetSystemTime();
  }
  if( touch & (1 << 3) ) {
    if( chVTTimeElapsedSinceX(uiinput.down_last) > touch_debounce ) {
      touch_debounce = TOUCH_DEBOUNCE;
      uiinput.down = 1;
    }
    uiinput.down_last = chVTGetSystemTime();
  }
  if( touch & (1 << 7) ) {
    if( chVTTimeElapsedSinceX(uiinput.a_last) > touch_debounce ) {
      touch_debounce = TOUCH_DEBOUNCE;
      uiinput.a = 1;
    }
    uiinput.a_last = chVTGetSystemTime();
  }
  if( touch & (1 << 6) ) {
    if( chVTTimeElapsedSinceX(uiinput.b_last) > touch_debounce ) {
      touch_debounce = TOUCH_DEBOUNCE;
      uiinput.b = 1;
    }
    uiinput.b_last = chVTGetSystemTime();
  }
#endif
  
  touch_state = touch;

  keyHandler(id); // call to the app handler
}

void touchCb(EXTDriver *extp, expchannel_t channel) {
  (void)extp;
  (void)channel;

  chSysLockFromISR();
  chEvtBroadcastI(&touch_event);
  chSysUnlockFromISR();
}

void touchStart(void) {
  uint8_t tx[2], rx[1];
  int i;

  chEvtObjectInit(&touch_event);
  
  // don't block multiple touches
  tx[0] = 0x2A;
  tx[1] = 0; 
  i2cAcquireBus(&I2CD1);
  i2cMasterTransmitTimeout(&I2CD1, CAP1208_ADDR, tx, 2, rx, 0, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);

  // turn off repeated interrupts on press/hold
  tx[0] = 0x28;
  tx[1] = 0; 
  i2cAcquireBus(&I2CD1);
  i2cMasterTransmitTimeout(&I2CD1, CAP1208_ADDR, tx, 2, rx, 0, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);

  // ignore noise detection -- it's causing some buttons to be erroneously masked
  //  tx[0] = 0x44;
  //  tx[1] = 0x44;  // RF noise
  //  i2cAcquireBus(&I2CD1);
  //  i2cMasterTransmitTimeout(&I2CD1, CAP1208_ADDR, tx, 2, rx, 0, TIME_INFINITE);
  //  i2cReleaseBus(&I2CD1);
  tx[0] = 0x20; // ana noise
  tx[1] = 0x10;  // 0x10 = disable analog noise, 0x20 = disable digital noise
  i2cAcquireBus(&I2CD1);
  i2cMasterTransmitTimeout(&I2CD1, CAP1208_ADDR, tx, 2, rx, 0, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);
  
  // adjust thresholds
  for( i = 0; i < 8; i++ ) {
    tx[0] = 0x30 + i;
    tx[1] = 0x40; // default is 0x40
    i2cAcquireBus(&I2CD1);
    i2cMasterTransmitTimeout(&I2CD1, CAP1208_ADDR, tx, 2, rx, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
  }

  // enable interrupts
  tx[0] = 0x27;
  tx[1] = 0xFF;
  i2cAcquireBus(&I2CD1);
  i2cMasterTransmitTimeout(&I2CD1, CAP1208_ADDR, tx, 2, rx, 0, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);

  // clear any pending interrupts
  tx[0] = 0;
  tx[1] = 0;
  i2cAcquireBus(&I2CD1);
  i2cMasterTransmitTimeout(&I2CD1, CAP1208_ADDR, tx, 2, rx, 0, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);

  touch_force_cal();
}

OrchardTestResult test_captouch(const char *my_name, OrchardTestType test_type) {
  (void) my_name;
  char touchstat[9];
  uint16_t touchmask = 0;
  uint8_t i;
  uint8_t tx[1], rx[1];
  
  switch(test_type) {
  case orchardTestPoweron:
  case orchardTestTrivial:
    tx[0] = 0xFD;
    i2cAcquireBus(&I2CD1);
    i2cMasterTransmitTimeout(&I2CD1, CAP1208_ADDR, tx, 1, rx, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( rx[0] != 0x6B ) { // this is the product ID
      return orchardResultFail;
    } else {
      return orchardResultPass;
    }
    break;
  case orchardTestInteractive:
    if( orchardTestPrompt("Do not touch", "touch sensors", 2) == orchardResultPass ) {
      orchardTestPrompt("captouch fail", "sensor stuck on", 0);
      return orchardResultFail;
    }
    touchstat[8] = '\0';
    while( touchmask != 0xFF ) {
      touchmask |= captouchRead();
      for( i = 0; i < 8; i++ ) {
	if( (touchmask >> i) & 1 )
	  touchstat[i] = 'T';
	else
	  touchstat[i] = '_';
      }
      orchardTestPrompt("touch each sensor", touchstat, 0);
      chThdYield();
      chThdSleepMilliseconds(10);
      // if not all sensors can be touched, the test will never time out and we'll get stuck.
    }
    orchardTestPrompt("touch test PASS", touchstat, 0);
    return orchardResultPass;
    
  default:
    return orchardResultNoTest;
  }
  
  return orchardResultNoTest;
}
orchard_test("ctouch", test_captouch);

