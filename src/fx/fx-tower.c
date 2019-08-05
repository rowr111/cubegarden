//
#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <barometer.h>

#ifndef MASTER_BADGE

#include "gyro.h"
#include "gfx.h"
#include "trigger.h"

#define FUNDEFINE -0.0

float baseline = FUNDEFINE;
static int whenToStart = 0;
static int brightness = 5;

static void tower(struct effects_config *config) {
    uint8_t *fb = config->hwconfig->fb;
    int loop = config->loop;
    int count = config->count;

    if (!baro_avg_valid) {
        chprintf(stream, "Waiting for barometer to be available\n\r");
        return;
    } else if (loop < whenToStart) {
        chprintf(stream, "Getting baseline...\n\r");
    } else if (loop > whenToStart) {
        
        if (baseline == FUNDEFINE) {
            baseline = baro_pressure;
        }

        if (loop % 30 == 0) {
            chprintf(stream, "baseline: %f\n\r", baseline);
            chprintf(stream, "baro_avg is: %f\n\r", baro_avg);
            chprintf(stream, "baro_pressure is: %f\n\r", baro_pressure);
            chprintf(stream, "%f\n\r", baro_pressure - baseline);
        }

        if (loop % 10 == 0) {

            int delta = (int)abs(baro_avg - baseline);

            RgbColor color = {delta, delta, delta};

            ledSetAllRGB(fb, count, color.r, 0, 0, shift);

            chprintf(stream, "%f\n\r", baro_pressure - baseline);
        }
    }
}

orchard_effects("aa-tower", tower, 0);

#endif

