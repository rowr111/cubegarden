#include "ch.h"
#include "hal.h"
#include "shell.h"

#include "chprintf.h"

#include "led.h"
#include "orchard.h"
#include "radio.h"
#include "baton.h"
#include "userconfig.h"
#include "orchard-math.h"

static BatonState bstate;
uint8_t maxActualCubes = MAX_ACTUAL_CUBES;

BatonState *getBatonState(void) {
  return &bstate;
}

uint8_t getBatonFx(void) {
  return bstate.fx;
}

void setBatonFx(uint8_t fx) {
  bstate.fx = fx;
}

void initBaton(void) {
  const struct userconfig *config;
  config = getConfig();
  
  bstate.state = baton_not_holding;
  bstate.passing_to_addr = config->cfg_address;
  bstate.retry_interval = 0;
  bstate.strategy = baton_random;
  bstate.retry_time = chVTGetSystemTime();
  bstate.announce_time = chVTGetSystemTime();
  bstate.fx = 0;
}

void sendBatonAck(void) {
  const struct userconfig *config;
  config = getConfig();
  
  BatonPacket pkt;

  pkt.type = baton_ack;
  pkt.address = config->cfg_address;
  pkt.fx = bstate.fx;

  int i;
  // this handler runs in the event thread so it means we won't be able
  // to respond to events for the duration of the retry (e.g. chgcheck event, radio page, radio ping, gyro)
  for( i = 0; i < BATON_RADIO_ACK_DUP; i++ ) {
    radioAcquire(radioDriver);
    radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_baton, sizeof(pkt), &pkt);
    radioRelease(radioDriver);
    chThdSleepMilliseconds(BATON_RADIO_ACK_DUP_DELAY);
  }
}

baton_return_type hasBaton(void) {
  return bstate.state;
}

void handleRadioBaton(uint8_t prot, uint8_t src, uint8_t dst, uint8_t length, const void *data) {
  (void) prot;
  (void) src;
  (void) dst;
  (void) length;
  const struct userconfig *config;
  config = getConfig();

  BatonPacket *pkt = (BatonPacket *)data;

  switch(pkt->type) {
  case baton_holder: // the "true" baton holder is confirming its baton holding
    if( config->cfg_address != pkt->address ) {
      bstate.state = baton_not_holding;
      bstate.fx = pkt->fx; // fx updates based on the holder beacon
    }
    break;
  case baton_pass:
    if( config->cfg_address == pkt->address ) {
      bstate.state = baton_holding;
      bstate.announce_time = chVTGetSystemTime();
      bstate.fx = pkt->fx; // or if it's been passed to us specifically
      sendBatonAck();
    }
    break;
  case baton_ack:
    if( bstate.state == baton_passing )
      bstate.state = baton_not_holding;
    // right now, *any* ack will clear this -- we might get dups etc.
    // we could also check address of the ack sent to see if it's from the cube we were targeting
    // to catch the case where we have two batons...

    // don't update fx on the ack
    break;
  case baton_maxcube:
    maxActualCubes = pkt->address;
    // don't update fx on the maxcube, this is an admin message only
    break;
  default:
    chprintf(stream, "baton: bad packet type\n\r");
  }

}

void abortBatonPass(void) {
  const struct userconfig *config;
  config = getConfig();
  
  bstate.state = baton_holding;
  bstate.retry_interval = 0;
  
  bstate.passing_to_addr = config->cfg_address;
}

// this will send the baton pass exactly once -- rely on the native retry mechanism for now?
void sendBatonPassPacket(void) {
  BatonPacket pkt;

  pkt.type = baton_pass;
  pkt.address = bstate.passing_to_addr;
  pkt.fx = bstate.fx;

  radioAcquire(radioDriver);
  radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_baton, sizeof(pkt), &pkt);
  radioRelease(radioDriver);
}

baton_return_type passBaton(baton_strategy_type strategy, uint8_t address, uint32_t retry) {
  const struct userconfig *config;
  config = getConfig();

  // we can't pass the baton if we aren't holding it...
  if( bstate.state == baton_not_holding )
    return baton_not_holding;

  // if there aren't any cubes to pass the baton to, don't even try
  if (maxActualCubes < 2) {
    return baton_holding;
  }

  bstate.state = baton_passing;
  bstate.strategy = strategy;
  bstate.retry_interval = retry;
  bstate.retry_time = chVTGetSystemTime();
  if( bstate.strategy == baton_specified ) {
    bstate.passing_to_addr = address;
    if( bstate.passing_to_addr == config->cfg_address ) {
      chprintf(stream, "baton: passed to ourselves, check pass specification logic!\n\r" );
      bstate.state = baton_holding; // we passed the baton to ourselves, we're done!
      return baton_holding;
    }
  } else if( bstate.strategy == baton_increment ) {
    bstate.passing_to_addr = config->cfg_address + 1; // scan through the space of cubes
    if( bstate.passing_to_addr > maxActualCubes )
      bstate.passing_to_addr = 1;
  } else if( bstate.strategy == baton_random ) {
    do {
      bstate.passing_to_addr = (((uint32_t) rand()) % maxActualCubes) + 1;
    } while( bstate.passing_to_addr == config->cfg_address );
  }

  sendBatonPassPacket();
  // wait to see if we get an ack
  chThdSleepMilliseconds(BATON_PASS_WAIT);
  
  // if an ack arrived, the event thread would have updated our internal variables
  return bstate.state;
}

baton_return_type retryBatonPass(void) {
  sendBatonPassPacket();
  // wait to see if we get an ack
  chThdSleepMilliseconds(BATON_PASS_WAIT);

  // reset the retry timer, so that manual calls to this continuously push the timer out
  bstate.retry_time = chVTGetSystemTime();
  
  // if an ack arrived, the event thread would have updated our internal variables
  return bstate.state;
}

void sendBatonHoldingPacket(void) {
  if( bstate.state != baton_holding )
    return;  // don't allow someone to accidentally broadcast this

  const struct userconfig *config;
  config = getConfig();
  
  BatonPacket pkt;

  pkt.type = baton_holder;
  pkt.address = config->cfg_address;
  pkt.fx = bstate.fx;

  radioAcquire(radioDriver);
  radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_baton, sizeof(pkt), &pkt);
  radioRelease(radioDriver);
}

static thread_t *batonThr = NULL;
static THD_WORKING_AREA(waBatonThread, 0x200);
static THD_FUNCTION(baton_thread, arg) {
  (void)arg;

  uint8_t retry_attempts = 0;
  
  chRegSetThreadName("BatonManager");
  initBaton();
  
  while (!chThdShouldTerminateX()) {
    if( (bstate.state == baton_passing) && (bstate.retry_interval > 0) ) {
      // we've specified a retry interval, so let's see if it's time to retry
      if( chVTTimeElapsedSinceX(bstate.retry_time) > bstate.retry_interval ) {
	//	chprintf(stream, "baton pass retry\n\r" );
	sendBatonPassPacket();
	bstate.retry_time = chVTGetSystemTime();
	retry_attempts++;
      }
      
      // if not specified, consider trying a new address if we hit our retry attempt limit
      if( ((bstate.strategy == baton_random) || (bstate.strategy == baton_increment)) &&
	  retry_attempts > BATON_MAX_AUTORETRIES ) {
	// retry the current strategy
	passBaton(bstate.strategy, 0, bstate.retry_interval);
	retry_attempts = 0;
      }
    }

    // if I'm the baton holder, periodically announce my holding
    if( bstate.state == baton_holding ) {
      if( bstate.fx_uses_baton ) {
	if( chVTTimeElapsedSinceX(bstate.announce_time) > BATON_HOLDER_INTERVAL ) {
	  bstate.announce_time = chVTGetSystemTime();
	  sendBatonHoldingPacket();
	}
      } else {
	// the effect doesn't use a baton, pass it off, so we don't screw up effects that are using it...
	chThdSleepMilliseconds(500); // wait a half second, then pass it
	// the half-second cooldown is here so if a baton exists on a network that doesn't need it,
	// bandwidth isn't being consumed by batons just being passed around pointlessly
	passBaton(baton_random, 0, 100); // but pass it urgently once we do pass it
      }
      retry_attempts = 0;
    }
    
    chThdSleepMilliseconds(10); // give some time for other threads
    // this also rate-limits retry packets to once every 10ms
  }

  chSysLock();
  chThdExitS(MSG_OK);
}

void startBaton(void) {
  // start the baton management
  batonThr = chThdCreateStatic(waBatonThread,
			       sizeof(waBatonThread),
			       (NORMALPRIO + 1), // slightly higher than the main thread
			       baton_thread,
			       NULL);
}

void setMaxCubes(uint8_t maxcubes) {
  BatonPacket pkt;

  pkt.type = baton_maxcube;
  pkt.address = maxcubes;
  pkt.fx = bstate.fx;

  int i;
  for( i = 0; i < 5; i++ ) {
    radioAcquire(radioDriver);
    radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_baton, sizeof(pkt), &pkt);
    radioRelease(radioDriver);
    chThdSleepMilliseconds(33);
  }
}
