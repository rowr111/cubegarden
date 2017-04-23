#include "ch.h"
#include "hal.h"

#include "orchard.h"

#include "mic.h"

#include <string.h>

uint32_t mic_return[MIC_SAMPLE_DEPTH];

void i2s_handler(I2SDriver *i2sp, size_t offset, size_t n);
int32_t rx_samples[NUM_RX_SAMPLES];
int32_t rx_savebuf[NUM_RX_SAMPLES];
uint32_t rx_handler_count = 0;

uint8_t gen_mic_event = 0;

static I2SConfig i2s_config = {
  NULL,
  rx_samples,
  NUM_RX_SAMPLES,
  i2s_handler,
  { // sai_tx_state
    {48000u, 12288000 /*mclk freq*/, 32, kSaiStereo},  // mclk must be at least 2x bitclock
    NULL,
    0,
    0,
    NULL,
    NULL,
    kSaiModeAsync,
    0,
    4,
    kSaiMaster,
    kSaiBusI2SType,
    //    NULL,  // semaphore_t
    FALSE,
    0,
  },
  { // sai_rx_state
    {48000u, 12288000 /*mclk freq*/, 32, kSaiStereo},
    (uint8_t *) rx_samples,  // regardless fo sample size, driver thinks of this as char stream...for now.
    NUM_RX_SAMPLES,
    0,
    NULL,
    NULL,
    kSaiModeAsync,
    0,
    4,
    kSaiMaster,
    kSaiBusI2SType,
    //    NULL,  // semaphore_t
    FALSE,
    0,
  },
  { // tx_userconfig
    kSaiMclkSourceSysclk,
    0,
    kSaiModeAsync,
    kSaiBusI2SType,
    kSaiMaster,
    kSaiBclkSourceMclkDiv,
    4,
    0,
  },
  { // rx_userconfig
    kSaiMclkSourceSysclk,
    0,
    kSaiModeAsync,
    kSaiBusI2SType,
    kSaiMaster,
    kSaiBclkSourceMclkDiv,
    4,
    0,
  }
};

extern event_source_t i2s_full_event;

void i2s_handler(I2SDriver *i2sp, size_t offset, size_t n) {
  (void) i2sp;
  (void) offset;
  (void) n;
  
  // for now just copy it into the save buffer over and over again.
  // in the future, this would then kick off a SPI MMC data write event to save out the blocks
  rx_handler_count++;
  memcpy( rx_savebuf, rx_samples, NUM_RX_SAMPLES * sizeof(uint32_t) );
  // kick out an event to write data to disk
  //  chSysLockFromISR();
  chEvtBroadcastI(&i2s_full_event);
  // chSysUnlockFromISR();

}

void micStart(void) {
  i2sStart(&I2SD1, (const I2SConfig *) &i2s_config);
}

void analogUpdateMic(void) {
  gen_mic_event = 1;
}

uint8_t *analogReadMic(void) {
  return mic_return;
}
