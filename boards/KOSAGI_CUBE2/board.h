/*
    ChibiOS - Copyright (C) 2006..2015 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef _BOARD_H_
#define _BOARD_H_

/*
 * Setup for Freescale Freedom K20D50M board.
 */

/*
 * Board identifier.
 */
#define BOARD_KOSAGI_CUBE2
#define BOARD_NAME                  "Cubegarden 2020"

#define BOARD_XTAL_MHZ  32768

#define K22x10

#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
#endif
  void boardInit(void);
#ifdef __cplusplus
}
#endif
#endif /* _FROM_ASM_ */



/* MCG mode constants */

#define MCG_MODE_FEI                   0U
#define MCG_MODE_FBI                   1U
#define MCG_MODE_BLPI                  2U
#define MCG_MODE_FEE                   3U
#define MCG_MODE_FBE                   4U
#define MCG_MODE_BLPE                  5U
#define MCG_MODE_PBE                   6U
#define MCG_MODE_PEE                   7U

/* Predefined clock setups
   0 ... Default part configuration
         Multipurpose Clock Generator (MCG) in FEI mode.
         Reference clock source for MCG module: Slow internal reference clock
         Core clock = 20.97152MHz
         Bus clock  = 20.97152MHz
   1 ... Maximum achievable clock frequency configuration
         Multipurpose Clock Generator (MCG) in PEE mode.
         Reference clock source for MCG module: System oscillator 0 reference clock
         Core clock = 100MHz
         Bus clock  = 50MHz
   2 ... Chip internally clocked, ready for Very Low Power Run mode
         Multipurpose Clock Generator (MCG) in BLPI mode.
         Reference clock source for MCG module: Fast internal reference clock
         Core clock = 4MHz
         Bus clock  = 4MHz
   3 ... Chip externally clocked, ready for Very Low Power Run mode
         Multipurpose Clock Generator (MCG) in BLPE mode.
         Reference clock source for MCG module: System oscillator 0 reference clock
         Core clock = 4MHz
         Bus clock  = 4MHz
   4 ... USB clock setup
         Multipurpose Clock Generator (MCG) in PEE mode.
         Reference clock source for MCG module: System oscillator 0 reference clock
         Core clock = 96MHz
         Bus clock  = 48MHz
*/

/* Define clock source values */

#define CPU_XTAL_CLK_HZ                32768U            /* Value of the external crystal or oscillator clock frequency of the system oscillator (OSC) in Hz */
#define CPU_INT_SLOW_CLK_HZ            32768U              /* Value of the slow internal oscillator clock frequency in Hz */
#define CPU_INT_FAST_CLK_HZ            4000000U            /* Value of the fast internal oscillator clock frequency in Hz */

/* RTC oscillator setting */
/* #undef SYSTEM_RTC_CR_VALUE */                           /* RTC oscillator not enabled. Commented out for MISRA compliance. */

/* Low power mode enable */
/* SMC_PMPROT: AVLP=1,ALLS=1,AVLLS=1 */
#define SYSTEM_SMC_PMPROT_VALUE        0x2AU               /* SMC_PMPROT */

/* Internal reference clock trim */
/* #undef SLOW_TRIM_ADDRESS */                             /* Slow oscillator not trimmed. Commented out for MISRA compliance. */
/* #undef SLOW_FINE_TRIM_ADDRESS */                        /* Slow oscillator not trimmed. Commented out for MISRA compliance. */
/* #undef FAST_TRIM_ADDRESS */                             /* Fast oscillator not trimmed. Commented out for MISRA compliance. */
/* #undef FAST_FINE_TRIM_ADDRESS */                        /* Fast oscillator not trimmed. Commented out for MISRA compliance. */

//  #define DEFAULT_SYSTEM_CLOCK         100000000U          /* Default System clock value */
  #define MCG_MODE                     MCG_MODE_FEI /* Clock generator mode */
#define SYSTEM_MCG_C1_VALUE          0x07U
// select fll or pll; divide by 1; slow internal selected; inter ref on; internal ref is enabled

#define SYSTEM_MCG_C2_VALUE          0x04U               /* MCG_C2 */ // low frequency range, oscillator select
// interrupt on osc0 loss, low range xosc, low power, oscillator requested, fll not disablefd, slow internal ref selected

  #define SYSTEM_MCG_C4_VALUE          0xe0U               /* MCG_C4 */
  /* MCG_SC: ATME=0,ATMS=0,ATMF=0,FLTPRSRV=0,FCRDIV=0,LOCS0=0 */
// dmx32 = 32.768khz, 2929 ref range, 966 mhz fll factor, dco 80-100mhz, no trim

  #define SYSTEM_MCG_SC_VALUE          0x00U               /* MCG_SC */

/* MCG_C5: PLLCLKEN0=0,PLLSTEN0=0,PRDIV0=1 */
  #define SYSTEM_MCG_C5_VALUE          0x01U               /* MCG_C5 */
// pll is disabled, prdiv is divide by 2, but should't matter?

/* MCG_C6: LOLIE0=0,PLLS=1,CME0=0,VDIV0=1 */
  #define SYSTEM_MCG_C6_VALUE          0x01U               /* MCG_C6 */
// fll select, no clock monitoring. vco multiply factor is 25 but shouldn't matter

  /* MCG_C7: OSCSEL=0 */
  #define SYSTEM_MCG_C7_VALUE          0x00U               /* MCG_C7 */
// select oscillator (not RTC)

  /* OSC_CR: ERCLKEN=1,EREFSTEN=0,SC2P=0,SC4P=0,SC8P=0,SC16P=0 */
  #define SYSTEM_OSC_CR_VALUE          0x00U               /* OSC_CR */
// external ref disabled, no caps selected

  /* SMC_PMCTRL: LPWUI=0,RUNM=0,STOPA=0,STOPM=0 */
  #define SYSTEM_SMC_PMCTRL_VALUE      0x00U               /* SMC_PMCTRL */

  /* SIM_CLKDIV1: OUTDIV1=0,OUTDIV2=1,OUTDIV3=1,OUTDIV4=3 */
  #define SYSTEM_SIM_CLKDIV1_VALUE     0x01130000U         /* SIM_CLKDIV1 */
// outdiv1 = divide by 1 = 96mhz core
// outdiv2 = divide by 2 = 48mhz bus
// outdiv3 = divide by 2 = 48mhz flexbus
// outdiv4 = divide by 4 = 24mhz flash clock

  /* SIM_SOPT1: USBREGEN=0,USBSSTBY=0,USBVSTBY=0,OSC32KSEL=3,RAMSIZE=0 */
  #define SYSTEM_SIM_SOPT1_VALUE       0x00009000U         /* SIM_SOPT1 */

  /* SIM_SOPT2: SDHCSRC=0,USBSRC=0,PLLFLLSEL=1,TRACECLKSEL=0,PTD7PAD=0,FBSL=0,CLKOUTSEL=0,RTCCLKOUTSEL=0 */
  #define SYSTEM_SIM_SOPT2_VALUE       0x00000000U         /* SIM_SOPT2 */
// select FLL 

/**
 * @brief System clock frequency (core clock)
 *
 * The system clock frequency supplied to the SysTick timer and the processor
 * core clock. This variable can be used by the user application to setup the
 * SysTick timer or configure other parameters. It may also be used by debugger to
 * query the frequency of the debug timer or configure the trace clock speed
 * SystemCoreClock is initialized with a correct predefined value.
 */
extern uint32_t SystemCoreClock;

#endif /* _BOARD_H_ */
