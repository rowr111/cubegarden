#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "i2c.h"
#include "charger.h"
#include "orchard-ui.h"

#include "orchard-test.h"
#include "test-audit.h"

#define NL SHELL_NEWLINE_STR

virtual_timer_t chg_vt;
event_source_t chg_keepalive_event;

static int keepalive_mod = 0;

int16_t ggStateofCharge(void) {
  uint8_t tx[4], rx[3];
  msg_t retval;
  uint16_t soc;
  uint16_t voltage;

  tx[0] = 0x0f; // ITE register
  i2cAcquireBus(&I2CD1);
  retval = i2cMasterTransmitTimeout(&I2CD1, LC709203_ADDR, tx, 1, rx, 3, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);
  if( retval != MSG_OK ) {
    chprintf(stream, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
  }
  //chprintf(stream, "State of charge: %.1f%%"NL, (rx[0] | (rx[1] << 8)) / 10.0);
  soc = (int16_t) (rx[0] | (rx[1] << 8)) / 10;

#if 0 // no longer needed with correct battery profile selected! woooooo....
  // hack to push SOC to 100% for UI reporting reasons
  voltage = ggVoltage();
  if( voltage > 4000 ) { // because the GG measurement is a bit noisy..may need to revisit once R20P is removed
    soc = soc + ((voltage - 4000) / 10);
    if( soc > 100 )
      soc = 100;
  }
#endif
  
  return soc;
}

int16_t ggVoltage(void) {
  uint8_t tx[4], rx[3];
  msg_t retval;
  
  tx[0] = 0x09; // voltage register
  i2cAcquireBus(&I2CD1);
  retval = i2cMasterTransmitTimeout(&I2CD1, LC709203_ADDR, tx, 1, rx, 3, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);
  if( retval != MSG_OK ) {
    chprintf(stream, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
  }
  //  chprintf(stream, "Voltage: %dmV"NL, (rx[0] | (rx[1] << 8)));
  return (rx[0] | (rx[1] << 8));
}

static uint8_t crc_update(uint8_t incrc, uint8_t indata) {
  uint8_t i;
  uint8_t data;

  data = incrc ^ indata;
  for( i = 0; i < 8; i++ ) {
    if(( data & 0x80) != 0) {
      data <<= 1;
      data ^= 0x07;
    } else {
      data <<= 1;
    }
  }
  
  return data;
}

void comp_crc8(uint8_t *tx) {
  int i;
  uint8_t data[5]; // need to include the I2C address

  // pre-pend the I2C address to the tx buffer in a local copy
  data[0] = 0x16; // they use the left-shifted version for computations
  // note for reads this changes to 0x17
  for( i = 1; i < 4; i++ ) {
    data[i] = tx[i-1];
  }
  
  data[4] = 0x00;
  for( i = 0; i < 4; i++ ) {
    data[4] = crc_update(data[4], data[i]);
  }
  
  tx[3] = data[4];
}

void ggOn(void) {
  uint8_t tx[4];
  
  tx[0] = 0x15; // power mode
  i2cAcquireBus(&I2CD1);
  tx[1] = 0x1;
  tx[2] = 0x0;
  comp_crc8(tx);
  i2cMasterTransmitTimeout(&I2CD1, LC709203_ADDR, tx, 4, NULL, 0, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);

  tx[0] = 0x12; // change of the parameter, selects a 3.7V/4.2V pack
  i2cAcquireBus(&I2CD1);
  tx[1] = 0x1;
  tx[2] = 0x0;
  comp_crc8(tx);
  i2cMasterTransmitTimeout(&I2CD1, LC709203_ADDR, tx, 4, NULL, 0, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);
}

void chgKeepaliveHandler(eventid_t id) {
  (void) id;
  uint8_t tx[2];
  uint8_t rx[2];

  tx[0] = BQ24157_STAT_ADR;
  tx[1] = 0xC0; // 32sec timer reset, enable stat pin

  i2cAcquireBus(&I2CD1);
  i2cMasterTransmitTimeout(&I2CD1, BQ24157_ADDR, tx, 2, NULL, 0, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);

  // read gg voltage register
  tx[0] = 0x09; // voltage register
  i2cAcquireBus(&I2CD1);
  i2cMasterTransmitTimeout(&I2CD1, LC709203_ADDR, tx, 1, rx, 3, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);
  
  uibat.batt_mv = rx[0] | (rx[1] << 8);
#if 0   // for debug only
  if( (keepalive_mod++ % 10) == 0 )
    chprintf(stream, " %dmV"NL, uibat.batt_mv);
#endif

  tx[0] = 0x0f; // ITE register
  i2cAcquireBus(&I2CD1);
  i2cMasterTransmitTimeout(&I2CD1, LC709203_ADDR, tx, 1, rx, 3, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);
  uibat.batt_soc = (rx[0] | (rx[1] << 8));
}

void chgSetSafety(void) {
  uint8_t tx[2];

  // 56 mOhm resistor
  // (37.4mV + 54.4mV * Vmchrg[3] + 27.2mV * Vmchrg[2] + 13.6mV * Vmchrg[1] + 6.8mV * Vmchrg[0]) / 0.056ohm = I charge
  tx[0] = BQ24157_SAFE_ADR;
  tx[1] = 0x81;  // 1639mA *max* current (limited by chip), 4.22V *maximum* regulation voltage
  i2cAcquireBus(&I2CD1);
  i2cMasterTransmitTimeout(&I2CD1, BQ24157_ADDR, tx, 2, NULL, 0, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);
}

int isCharging(void) {
  uint8_t tx[2], rx[1];
  
  tx[0] = BQ24157_STAT_ADR;
  i2cAcquireBus(&I2CD1);
  i2cMasterTransmitTimeout(&I2CD1, BQ24157_ADDR, tx, 1, rx, 1, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);
    
  switch( (rx[0] >> 4) & 0x3 ) {
  case 0:
    return(0);
  case 1:
    return(1);
  case 2:
    return(0);
  case 3:
    return(0);
  default:
    return 0;
  }
}

const char *chgStat(void) {
  uint8_t tx[2], rx[1];
  
  tx[0] = BQ24157_STAT_ADR;
  i2cAcquireBus(&I2CD1);
  i2cMasterTransmitTimeout(&I2CD1, BQ24157_ADDR, tx, 1, rx, 1, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);
    
  switch( (rx[0] >> 4) & 0x3 ) {
  case 0:
    return("Idle");
  case 1:
    tx[0] = BQ24157_SPCHG_ADR;
    i2cAcquireBus(&I2CD1);
    i2cMasterTransmitTimeout(&I2CD1, BQ24157_ADDR, tx, 1, rx, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( (rx[0] >> 4) & 0x1 )
      return("DPM mode - slow chg");
    else
      return("Fast chg - no DPM detected");
  case 2:
    return("Charged");
  case 3:
    return("Fault");
  default:
    return("Badness");
  }
}

const char *chgFault(void) {
  uint8_t tx[2], rx[1];
  
  tx[0] = BQ24157_STAT_ADR;
  i2cAcquireBus(&I2CD1);
  i2cMasterTransmitTimeout(&I2CD1, BQ24157_ADDR, tx, 1, rx, 1, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);
    
  switch( (rx[0] >> 4) & 0x3 ) {
  case 3:
    switch( rx[0] & 0x7 ) {
    case 0:
      return( "No fault" );
    case 1:
      return( "Vbus OVP" );
    case 2:
      return( "Sleep mode" );
    case 3:
      return( "Bad adapter" );
    case 4:
      return( "Battery OVP" );
    case 5:
      return( "Thermal shutdown" );
    case 6:
      return( "Timeout" );
    case 7:
      return( "No battery" );
    }
  default:
    return( "No Fault" );
  }
}

void chgAutoParams(void) {
  uint8_t tx[2];
  uint8_t rx[1];
  msg_t retval;
  
  // set battery volage
  tx[0] = BQ24157_BATV_ADR;
  // 4.2V target regulation. 3.5V offset = 0.7V coded. 0.64 + 0.04 + 0.02 = 10_0011 = 0x23
  tx[1] = 0x23 << 2 | 2; // 2 = disable otg, OTG enabled when pin is high
  i2cAcquireBus(&I2CD1);
  retval = i2cMasterTransmitTimeout(&I2CD1, BQ24157_ADDR, tx, 2, rx, 0, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);
  if( retval != MSG_OK ) {
    chprintf(stream, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
  }
    
  // special charger voltage settings
  tx[0] = BQ24157_SPCHG_ADR;
  tx[1] = 0x3; // 4.44V is DPM thresh, normal charge current sense voltage for IBAT
  i2cAcquireBus(&I2CD1);
  retval = i2cMasterTransmitTimeout(&I2CD1, BQ24157_ADDR, tx, 2, rx, 0, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);
  if( retval != MSG_OK ) {
    chprintf(stream, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
  }
  
  // target charge current + termination current
  tx[0] = BQ24157_IBAT_ADR;
  // 1.55A target current. 
  // 56 mOhm resistor
  // (37.4mV + 27.2mV * Vichrg[3] + 13.6mV * Vichrg[2] + 6.8mV * Vichrg[1]) / 0.056ohm = I charge
  // termination current offset is 3.4mV, +3.4mV/LSB
  tx[1] = (0x7 << 4) | 0x1; // 1.51A charge rate, 6.8mV/0.056 = 120mA termination
  i2cAcquireBus(&I2CD1);
  retval = i2cMasterTransmitTimeout(&I2CD1, BQ24157_ADDR, tx, 2, rx, 0, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);
  if( retval != MSG_OK ) {
    chprintf(stream, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
  }

}

void chg_cb(void *arg) {
  (void) arg;
  chSysLockFromISR();
  chEvtBroadcastI(&chg_keepalive_event);
  chVTSetI(&chg_vt, MS2ST(1000), chg_cb, NULL);
  chSysUnlockFromISR();
}

void chgStart(int force) {
  uint8_t tx[2], rx[1];
  msg_t retval;

  if( force ) {
    // set control register
    tx[0] = BQ24157_CTRL_ADR;
    tx[1] = 0x3 << 6 | 0x3 << 4 | 0x8;
    // charger mode, not HiZ, charger enabled, enable charge current term, weak battery==3.7V, Iin limit = no limit

    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, BQ24157_ADDR, tx, 2, rx, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(stream, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }
  } else {
    tx[0] = BQ24157_CTRL_ADR;
    i2cAcquireBus(&I2CD1);
    i2cMasterTransmitTimeout(&I2CD1, BQ24157_ADDR, tx, 1, rx, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    
    tx[1] = rx[0] & 0xFB; // bit 2 low to start charging
    tx[0] = BQ24157_CTRL_ADR;
    i2cAcquireBus(&I2CD1);
    i2cMasterTransmitTimeout(&I2CD1, BQ24157_ADDR, tx, 2, rx, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
  }
  palClearPad(IOPORT3, 8); // enable charging pin
    
  chVTSet(&chg_vt, MS2ST(1000), chg_cb, NULL);
}

void chargerShipMode(void) {
  uint8_t tx[2], rx[1];
  
  while( !(GPIOA->PDIR & 0x10) ) // wait until button-up before shutting down
    ;
  chThdSleepMilliseconds(50); // debounce switch

  // set control register
  tx[0] = BQ24157_CTRL_ADR;
  tx[1] = 0x3 << 6 | 0x3 << 4 | 0x8 | 0x4 | 0x2;
  // charger mode, not HiZ, charger enabled, enable charge current term, weak battery==3.7V, Iin limit = no limit
  // charger disabled, hi-Z mode, boost mode off
  
  i2cAcquireBus(&I2CD1);
  i2cMasterTransmitTimeout(&I2CD1, BQ24157_ADDR, tx, 2, rx, 0, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);

  // palSetPad(IOPORT3, 8);  // disable charging pin, set battery to HiZ mode

  while(1) {
    tx[0] = BQ24157_STAT_ADR;
    i2cAcquireBus(&I2CD1);
    i2cMasterTransmitTimeout(&I2CD1, BQ24157_ADDR, tx, 1, rx, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);

    if( rx[0] & 0x08 ) {
      chprintf(stream, "Warning: BOOST mode detected, shutdown will fail\n\r");
    }

    // force switch pin high, so it can't trigger ship mode
    GPIOA->PDDR |= 0x10;
    GPIOA->PSOR = 0x10;

    // make ship mode an input instead of driven
    PORTB->PCR[1] &= ~0x1; // select pulldown
    PORTB->PCR[1] |= 0x2; // enable pulldown
    GPIOB->PDDR &= ~0x2;  // convert from output to input
    // palClearPad(IOPORT2, 1); // force ship mode, should shut the whole thing down...
  }
}


OrchardTestResult test_charger(const char *my_name, OrchardTestType test_type) {
  (void) my_name;
  uint8_t tx[2];
  uint8_t rx[2];
  
  switch(test_type) {
  case orchardTestPoweron:
  case orchardTestTrivial:
  case orchardTestInteractive:
  case orchardTestComprehensive:
    tx[0] = BQ24157_ID_ADR;
    i2cAcquireBus(&I2CD1);
    i2cMasterTransmitTimeout(&I2CD1, BQ24157_ADDR, tx, 1, rx, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);

    // should be 010 10 0X1 = 0101_00X1 = 0x51 || 0x53
    if( !((rx[0] == 0x51) || (rx[0] == 0x53)) ) {
      return orchardResultFail;
    } else {
      return orchardResultPass;
    }
    break;
  default:
    return orchardResultNoTest;
  }
  
  return orchardResultNoTest;
}
orchard_test("charger", test_charger);


OrchardTestResult test_gasgauge(const char *my_name, OrchardTestType test_type) {
  (void) my_name;
  uint8_t tx[4], rx[3];
  uint16_t version;
  uint16_t voltage;
  
  switch(test_type) {
  case orchardTestPoweron:
  case orchardTestTrivial:
  case orchardTestInteractive:
  case orchardTestComprehensive:
    tx[0] = 0x11; // IC version
    i2cAcquireBus(&I2CD1);
    i2cMasterTransmitTimeout(&I2CD1, LC709203_ADDR, tx, 1, rx, 3, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    version = (int16_t) (rx[0] | (rx[1] << 8));

    voltage = ggVoltage();
    
    if( ((version & 0xFFF0) != 0x2AF0) || (voltage < 3000) || (voltage > 4300) ) { // seen 0x2AFF and 0x2AFE in the field
      return orchardResultFail;
    } else {
      return orchardResultPass;
    }
    break;
  default:
    return orchardResultNoTest;
  }
  
  return orchardResultNoTest;
}
orchard_test("gg", test_gasgauge);
