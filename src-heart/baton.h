#ifndef __ORCHARD_BATON__
#define __ORCHARD_BATON__

#include "ch.h"

#define MAX_ACTUAL_CUBES 2  // address space is up to 254, but for baton passing we want to not try
// passing to cubes that don't exist. So MAX_ACTUAL_CUBES limits the range of numbers to try for baton
// passing. This is now just a "first draft" for size, can be updated by a baton_maxcube packet
#define BATON_PASS_WAIT 200  // how long to wait in ms during a baton pass to sample responses
#define BATON_HOLDER_INTERVAL 2000  // time between baton holder broadcast pings
#define BATON_RADIO_ACK_DUP 3   // define how many times packets are resent by default
#define BATON_RADIO_ACK_DUP_DELAY 27 // time in milliseconds to wait between resending dups

extern uint8_t maxActualCubes;

typedef enum {
  baton_holder      = 1,   // this packet identifies the current baton holder
  baton_pass        = 2,   // this packet is an attempt to pass the baton to id at "address"
  baton_ack         = 3,   // this packet acknowledges the baton was received
  baton_maxcube     = 4,   // transmitted by the master badge to reset the maxActualCubes number.
} baton_packet_type;

typedef struct _BatonPacket {
  baton_packet_type type;
  uint8_t  address;
  uint8_t  fx;
} BatonPacket;

typedef enum {
  baton_passing = -1,
  baton_not_holding = 0,
  baton_holding = 1,
} baton_return_type;

typedef enum {
  baton_random = 0,
  baton_increment = 1,
  baton_specified = 2,
} baton_strategy_type;

typedef struct _BatonState {
  baton_return_type  state;
  uint8_t  passing_to_addr;
  uint32_t retry_interval;
  baton_strategy_type strategy;
  uint32_t retry_time;
  uint32_t announce_time;
  uint8_t fx;
} BatonState;

/*
  Baton passing protocol. 

  All cubes initialize to not holding a baton. 

  The current baton holder broadcasts packet with baton_packet_type set to baton_holder, and
  the address set to the holder's ID once every BATON_HOLDER_INTERVAL milliseconds. Any cube
  that receives this packet and the address is not equal to their own, will immediately call
  initBaton() and reset their baton state to not holding a baton. Furthermore, any cube that
  receives a baton_pass packet and the address is not their own, will also reset their baton
  state to baton_not_holding. This is a "catch-all" safety   in case two batons are 
  accidentally spawned.

  The master badge creates a single baton by calling passBaton().

  The baton holder can verify if they have the baton by calling hasBaton(). This should be
  called frequently and used to modify the effect behavior on an iteration-by-iteration basis.

  When the holder decides it is time to pass the baton, it should call passBaton(). The holder
  can specify the baton pass strategy and an optional retry interval. The passBaton() call will
  return quickly, but not instantly -- at an inteval specified by BATON_PASS_WAIT. The interval
  is specified to be long enough to give a reasonable chance of the baton receiver responding,
  and may be subject to tuning. It is not the end of the world if we tune this parameter to zero,
  it just means the call always returns baton_passing.

  Please read the respective API call docs for more details. There are some important subtleties
  around how passBaton() vs retryBatonPass() behave with respect to the increment and random
  strategies.
 */


/*
  Returns the current state of whether we have the baton or not. Returns almost instantly,
  can call within the main loop of led.c.
 */
baton_return_type hasBaton(void);

/*
  Pass the baton. The baton passing strategy is specified in strategy. 
    - baton_random: Pass to a cube between (1, address). Use MAX_ACTUAL_CUBES typically.
    - baton_increment: Each successive call scans through cube space by incrementing addresses.
      Wraps at MAX_ACTUAL_CUBES, if no recipient is found, returns baton_holding.
    - baton_specified: Attempts to pass baton to the cube specified at address.

   This call will attempt to pass the baton, and will cause the caller to idle for 
   BATON_PASS_WAIT milliseconds. Typically this call happens in the effect loop, so you will
   see a pause in the effect equivalent to BATON_PASS_WAIT duration.

   At the conclusion of BATON_PASS_WAIT, it will return:
     - baton_passing if an ACK is not received
     - baton_not_holding if an ACK is received
     - baton_holding if the strategy is baton_increment and the entire address space has been searched

   If retry is not 0, the function will schedule a recurring baton pass retry at an interval of
   "retry" milliseconds. If retry is 0, it will try exactly once. 

  If not currently holding the baton, does nothing and returns baton_not_holding.
 */
baton_return_type passBaton(baton_strategy_type strategy, uint8_t address, uint32_t retry);

/*
  Retry the current baton pass as specified using passBaton(). This basically re-broadcasts the
  last baton pass packet, and waits again for BATON_PASS_WAIT for an ACK. 

  This call will not change the address of the baton pass: so exact same address as specified in
  the most recent passBaton() call. 

  If not currently holding the baton, does nothing and returns baton_not_holding.

  Unlike hasBaton(), this call has a real cost, so you don't want to call it over and over again
  (e.g. it'll be bad if you call it over and over again inside an led effect loop). The upper
  level management code should pick a retry interval.
 */
baton_return_type retryBatonPass(void);

/*
   Abort the current baton passing operation. This is a little bit of a dangerous call, but
   this allows the upper-level management code to decide baton passing isn't going to work at
   some point and conclude it is once again the baton holder. 

   This side-effects the baton holding state to baton_holding, resets the baton_increment counter,
   and clears the retry baton pass scheduler.

   It's the reset of the baton state to baton_holding that makes this dangerous: if you call this
   willy-nilly, you can spawn extra batons. Use only when you're really convinced the baton pass
   didn't work.
 */
void abortBatonPass(void);


/*
  Initialize the baton state to baton_not_holding, sets baton_increment counter to our current
  address, and clears the retry timer.
 */
void initBaton(void);

/*
  Low level radio baton holder. 
 */
void handleRadioBaton(uint8_t prot, uint8_t src, uint8_t dst, uint8_t length, const void *data);

/*
   Create the baton handler thread. Call exactly once on boot.
 */
void startBaton(void);

uint8_t getBatonFx(void);
void setBatonFx(uint8_t fx);

// for debugging
BatonState *getBatonState(void);

// for master badge only
void setMaxCubes(uint8_t maxcubes);

#endif
