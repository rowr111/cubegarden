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

static void tower(struct effects_config *config) {
    int loop = config->loop;

    if (!baro_avg_valid) {
        chprintf(stream, "Waiting for barometer to be available\n\r");
        return;
    } else {
        
        if (baseline == FUNDEFINE) {
            baseline = baro_pressure;
        }

        if (loop % 30 == 0) {
            chprintf(stream, "%f\n\r", baro_pressure - baseline);
        }
    }
}

orchard_effects("aa-tower", tower, 0);

#endif

