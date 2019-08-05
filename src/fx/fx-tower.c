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

static void tower(struct effects_config *config) {
    uint8_t *fb = config->hwconfig->fb;
    int loop = config->loop;

    if (!baro_avg_valid) {
        chprintf(stream, "Waiting for barometer to be available");
        return;
    }

    if (baro_avg_valid && loop % 10 == 0) {
        chprintf(stream, "Value %f\n", baro_avg);
    }
}

orchard_effects("aa-tower", tower, 0);

