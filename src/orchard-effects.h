#ifndef __ORCHARD_EFFECTS_H__
#define __ORCHARD_EFFECTS_H__

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "orchard.h"

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

#endif /* __ORCHARD_EFFECTS_H__ */
