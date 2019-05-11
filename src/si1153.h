//-----------------------------------------------------------------------------
// Si115xDrv.h
//-----------------------------------------------------------------------------
// Copyright 2018 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// File Description: This file provides the registers and functions for controlling
//						the si115x
//
// Target:         MCU agnostic
// Command Line:   None
//
//-----------------------------------------------------------------------------

#ifndef SI115X_DRV_H_
#define SI115X_DRV_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HANDLE
typedef void *HANDLE;
#endif

//Defines for different parts:
#define SI1133_AA00_UV                   1
#define SI1153_AA00_GESTURE              2 // not supported, no demo for this sensor
#define SI1153_AA09_LONG_RANGE_PROX      3
#define SI1153_AA9X_SUNLIGHT_IMMUNE_PROX 4
#define SI1153_AA00_PROX                 5
#define SI1153_AA00_PROX_ALS             6
#define SI1133_AA00_UV_ALS               7
#define SI11XX_NONE                      0

//I2C / part address Defines:
#define SI1133_I2C_ADDR					 0x55
#define SI1153_I2C_ADDR					 0x53
#define SI1153_REG_HW_ID        		 0x01
#define SI1133_REG_HW_ID        		 0x01
#define SI1133_HW_ID					 0x03
#define SI1153_REG_PART_ID        		 0x00
#define SI1153_PART_ID					 0x53
#define SI1153_PROX_HW_ID				 0x00
#define SI1153_LR_PROX_HW_ID			 0x01
#define SI1153_SUN_PROX_HW_ID			 0x02

//Values
#define SENSOR_OVERFLOW_VALUE				 0x7FFFFF
#define SENSOR_OVERFLOW_DISPLAY_MAGNITUDE	 48500

/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/
typedef struct Si115xSample
{
    uint8_t     irq_status;
    int32_t     ch0;
    int32_t     ch1;
    int32_t     ch2;
    int32_t     ch3;
} Si115xSample_t;


typedef struct i2cSensorConfig
{
  I2CDriver			*i2cPort;   /**< I2C port Si115x is connected to */
  uint8_t				i2cAddress; /**< I2C address of Si115x */
  ioportid_t		irqPort;    /**< Port for Si115x INT pin */
  uint8_t				irqPin;     /**< Pin for Si115x INT pin */
} i2cSensorConfig_t;


/*******************************************************************************
 ************************** Si115x I2C Registers *******************************
 ******************************************************************************/
//
// I2C Registers
//
#define SI115x_REG_PART_ID      0x00
#define SI115x_REG_HW_ID        0x01
#define SI115x_REG_REV_ID       0x02
#define SI115x_REG_HOSTIN0      0x0A
#define SI115x_REG_COMMAND      0x0B
#define SI115x_REG_IRQ_ENABLE   0x0F
#define SI115x_REG_RESPONSE1    0x10
#define SI115x_REG_RESPONSE0    0x11
#define SI115x_REG_IRQ_STATUS   0x12
#define SI115x_REG_HOSTOUT0     0x13
#define SI115x_REG_HOSTOUT1     0x14
#define SI115x_REG_HOSTOUT2     0x15
#define SI115x_REG_HOSTOUT3     0x16
#define SI115x_REG_HOSTOUT4     0x17
#define SI115x_REG_HOSTOUT5     0x18
#define SI115x_REG_HOSTOUT6     0x19
#define SI115x_REG_HOSTOUT7     0x1A
#define SI115x_REG_HOSTOUT8     0x1B
#define SI115x_REG_HOSTOUT9     0x1C
#define SI115x_REG_HOSTOUT10    0x1D
#define SI115x_REG_HOSTOUT11    0x1E
#define SI115x_REG_HOSTOUT12    0x1F
#define SI115x_REG_HOSTOUT13    0x20
#define SI115x_REG_HOSTOUT14    0x21
#define SI115x_REG_HOSTOUT15    0x22
#define SI115x_REG_HOSTOUT16    0x23
#define SI115x_REG_HOSTOUT17    0x24
#define SI115x_REG_HOSTOUT18    0x25
#define SI115x_REG_HOSTOUT19    0x26
#define SI115x_REG_HOSTOUT20    0x27
#define SI115x_REG_HOSTOUT21    0x28
#define SI115x_REG_HOSTOUT22    0x29
#define SI115x_REG_HOSTOUT23    0x2A
#define SI115x_REG_HOSTOUT24    0x2B
#define SI115x_REG_HOSTOUT25    0x2C


/*******************************************************************************
 ************************** Si115x I2C Parameter Offsets ***********************
 ******************************************************************************/
#define SI115x_PARAM_I2C_ADDR          0x00
#define SI115x_PARAM_CH_LIST           0x01
#define SI115x_PARAM_ADCCONFIG0        0x02
#define SI115x_PARAM_ADCSENS0          0x03
#define SI115x_PARAM_ADCPOST0          0x04
#define SI115x_PARAM_MEASCONFIG0       0x05
#define SI115x_PARAM_ADCCONFIG1        0x06
#define SI115x_PARAM_ADCSENS1          0x07
#define SI115x_PARAM_ADCPOST1          0x08
#define SI115x_PARAM_MEASCONFIG1       0x09
#define SI115x_PARAM_ADCCONFIG2        0x0A
#define SI115x_PARAM_ADCSENS2          0x0B
#define SI115x_PARAM_ADCPOST2          0x0C
#define SI115x_PARAM_MEASCONFIG2       0x0D
#define SI115x_PARAM_ADCCONFIG3        0x0E
#define SI115x_PARAM_ADCSENS3          0x0F
#define SI115x_PARAM_ADCPOST3          0x10
#define SI115x_PARAM_MEASCONFIG3       0x11
#define SI115x_PARAM_ADCCONFIG4        0x12
#define SI115x_PARAM_ADCSENS4          0x13
#define SI115x_PARAM_ADCPOST4          0x14
#define SI115x_PARAM_MEASCONFIG4       0x15
#define SI115x_PARAM_ADCCONFIG5        0x16
#define SI115x_PARAM_ADCSENS5          0x17
#define SI115x_PARAM_ADCPOST5          0x18
#define SI115x_PARAM_MEASCONFIG5       0x19
#define SI115x_PARAM_MEASRATE_H        0x1A
#define SI115x_PARAM_MEASRATE_L        0x1B
#define SI115x_PARAM_MEASCOUNT0        0x1C
#define SI115x_PARAM_MEASCOUNT1        0x1D
#define SI115x_PARAM_MEASCOUNT2        0x1E
#define SI115x_PARAM_LED1_A            0x1F
#define SI115x_PARAM_LED1_B            0x20
#define SI115x_PARAM_LED2_A            0x21
#define SI115x_PARAM_LED2_B            0x22
#define SI115x_PARAM_LED3_A            0x23
#define SI115x_PARAM_LED3_B            0x24
#define SI115x_PARAM_THRESHOLD0_H      0x25
#define SI115x_PARAM_THRESHOLD0_L      0x26
#define SI115x_PARAM_THRESHOLD1_H      0x27
#define SI115x_PARAM_THRESHOLD1_L      0x28
#define SI115x_PARAM_THRESHOLD2_H      0x29
#define SI115x_PARAM_THRESHOLD2_L      0x2A
#define SI115x_PARAM_BURST             0x2B

#define SI115x_CMD_NOP                 0x00
#define SI115x_CMD_RESET               0x01
#define SI115x_CMD_NEW_ADDR            0x02
#define SI115x_CMD_FORCE_CH            0x11
#define SI115x_CMD_PAUSE_CH            0x12
#define SI115x_CMD_AUTO_CH             0x13
#define SI115x_CMD_PARAM_SET           0x80
#define SI115x_CMD_PARAM_QUERY         0x40

/*******************************************************************************
 *******    Si115x Register and Parameter Bit Definitions  *********************
 ******************************************************************************/
#define SI115x_RSP0_CHIPSTAT_MASK      0xe0
#define SI115x_RSP0_COUNTER_MASK       0x1f
#define SI115x_RSP0_SLEEP              0x20

/***************************************************************************//**
 * @brief
 *   Initialize SI115x to sense proximity or ambient light
 ******************************************************************************/
int16_t Si115xInitProxAls( HANDLE si115x_handle, bool proxOnly );

/***************************************************************************//**
 * @brief
 *   Initialize SI115x to sense long range proximity
 ******************************************************************************/
int16_t Si115xInitLongRangeProx( HANDLE si115x_handle );

/***************************************************************************//**
 * @brief
 *   Initialize SI115x to sense proximity even under sun conditions.
 ******************************************************************************/
int16_t Si115xInitSunlightImmuneProx( HANDLE si115x_handle );

/***************************************************************************//**
 * @brief
 *   Initialize SI113x to sense UV light or ambient light.
 ******************************************************************************/
int16_t Si1133InitUvAls( HANDLE si115x_handle );
/***************************************************************************//**
 * @brief
 *   Initialize SI115x to sense gestures.
 ******************************************************************************/
int16_t Si115xInitGesture( HANDLE si115x_handle );

/***************************************************************************//**
 * @brief
 *   Read the data after a measurement. To take a measurement, use SI115xForce.
 ******************************************************************************/
void Si115xHandler(HANDLE si115x_handle, Si115xSample_t *samples);


/*******************************************************************************
 ***************   Functions Needed by Si115x_functions.c   ********************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Write to a register on the Si115x
 ******************************************************************************/
int16_t Si115xWriteToRegister(HANDLE sensorConfig, uint8_t address, uint8_t data);

/***************************************************************************//**
 * @brief
 *   Read from a register on the Si115x
 ******************************************************************************/
int16_t Si115xReadFromRegister(HANDLE sensorConfig, uint8_t address);

/***************************************************************************//**
 * @brief
 *   Write to a block of registers
 ******************************************************************************/
int16_t Si115xBlockWrite(       HANDLE   si115x_handle,
                                uint8_t  address,
                                uint8_t  length,
                                uint8_t* values);

/***************************************************************************//**
 * @brief
 *   Read from a block of registers
 ******************************************************************************/
int16_t Si115xBlockRead(        HANDLE   si115x_handle,
                                uint8_t  address,
                                uint8_t  length,
                                uint8_t* values);

// Delay function used by reset
void Si115xDelay_10ms(void);

/*******************************************************************************
 ***************   Functions supplied by Si115x_functions.c   ******************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Resets the Si115x/33, clears any interrupts and initializes the HW_KEY
 *   register.
 * @param[in] si115x_handle
 *   The programmer's toolkit handle
 * @retval  0
 *   Success
 * @retval  <0
 *   Error
 ******************************************************************************/
int16_t  Si115xReset(           HANDLE  si115x_handle);

/***************************************************************************//**
 * @brief
 *   Sends a NOP command to the Si115x/33
 * @param[in] si115x_handle
 *   The programmer's toolkit handle
 * @retval  0
 *   Success
 * @retval  <0
 *   Error
 ******************************************************************************/
int16_t  Si115xNop(             HANDLE  si115x_handle);

/***************************************************************************//**
 * @brief
 *   Sends a FORCE command to the Si115x/33
 * @param[in] si115x_handle
 *   The programmer's toolkit handle
 * @retval  0
 *   Success
 * @retval  <0
 *   Error
 ******************************************************************************/
int16_t  Si115xForce(           HANDLE  si115x_handle);

/***************************************************************************//**
 * @brief
 *   Pauses autonomous measurements
 * @param[in] si115x_handle
 *  The programmer's toolkit handle
 * @retval  0
 *   Success
 * @retval  <0
 *   Error
 ******************************************************************************/
int16_t  Si115xPause(           HANDLE  si115x_handle);

/***************************************************************************//**
 * @brief
 *   Sends a START command to the Si1133/5x
 * @param[in] si115x_handle
 *   The programmer's toolkit handle
 * @retval  0
 *   Success
 * @retval  <0
 *   Error
 ******************************************************************************/
int16_t  Si115xStart(           HANDLE  si115x_handle);

/***************************************************************************//**
 * @brief
 *   Writes a byte to an Si115x/33 Parameter
 * @param[in] si115x_handle
 *   The programmer's toolkit handle
 * @param[in] address
 *   The parameter address
 * @param[in] value
 *   The byte value to be written to the Si1133/5x parameter
 * @retval 0
 *   Success
 * @retval <0
 *   Error
 * @note This function ensures that the Si115x/33 is idle and ready to
 * receive a command before writing the parameter. Furthermore,
 * command completion is checked. If setting parameter is not done
 * properly, no measurements will occur. This is the most common
 * error. It is highly recommended that host code make use of this
 * function.
 ******************************************************************************/
int16_t  Si115xParamSet(        HANDLE  si115x_handle,
                                uint8_t address,
                                uint8_t value);

/***************************************************************************//**
 * @brief
 *   Reads a Parameter from the Si115x/33
 * @param[in] si115x_handle
 *   The programmer's toolkit handle
 * @param[in] address
 *   The address of the parameter.
 * @retval <0
 *   Error
 * @retval 0-255
 *   Parameter contents
 ******************************************************************************/
int16_t  Si115xParamRead(       HANDLE  si115x_handle,
                                uint8_t address);
/* The following section is for functions related to Gesture */
typedef enum
{
  NONE,
  UP,
  DOWN,
  LEFT,
  RIGHT,
  PROX
} gesture_t;

typedef struct
{
  uint32_t timestamp;         /* Timestamp to record */
  int32_t ps1;               /* PS1 */
  int32_t ps2;               /* PS2 */
  int32_t ps3;               /* PS3 */
} Si115x_GestureSample;

gesture_t GestureAlgorithm(Si115x_GestureSample *samples);
int16_t Si115xInitGesture( HANDLE si115x_handle );
void Si115x_GestureHandler(HANDLE si115x_handle, Si115x_GestureSample *samples);


/*This section is for variables related to reading ambient light and UV*/
#define X_ORDER_MASK 0x0070
#define Y_ORDER_MASK 0x0007
#define SIGN_MASK    0x0080
#define get_x_order(m)   ( (m & X_ORDER_MASK) >> 4 )
#define get_y_order(m)   ( (m & Y_ORDER_MASK)      )
#define get_sign(m)      ( (m & SIGN_MASK   ) >> 7 )

typedef struct {
  int16_t     info;
  uint16_t    mag;
} COEFF;

typedef struct {
  COEFF   coeff_high[4];
  COEFF   coeff_low[9];
} LUX_COEFF;

#define ADC_THRESHOLD           16000
#define INPUT_FRACTION_HIGH     7
#define INPUT_FRACTION_LOW      15
#define LUX_OUTPUT_FRACTION     12
#define NUMCOEFF_LOW            9
#define NUMCOEFF_HIGH           4

float Si1153_getLuxReading( HANDLE si115x_handle,
                        Si115xSample_t *samples );

float Si1133_getLuxReading( HANDLE si115x_handle,
                        Si115xSample_t *samples );

float Si1133_getUVReading( HANDLE si115x_handle,
                        Si115xSample_t *samples );

#ifdef __cplusplus
}
#endif


#endif /* SI115X_DRV_H_ */

