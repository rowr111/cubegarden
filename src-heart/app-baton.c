#include "orchard-app.h"
#include "orchard-ui.h"
#include "userconfig.h"
#include "radio.h"
#include <string.h>
#include "baton.h"
#include "orchard-math.h"

uint8_t baton_holder_g = 0;
uint8_t baton_target_g = 0;
baton_packet_type last_baton_packet_g = baton_holder;
uint8_t baton_passing_g = 0;

uint32_t last_ping_g = 0;
/*
   Baton Management
  Holder: ### or 'none'
  Last ping: ##### ms
  Last pkt: 'holder' or 'pass' or 'ack' or 'maxcube'
  Maxcubes: ###
  A - clear baton
  B - new random baton
 */

static void redraw_ui(int banner) {
  char uiStr[32];
  
  coord_t width;
  coord_t height;
  font_t font;
  color_t draw_color = White;
  
  const struct userconfig *config;
  config = getConfig();

  orchardGfxStart();
  // draw the title bar
  font = gdispOpenFont("fixed_5x8");
  width = gdispGetWidth();
  height = gdispGetFontMetric(font, fontHeight);

  switch(banner) {
  case 1:
    chsnprintf(uiStr, sizeof(uiStr), "Clearing Batons");
    break;
  case 2:
    chsnprintf(uiStr, sizeof(uiStr), "Setting Baton");
    break;
  default:
    chsnprintf(uiStr, sizeof(uiStr), "Baton Management");
    break;
  }
  gdispClear(Black);
  gdispFillArea(0, 0, width, height, White);
  gdispDrawStringBox(0, 0, width, height,
                     uiStr, font, Black, justifyCenter);

  // 1st line: holder / passing
  if( !baton_passing_g ) {
    if( (baton_holder_g != 0) && (baton_holder_g != 254) )
      chsnprintf(uiStr, sizeof(uiStr), "Holder: %d", baton_holder_g);
    else
      chsnprintf(uiStr, sizeof(uiStr), "Holder: none");
    gdispDrawStringBox(0, height, width, height,
		       uiStr, font, White, justifyLeft);
  } else {
    if( baton_holder_g != 0 && (baton_holder_g != 254) )
      chsnprintf(uiStr, sizeof(uiStr), "Holder: %d->%d", baton_holder_g, baton_target_g);
    else
      chsnprintf(uiStr, sizeof(uiStr), "Holder: none->%d", baton_target_g);
    gdispDrawStringBox(0, height, width, height,
		       uiStr, font, White, justifyLeft);
  }


  // 2nd line: last ping
  chsnprintf(uiStr, sizeof(uiStr), "Last ping %dms ago", chVTTimeElapsedSinceX(last_ping_g) );
  gdispDrawStringBox(0, height*2, width, height,
		     uiStr, font, White, justifyLeft);

  // 3rd line: last packet
  switch( last_baton_packet_g ) {
  case baton_holder:
    chsnprintf(uiStr, sizeof(uiStr), "Last pkt: holder" );
    break;
  case baton_pass:
    chsnprintf(uiStr, sizeof(uiStr), "Last pkt: pass" );
    break;
  case baton_ack:
    chsnprintf(uiStr, sizeof(uiStr), "Last pkt: ack" );
    break;
  case baton_maxcube:
    chsnprintf(uiStr, sizeof(uiStr), "Last pkt: maxcube" );
    break;
  default:
    chsnprintf(uiStr, sizeof(uiStr), "Last pkt: UNKNOWN" );
    break;
  }
  gdispDrawStringBox(0, height*3, width, height,
		     uiStr, font, White, justifyLeft);
  

  // 4th line: maxcubes
  chsnprintf(uiStr, sizeof(uiStr), "Maxcubes: %d", config->cfg_addressCounter );
  gdispDrawStringBox(0, height*4, width, height,
		     uiStr, font, White, justifyLeft);

  // 5th line: A button help
  chsnprintf(uiStr, sizeof(uiStr), "A - clear baton" );
  gdispDrawStringBox(0, height*5, width, height,
		     uiStr, font, White, justifyLeft);

  // 6th line: B button help
  chsnprintf(uiStr, sizeof(uiStr), "B - new random baton" );
  gdispDrawStringBox(0, height*6, width, height,
		     uiStr, font, White, justifyLeft);

  gdispFlush();
  orchardGfxEnd();
}

static uint32_t baton_init(OrchardAppContext *context) {
  (void)context;
  return 0;
}

static void baton_start(OrchardAppContext *context) {
  (void)context;
  
  last_ping_g = chVTGetSystemTime();
  
  orchardAppTimer(context, 125 * 1000 * 1000, true); // update UI interval
  redraw_ui(0);
}

static void baton_event(OrchardAppContext *context, const OrchardAppEvent *event) {
  (void)context;
  BatonPacket pkt;
  int i;
  const struct userconfig *config;
  config = getConfig();

  if (event->type == keyEvent) {
    if( (event->key.flags == keyDown) && ((event->key.code == keyTopR)) ) { // "A" key
      redraw_ui(1);

      setMaxCubes(config->cfg_addressCounter);
      
      // clear the baton by claiming it to the master badge
      pkt.type = baton_holder;
      pkt.address = 254; // this is the master badge reserved address

      for( i = 0; i < 10; i++ ) {
	radioAcquire(radioDriver);
	radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_baton, sizeof(pkt), &pkt);
	radioRelease(radioDriver);
	chThdSleepMilliseconds(10); // really spam it
      }
      baton_holder_g = 254; // presume i've got it
      
    } else if( (event->key.flags == keyDown) && ((event->key.code == keyBottomR)) ) { // "B" key
      redraw_ui(2);

      setMaxCubes(config->cfg_addressCounter);
      
      // clear the baton by claiming it to the master badge
      pkt.type = baton_holder;
      pkt.address = 254; // this is the master badge reserved address

      for( i = 0; i < 10; i++ ) {
	radioAcquire(radioDriver);
	radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_baton, sizeof(pkt), &pkt);
	radioRelease(radioDriver);
	chThdSleepMilliseconds(10); // really spam it
      }
      baton_holder_g = 254; // presume i've got it

      chThdSleepMilliseconds(200); // give cubes a moment to have their other threads run
      
      pkt.type = baton_pass;
      pkt.address = (((uint32_t) rand()) % config->cfg_addressCounter) + 1;
      baton_target_g = pkt.address;
      baton_passing_g = 1;

      for( i = 0; i < 20; i++ ) {
	radioAcquire(radioDriver);
	radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_baton, sizeof(pkt), &pkt);
	radioRelease(radioDriver);
	chThdSleepMilliseconds(20); // really spam it

	if( baton_passing_g == 0 )
	  break; // stop spamming if we've got an ack
      }
    }
  } else if(event->type == timerEvent) {
    // timer event will trigger this
    redraw_ui(0);
  }
}

static void baton_exit(OrchardAppContext *context) {
  (void)context;
  
}

orchard_app("Baton Manager", baton_init, baton_start, baton_event, baton_exit);
