#ifndef __LED_H__
#define __LED_H__

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "orchard.h"

/////////// STRUCTURE

struct orchard_effects_instance;

typedef struct led_config_def {
  uint8_t       *fb; // effects frame buffer
  uint8_t       *final_fb;  // merged ui + effects frame buffer
  uint32_t      pixel_count;  // generated pixel length
  uint32_t      max_pixels;   // maximal generation length
  uint8_t       *ui_fb; // frame buffer for UI effects
  uint32_t      ui_pixels;  // number of LEDs on the PCB itself for UI use
} led_config_def;
extern led_config_def  led_config;

typedef struct effects_config {
  led_config_def *hwconfig;
  uint32_t count;
  uint32_t loop;
} effects_config;

void orchardEffectsInit(void);
void orchardEffectsRestart(void);

typedef struct _OrchardEffects {
  char *name;
  void (*computeEffect)(struct effects_config *context);
} OrchardEffects;

#define orchard_effects_start() \
({ \
  static char start[0] __attribute__((unused,  \
    aligned(4), section(".chibi_list_effects_1")));        \
  (const OrchardEffects *)&start;            \
})

#define orchard_effects(_name, _func) \
  const OrchardEffects _orchard_fx_list_##_func \
  __attribute__((unused, aligned(4), section(".chibi_list_effects_2_" _name))) = \
     { _name, _func }

#define orchard_effects_end() \
  const OrchardEffects _orchard_fx_list_##_func \
  __attribute__((unused, aligned(4), section(".chibi_list_effects_3_end"))) = \
     { NULL, NULL }


/////////////////// EFFECTS

#define sign(x) (( x > 0 ) - ( x < 0 ))

typedef struct Color Color;
struct Color {
  uint8_t g;
  uint8_t r;
  uint8_t b;
};

typedef struct HsvColor {
  uint8_t h;
  uint8_t s;
  uint8_t v;
} HsvColor;

typedef struct RgbColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} RgbColor;

RgbColor HsvToRgb(HsvColor hsv);
HsvColor RgbToHsv(RgbColor rgb);
uint8_t gray_encode(uint8_t n);
uint8_t gray_decode(uint8_t n);

void ledStart(uint32_t leds, uint8_t *o_fb, uint32_t ui_leds, uint8_t *o_ui_fb);

void effectsStart(void);
uint8_t effectsStop(void);
extern uint8_t ledsOff;;

uint8_t effectsNameLookup(const char *name);
void effectsSetPattern(uint8_t, uint16_t);
void effectsSetTempPattern(uint8_t, uint16_t);
void effectsCheckExpiredTempPattern(void);
uint8_t effectsGetPattern(void);
void bump(uint32_t amount);
void pressureChanged(void);
void singletap(void);
void setShift(uint8_t s);
uint8_t getShift(void);
void effectsNextPattern(int skipstrobe);
void effectsPrevPattern(int skipstrobe);

void uiLedGet(uint8_t index, Color *c);
void uiLedSet(uint8_t index, Color c);

void listEffects(void);

const char *effectsCurName(void);
const char *lightgeneName(void);

void check_lightgene_hack(void);

#define EFFECTS_REDRAW_MS 35


/// these were perviously static, now global to facilitate splitting up the led effects into files
void ledSetRGB(void *ptr, int x, uint8_t r, uint8_t g, uint8_t b, uint8_t shift);
void ledSetRgbColor(void *ptr, int x, RgbColor c, uint8_t shift);
void ledSetAllRGB(void *ptr, int n, uint8_t r, uint8_t g, uint8_t b, uint8_t shift);
void ledSetAllRgbColor(void *ptr, int n, RgbColor c, uint8_t shift);
void ledSetColor(void *ptr, int x, Color c, uint8_t shift);
void ledSetRGBClipped(void *fb, uint32_t i,
                      uint8_t r, uint8_t g, uint8_t b, uint8_t shift);
Color ledGetColor(void *ptr, int x);

// Colors
extern const RgbColor vividViolet;
extern const RgbColor vividCerulean;
extern const RgbColor electricGreen;
extern const RgbColor vividYellow;
extern const RgbColor vividOrangePeel;
extern const RgbColor vividRed;
extern RgbColor vividRainbow[6];

// hardware configuration information
// max length is different from actual length because some
// pattens may want to support the option of user-added LED
// strips, whereas others will focus only on UI elements in the
// circle provided on the board itself
extern uint16_t fx_duration; //effect duration in ms. 0 == persistent
extern uint32_t fx_starttime; //start time for temporary effect

extern uint8_t shift;  // start a little bit dimmer

extern uint32_t bump_amount;
extern uint8_t bumped;
extern uint8_t pressure_changed;
extern uint8_t singletapped;
extern unsigned int pressurechangedtime;
extern unsigned int singletaptime;
extern unsigned int patternChanged;

void do_lightgene(effects_config *config);

#endif /* __LED_H__ */
