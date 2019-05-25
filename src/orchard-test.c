#include <stdio.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "orchard.h"
#include "led.h"
#include "orchard-ui.h"
#include "touch.h"

#include "orchard-test.h"
#include "test-audit.h"

static const TestRoutine *first_test;

orchard_test_end();

static uint8_t orchard_test_index = 0;
static OrchardTestResult orchard_test_results[LED_COUNT];  // track results up to LED_COUNT

void orchardTestInit(void) {
  first_test = orchard_test_start();
  auditStart();  // start / initialize the test audit log
}

void orchardListTests(BaseSequentialStream *chp) {
  const TestRoutine *current;

  current = orchard_test_start();
  chprintf(chp, "Available tests:\n\r" );
  while (current->test_name) {
    chprintf(chp, "%s\r\n", current->test_name);
    current++;
  }
}

const TestRoutine *orchardGetTestByName(const char *name) {
  const TestRoutine *current;

  current = orchard_test_start();
  while (current->test_name) {
    if (!strncmp(name, current->test_name, 16))
      return current;
    current++;
  }
  return NULL;
}

OrchardTestResult orchardTestRun(const TestRoutine *test, uint32_t test_type) {

  return test->test_function(test->test_name, test_type);
}

OrchardTestResult orchardTestRunByName(const char *name, uint32_t test_type) {

  const TestRoutine *test = orchardGetTestByName(name);
  if (!test)
    return orchardResultNoTest;
  return orchardTestRun(test, test_type);
}

// helper routine for the test function to set LEDs all to one color and force the update
void test_led_setall(uint8_t r, uint8_t g, uint8_t b) {
  unsigned int i;
  
  for( i = 0; i < led_config.pixel_count * 3; i += 3 ) {
    led_config.final_fb[i] = g;
    led_config.final_fb[i+1] = r;
    led_config.final_fb[i+2] = b;
  }
  chSysLock();
  ledUpdate(led_config.final_fb, led_config.pixel_count);
  chSysUnlock();
}

void orchardTestRunAll(BaseSequentialStream *chp, OrchardTestType test_type) {
  const TestRoutine *cur_test;
  OrchardTestResult test_result;
  char prompt[24];
  uint32_t auditval;
  unsigned int i;
  int test_index;
  uint8_t leds_was_off = 0;
  uint32_t delay_time = 1000;
  
  cur_test = orchard_test_start();
  orchard_test_index = 0;
  for ( i = 0; i < LED_COUNT; i++ ) {
    orchard_test_results[i] = orchardResultTBD;
  }

  if( ledsOff ) { // stash LED state
    leds_was_off = 1;
  } else {
    // stop LED effects so we can use the LEDs as feedback for the test
    while(ledsOff == 0) {
      effectsStop();
      chThdYield();
      chThdSleepMilliseconds(100);
    }
  }
  
  while(cur_test->test_function) {
    test_result = cur_test->test_function(cur_test->test_name, test_type);

    if( test_type == orchardTestPoweron )
      delay_time = 350;  // a little faster for the quick poweron test
    else
      delay_time = 1000;
    
    if( test_type != orchardTestPoweron ) {
      auditUpdate(cur_test->test_name, test_type, test_result);
    } else {
      switch(test_result) {
      case orchardResultPass:
        break;
      case orchardResultFail:
        chprintf(chp, "TEST: %s subystem failed test with code %d\n\r",
                 cur_test->test_name, test_result);
        break;
      case orchardResultNoTest:
        chprintf(chp, "TEST: reminder: write test for subystem %s\n\r",
                 cur_test->test_name );
        break;
      case orchardResultUnsure:
        chprintf(stream, "TEST: %s subystem requires operator judgement to evaluate test type %d\n\r",
                 cur_test->test_name, test_result, test_type);
        break;
      default:
        // lolwut?
        break;
      }
    }
    
    cur_test++;
    orchard_test_results[orchard_test_index] = test_result;
    
    // update the test progress wheel
    for( i = 0, test_index = 0; i < led_config.pixel_count * 3; i += 3, test_index++ ) {
      switch( orchard_test_results[test_index] ) {
      case orchardResultPass:
	led_config.final_fb[i] = 255;  // green
	led_config.final_fb[i+1] = 0;
	led_config.final_fb[i+2] = 0;
	break;
      case orchardResultFail:
	led_config.final_fb[i] = 0;
	led_config.final_fb[i+1] = 255;  // red
	led_config.final_fb[i+2] = 0;
	break;
      case orchardResultUnsure:
      case orchardResultNoTest:
	led_config.final_fb[i] = 96;    // dim yellow
	led_config.final_fb[i+1] = 96;
	led_config.final_fb[i+2] = 2;
	break;
      default:
	led_config.final_fb[i] = 0;    // off
	led_config.final_fb[i+1] = 0;
	led_config.final_fb[i+2] = 0;
      }
    }
    chSysLock();
    ledUpdate(led_config.final_fb, led_config.pixel_count);
    chSysUnlock();
    chThdSleepMilliseconds(delay_time);
    
    orchard_test_index++;
  }
  if( test_type == orchardTestInteractive ) {
    auditval = auditCheck(test_type);
    chThdSleepMilliseconds(2000); // give 2 seconds for FLASH udpates to happen after test is done, it fucks wit the UART
    
    if( auditval > 0xFFFF ) {
      // failure
      chprintf(chp, "FAIL\n\r" );
      chsnprintf(prompt, sizeof(prompt), "0x%x", auditval);
      chprintf(chp, "FAIL %s\n\r", prompt );
      orchardTestPrompt( "FAIL", prompt, -30 );
    } else {
      // pass
      chprintf(chp, "PASS\n\r" );
      chsnprintf(prompt, sizeof(prompt), "0x%x", auditval);
      chprintf(chp, "PASS %s\n\r", prompt );
      orchardTestPrompt( "NO ERRORS", prompt, -30 );
    }
  }

  if( !leds_was_off ) {
    // resume effects
    effectsStart();
  }
}

// used to draw UI prompts for tests to the screen
// print up to 2 lines of text
// interaction_delay specifies how long we should wait before we declare failure
//   0 means don't delay
//   negative numbers delay that amount of seconds, but don't print a timeout failure
//    (meant for prompts that need to be read, but not interacted with)
extern uint8_t test_switch;
OrchardTestResult orchardTestPrompt(char *line1, char *line2,
                                    int8_t interaction_delay) {
  (void) line2; // in this LED version we ignore line 2 of the output
  unsigned int i, j;
  
  if( ledsOff != 0 ) {
    chprintf(stream, "orchardTestPrompt() called, but LED effects are still running!\n\r" );
    chprintf(stream, "orchardTestPrompt() can't render output to LEDs, aborting.\n\r" );
    return orchardResultFail;
  }

  // flash the LED that is being tested
  for( i = 0; i < 2; i ++ ) {
    led_config.final_fb[orchard_test_index*3] = 255;
    led_config.final_fb[orchard_test_index*3+1] = 255;
    led_config.final_fb[orchard_test_index*3+2] = 255;
    chSysLock();
    ledUpdate(led_config.final_fb, led_config.pixel_count);
    chSysUnlock();
    chThdSleepMilliseconds(250);
    led_config.final_fb[orchard_test_index*3] = 0;
    led_config.final_fb[orchard_test_index*3+1] = 0;
    led_config.final_fb[orchard_test_index*3+2] = 0;
    chThdSleepMilliseconds(250);
  }

  if( interaction_delay > 0 ) {
    test_switch = 0;
    while( !test_switch ) {
      led_config.final_fb[orchard_test_index*3] = 159;  // violet color to indicate switch hit needed
      led_config.final_fb[orchard_test_index*3+1] = 5;
      led_config.final_fb[orchard_test_index*3+2] = 255;
      chSysLock();
      ledUpdate(led_config.final_fb, led_config.pixel_count);
      chSysUnlock();
      chThdSleepMilliseconds(250);
      led_config.final_fb[orchard_test_index*3] = 0;
      led_config.final_fb[orchard_test_index*3+1] = 0;
      led_config.final_fb[orchard_test_index*3+2] = 0;
      chSysLock();
      ledUpdate(led_config.final_fb, led_config.pixel_count);
      chSysUnlock();
      chThdSleepMilliseconds(250);
    }
  } else if( interaction_delay < 0 ) {
    int red = 0;
    int green = 0;
    // this is used to indicate final test
    if( strncmp( "NO ERRORS", line1, 8 ) == 0 ) {
      green = 255;
    } else {
      red = 255;
    }

    // flash result code for 15 seconds
    for( i = 0; i < 30; i ++ ) {
      for( j = 0; j < led_config.pixel_count; j++ ) {
	led_config.final_fb[j*3] = red;
	led_config.final_fb[j*3+1] = green;
	led_config.final_fb[j*3+2] = 0;
      }
      chSysLock();
      ledUpdate(led_config.final_fb, led_config.pixel_count);
      chSysUnlock();
      chThdSleepMilliseconds(250);
      for( j = 0; j < led_config.pixel_count; j++ ) {
	led_config.final_fb[orchard_test_index*3] = 0;
	led_config.final_fb[orchard_test_index*3+1] = 0;
	led_config.final_fb[orchard_test_index*3+2] = 0;
      }
      chSysLock();
      ledUpdate(led_config.final_fb, led_config.pixel_count);
      chSysUnlock();
      chThdSleepMilliseconds(250);
    }
  }
  return 0;
}
