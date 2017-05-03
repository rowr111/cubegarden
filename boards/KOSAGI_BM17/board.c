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

#include "hal.h"

#if HAL_USE_PAL || defined(__DOXYGEN__)
/**
 * @brief   PAL setup.
 * @details Digital I/O ports static configuration as defined in @p board.h.
 *          This variable is used by the HAL when initializing the PAL driver.
 */
const PALConfig pal_default_config =
{
  .ports = {
    {
      .port = IOPORT1,  // PORTA
      .pads = {
        /* PTA0*/ PAL_MODE_ALTERNATIVE_7,   /* PTA1*/ PAL_MODE_UNCONNECTED,     /* PTA2*/ PAL_MODE_UNCONNECTED,
        /* PTA3*/ PAL_MODE_ALTERNATIVE_7,   /* PTA4*/ PAL_MODE_UNCONNECTED,     /* PTA5*/ PAL_MODE_UNCONNECTED,
        /* PTA6*/ PAL_MODE_UNCONNECTED,     /* PTA7*/ PAL_MODE_UNCONNECTED,     /* PTA8*/ PAL_MODE_UNCONNECTED,
        /* PTA9*/ PAL_MODE_UNCONNECTED,     /*PTA10*/ PAL_MODE_UNCONNECTED,     /*PTA11*/ PAL_MODE_UNCONNECTED,
        /*PTA12*/ PAL_MODE_OUTPUT_PUSHPULL, /*PTA13*/ PAL_MODE_INPUT,           /*PTA14*/ PAL_MODE_UNCONNECTED, // led0, rstat1, nc
        /*PTA15*/ PAL_MODE_UNCONNECTED,     /*PTA16*/ PAL_MODE_UNCONNECTED,     /*PTA17*/ PAL_MODE_UNCONNECTED,
        /*PTA18*/ PAL_MODE_OUTPUT_PUSHPULL, /*PTA19*/ PAL_MODE_OUTPUT_PUSHPULL, /*PTA20*/ PAL_MODE_UNCONNECTED, // oled_res_n, radio_reset
        /*PTA21*/ PAL_MODE_UNCONNECTED,     /*PTA22*/ PAL_MODE_UNCONNECTED,     /*PTA23*/ PAL_MODE_UNCONNECTED,
        /*PTA24*/ PAL_MODE_UNCONNECTED,     /*PTA25*/ PAL_MODE_UNCONNECTED,     /*PTA26*/ PAL_MODE_UNCONNECTED,
        /*PTA27*/ PAL_MODE_UNCONNECTED,     /*PTA28*/ PAL_MODE_UNCONNECTED,     /*PTA29*/ PAL_MODE_UNCONNECTED,
        /*PTA30*/ PAL_MODE_UNCONNECTED,     /*PTA31*/ PAL_MODE_UNCONNECTED, 
      },
    },
    {
      .port = IOPORT2,  // PORTB
      .pads = {
        /* PTB0*/ PAL_MODE_INPUT_PULLUP,    /* PTB1*/ PAL_MODE_OUTPUT_PUSHPULL, /* PTB2*/ PAL_MODE_ALTERNATIVE_2, // touchint, shipmode_n, i2c0_scl
        /* PTB3*/ PAL_MODE_ALTERNATIVE_2,   /* PTB4*/ PAL_MODE_UNCONNECTED,     /* PTB5*/ PAL_MODE_UNCONNECTED, // i2c0_sda
        /* PTB6*/ PAL_MODE_UNCONNECTED,     /* PTB7*/ PAL_MODE_UNCONNECTED,     /* PTB8*/ PAL_MODE_UNCONNECTED,
        /* PTB9*/ PAL_MODE_UNCONNECTED,     /*PTB10*/ PAL_MODE_UNCONNECTED,     /*PTB11*/ PAL_MODE_UNCONNECTED,
        /*PTB12*/ PAL_MODE_UNCONNECTED,     /*PTB13*/ PAL_MODE_UNCONNECTED,     /*PTB14*/ PAL_MODE_UNCONNECTED,
        /*PTB15*/ PAL_MODE_UNCONNECTED,     /*PTB16*/ PAL_MODE_INPUT,           /*PTB17*/ PAL_MODE_INPUT, // nc, rstat5, rstat4
        /*PTB18*/ PAL_MODE_INPUT,           /*PTB19*/ PAL_MODE_INPUT,           /*PTB20*/ PAL_MODE_UNCONNECTED,  //  rstat3, rstat2
        /*PTB21*/ PAL_MODE_UNCONNECTED,     /*PTB22*/ PAL_MODE_UNCONNECTED,     /*PTB23*/ PAL_MODE_UNCONNECTED,
        /*PTB24*/ PAL_MODE_UNCONNECTED,     /*PTB25*/ PAL_MODE_UNCONNECTED,     /*PTB26*/ PAL_MODE_UNCONNECTED,
        /*PTB27*/ PAL_MODE_UNCONNECTED,     /*PTB28*/ PAL_MODE_UNCONNECTED,     /*PTB29*/ PAL_MODE_UNCONNECTED,
        /*PTB30*/ PAL_MODE_UNCONNECTED,     /*PTB31*/ PAL_MODE_UNCONNECTED,
      },
    },
    {
      .port = IOPORT3,  // PORTC
      .pads = {
        /* PTC0*/ PAL_MODE_INPUT,           /* PTC1*/ PAL_MODE_INPUT_PULLUP,    /* PTC2*/ PAL_MODE_OUTPUT_PUSHPULL, //batt_stat, accel_int, p5v_en
        /* PTC3*/ PAL_MODE_ALTERNATIVE_7,   /* PTC4*/ PAL_MODE_ALTERNATIVE_7,   /* PTC5*/ PAL_MODE_ALTERNATIVE_4, // lpuart dbg rx, tx, i2s0_rxd0
        /* PTC6*/ PAL_MODE_INPUT_PULLUP,    /* PTC7*/ PAL_MODE_ALTERNATIVE_4,   /* PTC8*/ PAL_MODE_OUTPUT_PUSHPULL,// gg_alarmb, i2s0_rx_fs, batt_srst
        /* PTC9*/ PAL_MODE_ALTERNATIVE_4,   /*PTC10*/ PAL_MODE_ALTERNATIVE_2,   /*PTC11*/ PAL_MODE_ALTERNATIVE_2, // i2s0_rx_bclk, i2c1
        /*PTC12*/ PAL_MODE_UNCONNECTED,     /*PTC13*/ PAL_MODE_UNCONNECTED,     /*PTC14*/ PAL_MODE_UNCONNECTED,
        /*PTC15*/ PAL_MODE_UNCONNECTED,     /*PTC16*/ PAL_MODE_UNCONNECTED,     /*PTC17*/ PAL_MODE_UNCONNECTED,
        /*PTC18*/ PAL_MODE_UNCONNECTED,     /*PTC19*/ PAL_MODE_UNCONNECTED,     /*PTC20*/ PAL_MODE_UNCONNECTED,
        /*PTC21*/ PAL_MODE_UNCONNECTED,     /*PTC22*/ PAL_MODE_UNCONNECTED,     /*PTC23*/ PAL_MODE_UNCONNECTED,
        /*PTC24*/ PAL_MODE_UNCONNECTED,     /*PTC25*/ PAL_MODE_UNCONNECTED,     /*PTC26*/ PAL_MODE_UNCONNECTED,
        /*PTC27*/ PAL_MODE_UNCONNECTED,     /*PTC28*/ PAL_MODE_UNCONNECTED,     /*PTC29*/ PAL_MODE_UNCONNECTED,
        /*PTC30*/ PAL_MODE_UNCONNECTED,     /*PTC31*/ PAL_MODE_UNCONNECTED,
      },
    },
    {
      .port = IOPORT4,  // PORTD
      .pads = {
        /* PTD0*/ PAL_MODE_ALTERNATIVE_2,   /* PTD1*/ PAL_MODE_ALTERNATIVE_2,   /* PTD2*/ PAL_MODE_ALTERNATIVE_2, // spi0_pcs0, spi0_clk, spi0_mosi
        /* PTD3*/ PAL_MODE_ALTERNATIVE_2,   /* PTD4*/ PAL_MODE_OUTPUT_PUSHPULL, /* PTD5*/ PAL_MODE_ALTERNATIVE_7, // spi0_miso, spi1_pcs0, spi1_sck
        /* PTD6*/ PAL_MODE_ALTERNATIVE_7,   /* PTD7*/ PAL_MODE_ALTERNATIVE_7,   /* PTD8*/ PAL_MODE_UNCONNECTED, // spi1_mosi, spi1_miso
        /* PTD9*/ PAL_MODE_UNCONNECTED,     /*PTD10*/ PAL_MODE_UNCONNECTED,     /*PTD11*/ PAL_MODE_UNCONNECTED,
        /*PTD12*/ PAL_MODE_UNCONNECTED,     /*PTD13*/ PAL_MODE_UNCONNECTED,     /*PTD14*/ PAL_MODE_UNCONNECTED,
        /*PTD15*/ PAL_MODE_UNCONNECTED,     /*PTD16*/ PAL_MODE_UNCONNECTED,     /*PTD17*/ PAL_MODE_UNCONNECTED,
        /*PTD18*/ PAL_MODE_UNCONNECTED,     /*PTD19*/ PAL_MODE_UNCONNECTED,     /*PTD20*/ PAL_MODE_UNCONNECTED,
        /*PTD21*/ PAL_MODE_UNCONNECTED,     /*PTD22*/ PAL_MODE_UNCONNECTED,     /*PTD23*/ PAL_MODE_UNCONNECTED,
        /*PTD24*/ PAL_MODE_UNCONNECTED,     /*PTD25*/ PAL_MODE_UNCONNECTED,     /*PTD26*/ PAL_MODE_UNCONNECTED,
        /*PTD27*/ PAL_MODE_UNCONNECTED,     /*PTD28*/ PAL_MODE_UNCONNECTED,     /*PTD29*/ PAL_MODE_UNCONNECTED,
        /*PTD30*/ PAL_MODE_UNCONNECTED,     /*PTD31*/ PAL_MODE_UNCONNECTED,
      },
    },
    {
      .port = IOPORT5,  // PORTE
      .pads = {
        /* PTE0*/ PAL_MODE_OUTPUT_PUSHPULL, /* PTE1*/ PAL_MODE_INPUT_PULLUP,    /* PTE2*/ PAL_MODE_UNCONNECTED, // uart1sel, uart1rx
        /* PTE3*/ PAL_MODE_UNCONNECTED,     /* PTE4*/ PAL_MODE_UNCONNECTED,     /* PTE5*/ PAL_MODE_UNCONNECTED,
        /* PTE6*/ PAL_MODE_UNCONNECTED,     /* PTE7*/ PAL_MODE_UNCONNECTED,     /* PTE8*/ PAL_MODE_UNCONNECTED,
        /* PTE9*/ PAL_MODE_UNCONNECTED,     /*PTE10*/ PAL_MODE_UNCONNECTED,     /*PTE11*/ PAL_MODE_UNCONNECTED,
        /*PTE12*/ PAL_MODE_UNCONNECTED,     /*PTE13*/ PAL_MODE_UNCONNECTED,     /*PTE14*/ PAL_MODE_UNCONNECTED,
        /*PTE15*/ PAL_MODE_UNCONNECTED,     /*PTE16*/ PAL_MODE_UNCONNECTED,     /*PTE17*/ PAL_MODE_UNCONNECTED,
        /*PTE18*/ PAL_MODE_UNCONNECTED,     /*PTE19*/ PAL_MODE_UNCONNECTED,     /*PTE20*/ PAL_MODE_UNCONNECTED,
        /*PTE21*/ PAL_MODE_UNCONNECTED,     /*PTE22*/ PAL_MODE_UNCONNECTED,     /*PTE23*/ PAL_MODE_UNCONNECTED,
        /*PTE24*/ PAL_MODE_UNCONNECTED,     /*PTE25*/ PAL_MODE_UNCONNECTED,     /*PTE26*/ PAL_MODE_UNCONNECTED,
        /*PTE27*/ PAL_MODE_UNCONNECTED,     /*PTE28*/ PAL_MODE_UNCONNECTED,     /*PTE29*/ PAL_MODE_UNCONNECTED,
        /*PTE30*/ PAL_MODE_UNCONNECTED,     /*PTE31*/ PAL_MODE_UNCONNECTED,
      },
    },
  },
};
#endif

#if HAL_USE_MMC_SPI || defined(__DOXYGEN__)
/**
 * @brief   MMC_SPI card detection.
 */
bool mmc_lld_is_card_inserted(MMCDriver *mmcp) {

  (void)mmcp;
  if( palReadPad(GPIOB, 16) == PAL_HIGH) {
    // floats high when no card is present
    return false;
  } else {
    return true;
  }
}

/**
 * @brief   MMC_SPI card write protection detection.
 */
bool mmc_lld_is_write_protected(MMCDriver *mmcp) {

  (void)mmcp;
  // there is no write detect on this board, so always writeable
  return false;
}
#endif

/**
 * @brief   Early initialization code.
 * @details This initialization must be performed just after stack setup
 *          and before any other initialization.
 */
void __early_init(void) {

  *((unsigned int *) 0x40048038) |= 0x400; // enable clock to port 2
  *((unsigned int *) 0x4004A004) = 0x104; // select GPIO
  *((unsigned int *) 0x400ff054) |= 0x2; // set DDR to output
  *((unsigned int *) 0x400ff044) |= 0x2; // set output
  
  k22x_clock_init();
  SystemCoreClockUpdate();
}

/**
 * @brief   Board-specific initialization code.
 * @todo    Add your board-specific code, if any.
 */
void boardInit(void) {
}
