#include "hal.h"
#include "shell.h"
#include "chprintf.h"

#include <string.h>

void cmd_laser(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  if (argc <= 0) {
    chprintf(chp, "Usage: laser [0/1]:"SHELL_NEWLINE_STR);
    return;
  }
  
  palSetPadMode(IOPORT2, 18, PAL_MODE_OUTPUT_PUSHPULL);

  if( argv[0][0] == '1' ) {
    palSetPad(IOPORT2, 18);
  } else {
    palClearPad(IOPORT2, 18);
  }
  
  return;
}
  // FTM2_CH0 / ALT3


void cmd_pulse(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  uint16_t psc;
  uint8_t i=0;
  
  uint16_t modlen = 10000; // needs to be less than 16 bits, or 65535

  if (argc <= 0) {
    chprintf(chp, "Usage: pulse [1-%d]:"SHELL_NEWLINE_STR, modlen);
    return;
  }

  uint16_t pulse_length = (uint16_t) strtol(argv[0], NULL, 0);
  if( pulse_length <= 0 )
    pulse_length = 1;
  if( pulse_length >= modlen )
    pulse_length = modlen - 1;

  PWMDriver *pwmp = &PWMD3;
  
  SIM->SCGC6 |= SIM_SCGC6_FTM2_MASK;
  palSetPadMode(IOPORT2, 18, PAL_MODE_ALTERNATIVE_3);

  pwmp->ftm->MODE =  FTM_MODE_FTMEN_MASK;
  pwmp->ftm->SYNC =  FTM_SYNC_CNTMIN_MASK|FTM_SYNC_CNTMAX_MASK
                    | FTM_SYNC_SWSYNC_MASK | FTM_SYNC_REINIT_MASK;
  pwmp->ftm->COMBINE =  FTM_COMBINE_SYNCEN3_MASK | FTM_COMBINE_SYNCEN2_MASK
                      | FTM_COMBINE_SYNCEN1_MASK | FTM_COMBINE_SYNCEN0_MASK;
  pwmp->ftm->SYNCONF =  FTM_SYNCONF_SYNCMODE_MASK | FTM_SYNCONF_SWWRBUF_MASK |
    FTM_SYNCONF_SWRSTCNT_MASK;

  //~ pwmp->ftm->SC = 0;       /* Disable FTM counter.*/
  pwmp->ftm->CNT = 0x0000; /* Clear count register.*/

  /* Prescaler value calculation.*/
  psc = (KINETIS_SYSCLK_FREQUENCY / 1499648);  // 1499648Hz is the target
  //  chprintf(chp, "sysclk freq: %d\n\r", KINETIS_SYSCLK_FREQUENCY);
  //  chprintf(chp, "psc: %d\n\r", psc);
  //~ /* Prescaler must be power of two between 1 and 128.*/
  //osalDbgAssert(psc <= 128 && !(psc & (psc - 1)), "invalid frequency");
  //~ /* Prescaler register value determination.
     //~ Prescaler register value conveniently corresponds to bit position,
     //~ i.e., register value for prescaler CLK/64 is 6 ((1 << 6) == 64).*/
  for (i = 0; i < 8; i++) {
    if (psc == (unsigned)(1 << i)) {
      break;
    }
  }
  /*
    a value of 1499648 as the psc divider gives an increment of 
    1.36us per unit of pulse length specified on the command line,
    up to a maximum of 13.6ms given the limit of MOD at 10,000
   */
  //  chprintf(chp, "i: %x\n\r", i);

  // disable all notifications and interrupts
  pwmp->ftm->CHANNEL[0].CnSC &= ~FTM_CnSC_CHIE;
  pwmp->ftm->SC &= ~FTM_SC_TOIE;
  
  /* Set prescaler and clock mode.
     This also sets the following:
          CPWMS up-counting mode
          Timer overflow interrupt disabled
          DMA disabled.*/
  uint32_t sc_val = FTM_SC_CLKS(1) | FTM_SC_PS(i);
  pwmp->ftm->SC = sc_val;
  /* Configure period */

  pwmp->ftm->MOD = modlen; 
  pwmp->ftm->CNTIN = 0x0000;
  //  pwmp->ftm->PWMLOAD = FTM_PWMLOAD_LDOK_MASK;
  
  
  uint32_t mode = FTM_CnSC_MSB; /* Edge-aligned PWM mode.*/
  mode |= FTM_CnSC_ELSA;

  pwmp->ftm->CHANNEL[0].CnSC = mode;
  pwmp->ftm->CHANNEL[0].CnV = modlen - pulse_length;  // pulse width here
  
  pwmp->ftm->SYNC =  FTM_SYNC_CNTMIN_MASK|FTM_SYNC_CNTMAX_MASK
                    | FTM_SYNC_SWSYNC_MASK | FTM_SYNC_REINIT_MASK;
  
  pwmp->ftm->PWMLOAD = FTM_PWMLOAD_LDOK_MASK;

  // wait for the timer to overflow
  while( !(pwmp->ftm->SC & FTM_SC_TOF) )
    ;
  
  pwmp->ftm->SC = 0;
  
  return;
}
