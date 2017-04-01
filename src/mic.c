#include "ch.h"
#include "hal.h"

#include "orchard.h"

#include "mic.h"

static adcsample_t mic_sample[MIC_SAMPLE_DEPTH];
static uint8_t mic_return[MIC_SAMPLE_DEPTH];

void analogUpdateTemperature(void) {
}

int32_t analogReadTemperature(void) {
  return 0;
}

void analogUpdateMic(void) {
  
}

uint8_t *analogReadMic(void) {
  return mic_return;
}
