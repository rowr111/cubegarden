#include "ch.h"
#include "hal.h"
#include "orchard.h"

#include "radio.h"
#include "storage.h"
#include "userconfig.h"

#include "mic.h"

#include <string.h>

static userconfig config_cache;

// reading is easy, just return an immutable structure
const userconfig *getConfig(void) {
  return (const userconfig *) &config_cache;
}

// writing is harder, wrap in helper functions
// why? because we want the flexibility to not use
// a RAM-only cache if the config structure starts to
// grow pretty big. For now we can do ramcache because
// it's small, but later on we may have to refactor this
// implementation, so make the API future-proof.
void configIncSexInitiations(void) {
  config_cache.sex_initiations++;
}

void configIncSexResponses(void) {
  config_cache.sex_responses++;
}

void configSetAutosex(void) {
  config_cache.cfg_autosex = 1;
}

void configSetClearAutosex(void) {
  config_cache.cfg_autosex = 0;
}

void configToggleAutosex(void) {
  config_cache.cfg_autosex = !config_cache.cfg_autosex;
}

void configToggleBoost(void) {
  config_cache.cfg_txboost = !config_cache.cfg_txboost;
}

void configSetdBbkgd(uint8_t dBbkgd){
  config_cache.cfg_dBbkgd = dBbkgd;
}

void configSetdBmax(uint8_t dBmax){
  config_cache.cfg_dBmax = dBmax;
}

void configSetBrightThresh(uint32_t bright_thresh){
  config_cache.cfg_bright_thresh = bright_thresh;
}

void configSetBrightThresh2(uint32_t bright_thresh2){
  config_cache.cfg_bright_thresh2 = bright_thresh2;
}

void configSetBrightThresh3(uint32_t bright_thresh3){
  config_cache.cfg_bright_thresh3 = bright_thresh3;
}

void configSetAutoAdv(uint32_t autoadv) {
  config_cache.cfg_autoadv = autoadv;
}

void configSetTimeSyncInterval(uint32_t timesync_interval) {
  config_cache.cfg_timesync_interval = timesync_interval;
}

void configSetFxNewcubeTime(uint32_t fx_newcube_time) {
  config_cache.cfg_fx_newcube_time = fx_newcube_time;
}

void configSetChannel(uint32_t channel) {
  if( channel >= RADIO_MAXCHANNELS )
    channel = 0;  // force to default if bogus stuff is sent to us
  
  config_cache.cfg_channel = channel;
}

void configSetAddressCounter(uint8_t addressCounter) {
  config_cache.cfg_addressCounter = addressCounter;
}

void configClipMarkUsed(uint32_t clip) {
  if( clip >= MAX_CLIPS )
    return;

  config_cache.cfg_clip_used[clip] = 1;
}

void configClipClearMarks(void) {
  int i;

  for( i = 0; i < MAX_CLIPS; i++ ) {
    config_cache.cfg_clip_used[i] = 0;
  }
}

void configFlush(void) {
  storagePatchData(CONFIG_BLOCK, (uint32_t *) &config_cache, CONFIG_OFFSET, sizeof(struct userconfig));
}

void configLazyFlush(void) {
  const struct userconfig *config;

  config = (const struct userconfig *) storageGetData(CONFIG_BLOCK);
  if( memcmp( config, &config_cache, sizeof(struct userconfig) ) != 0) {
    storagePatchData(CONFIG_BLOCK, (uint32_t *) &config_cache, CONFIG_OFFSET, sizeof(struct userconfig));
  }
}

static void init_config(uint32_t block) {
  struct userconfig config;
  int i;

  config.signature = CONFIG_SIGNATURE;
  config.version = CONFIG_VERSION;

  config.sex_initiations = 0;
  config.sex_responses = 0;
  config.cfg_autosex = 1;   // cubegarden is always DTF!
  config.cfg_channel = 0;
  config.cfg_addressCounter = 1;
  config.cfg_txboost = 0;   // range seems good enough without the boost
  config.cfg_autoadv = 0;
  config.cfg_dBbkgd = 50;        // bkg dB threshhold
  config.cfg_dBmax = 90;         // max dB threshhold
  config.cfg_bright_thresh = 3750;
  config.cfg_bright_thresh2 = 3650;
  config.cfg_bright_thresh3 = 3550;
  config.cfg_fx_newcube_time = 4; // seconds by default for people to find the new cube
  config.cfg_timesync_interval = 30; // 30 seconds by default

  for( i = 0; i < MAX_CLIPS; i++ ) {
    config.cfg_clip_used[i] = 0;
  }

  storagePatchData(block, (uint32_t *) &config, CONFIG_OFFSET, sizeof(struct userconfig));
}

void configStart(void) {
  const struct userconfig *config;

  config = (const struct userconfig *) storageGetData(CONFIG_BLOCK);

  if( config->signature != CONFIG_SIGNATURE ) {
    init_config(CONFIG_BLOCK);
    config = (const struct userconfig *) storageGetData(CONFIG_BLOCK);
  } else if( config->version != CONFIG_VERSION ) {
    init_config(CONFIG_BLOCK);
    config = (const struct userconfig *) storageGetData(CONFIG_BLOCK);
  }

  memcpy( &config_cache, config, sizeof(userconfig) ); // copy configuration to volatile cache
}
