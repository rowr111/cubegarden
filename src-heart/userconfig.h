#include "mic.h"

#define CONFIG_SIGNATURE  0x55434647  // UCFG
#define CONFIG_BLOCK      1
#define CONFIG_OFFSET     0
#define CONFIG_VERSION    16

typedef struct userconfig {
  uint32_t  signature;
  uint32_t  version;
  uint32_t  sex_initiations; // number of times we've initiated sex
  uint32_t  sex_responses;   // numbef or times others have initiated sex
  uint32_t  cfg_autosex;     // set if sex automatically allowed
  uint32_t  cfg_channel;     // our default channel
  uint32_t  cfg_txboost;     // set txboost
  uint32_t  cfg_autoadv;     // set fx auto advance
  uint32_t  cfg_bright_thresh; //battery charge threshold for dimming
  uint32_t  cfg_bright_thresh2; //battery charge threshold for dimming, second threshold
  uint32_t  cfg_bright_thresh3; //battery charge threshold for dimming, third threshold
  uint32_t  cfg_timesync_interval;  // interval for timesync, in seconds
  uint32_t  cfg_fx_newcube_time; // duration of a "new cube" notification in interactive effects
  uint8_t   cfg_addressCounter; // radio address
  uint32_t  cfg_clip_used[MAX_CLIPS];
} userconfig;

void configStart(void);

const userconfig *getConfig(void);
void configIncSexInitiations(void);
void configIncSexResponses(void);
void configSetAutosex(void);
void configClearAutoSex(void);
void configToggleAutosex(void);
void configSetChannel(uint32_t channel);
void configSetAddressCounter(uint8_t addressCounter);
void configSetBrightThresh(uint32_t bright_thresh);
void configSetBrightThresh2(uint32_t bright_thresh2);
void configSetBrightThresh3(uint32_t bright_thresh3);
void configSetFxNewcubeTime(uint32_t fx_newcube_time);
void configSetTimeSyncInterval(uint32_t timesync_interval);
void configSetAutoAdv(uint32_t autoadv);
void configToggleBoost(void);
void configClipMarkUsed(uint32_t clip);
void configClipClearMarks(void);
void configFlush(void); // call on power-down to flush config state

void configLazyFlush(void);  // call periodically to sync state, but only when dirty
