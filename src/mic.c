#include "ch.h"
#include "hal.h"

#include "orchard.h"

#include "mic.h"

#include <string.h>
#include <limits.h>

#include "shell.h"
#include "chprintf.h"
#include "orchard-test.h"
#include "test-audit.h"
//uint32_t mic_return[MIC_SAMPLE_DEPTH];

void i2s_handler(I2SDriver *i2sp, size_t offset, size_t n);
int32_t rx_samples[NUM_RX_SAMPLES];
uint16_t rx_savebuf[NUM_RX_SAMPLES];
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
  int i;
  
  // for now just copy it into the save buffer over and over again.
  // in the future, this would then kick off a SPI MMC data write event to save out the blocks
  rx_handler_count++;
  //  memcpy( rx_savebuf, rx_samples, NUM_RX_SAMPLES * sizeof(uint32_t) );
  for( i = 0; i < NUM_RX_SAMPLES; i++ ) { // just grab the first 128 bytes and sample-size convert
    //    rx_savebuf[i] = (uint16_t) ((rx_samples[i] + INT_MAX + 1) >> 16) & 0xFFFF;
    rx_savebuf[i] = (uint16_t) (((rx_samples[i] >> 16) + 32768) & 0xFFFF);
  }
  
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

uint16_t *analogReadMic(void) {
  return rx_savebuf;
}

OrchardTestResult test_mic(const char *my_name, OrchardTestType test_type) {
  (void) my_name;
  uint16_t min, max;
  int i, j;
  char prompt[16];
  
  switch(test_type) {
  case orchardTestPoweron:
  case orchardTestTrivial:
    return orchardResultNoTest;
  case orchardTestInteractive:
  case orchardTestComprehensive:
    orchardTestPrompt("speak into", "microphone", 0);
    min = 65535; max = 0;

    for( j = 0; j < 20; j++ ) {
      gen_mic_event = 1;
      
      chThdYield();
      chThdSleepMilliseconds(200);  // wait for mic to sample

      for( i = 0; i < NUM_RX_SAMPLES; i++ ) { // input sample buffer is deeper, search all the way through
	if( rx_savebuf[i] > max )
	  max = rx_savebuf[i];
	if( rx_savebuf[i] < min )
	  min = rx_savebuf[i];
      }
    }
    
    uint16_t span = max - min;
    chsnprintf(prompt, sizeof(prompt), "span %d", span);

    if( span > 100 ) {
      orchardTestPrompt("mic test PASS", prompt, 0);
      chprintf(stream, "mic test pass, span: %d\n\r", span);
      return orchardResultPass;
    } else {
      orchardTestPrompt("mic test FAIL", prompt, 0);
      chprintf(stream, "mic test fail, span: %d\n\r", span);
      return orchardResultFail;
    }
    
  default:
    return orchardResultNoTest;
  }
  
  return orchardResultNoTest;
}
orchard_test("mic", test_mic);

