#include "ch.h"
#include "hal.h"

#include "chprintf.h"
#include "orchard.h"

#include "si1153.h"



/**************************************************************************//**
 * @brief Write to Si115x i2c.
 *****************************************************************************/
int16_t Si115xWriteToRegister(HANDLE sensorConfig, uint8_t address, uint8_t data)
{
  uint8_t tx[2];
  uint8_t rx[1];
  msg_t retval;
  i2cSensorConfig_t* i2cDrvHandle;
  i2cDrvHandle = (i2cSensorConfig_t *)sensorConfig;

  /* Select register and data to write */
  tx[0] = address | 0x40;
  tx[1] = data;

  i2cAcquireBus(i2cDrvHandle->i2cPort);
  retval = i2cMasterTransmitTimeout(i2cDrvHandle->i2cPort, i2cDrvHandle->i2cAddress, tx, 2, rx, 0, TIME_INFINITE);
  i2cReleaseBus(i2cDrvHandle->i2cPort);

  if (retval != MSG_OK) {
    return (int16_t)retval;
  }
  return (int16_t)0;
}


/**************************************************************************//**
 * @brief Read from Si115x i2c.
 *****************************************************************************/
int16_t Si115xReadFromRegister(HANDLE sensorConfig, uint8_t address)
{
  uint8_t tx[2], rx[1];
  msg_t retval;
  i2cSensorConfig_t* i2cDrvHandle;
  i2cDrvHandle = (i2cSensorConfig_t *)sensorConfig;

  /* Select register to start reading from */
  tx[0] = address | 0x40;

  i2cAcquireBus(i2cDrvHandle->i2cPort);
  retval = i2cMasterTransmitTimeout(i2cDrvHandle->i2cPort, i2cDrvHandle->i2cAddress, tx, 1, rx, 1, TIME_INFINITE);
  i2cReleaseBus(i2cDrvHandle->i2cPort);
  
  if (retval != MSG_OK) {
    retval = 0xff;
    return((int) retval);
  }
  return((int) rx[0]);
}

/**************************************************************************//**
 * @brief Write block of data to Si117x i2c.
 *****************************************************************************/
int16_t Si115xBlockWrite(HANDLE si115x_handle, uint8_t address, uint8_t length, uint8_t * data)
{
  uint8_t tx[10];
  uint8_t rx[1];
  msg_t retval;
  i2cSensorConfig_t* i2cDrvHandle;
  i2cDrvHandle = (i2cSensorConfig_t *)si115x_handle;
  int i;
  
  /* Select register and data to write */
  tx[0] = address;
  for (i=0; i<length;i++) {
    tx[i+1] = data[i];
  }

  i2cAcquireBus(i2cDrvHandle->i2cPort);
  retval = i2cMasterTransmitTimeout(i2cDrvHandle->i2cPort, i2cDrvHandle->i2cAddress, tx, 1+length, rx, 0, TIME_INFINITE);
  i2cReleaseBus(i2cDrvHandle->i2cPort);

  if (retval != MSG_OK) {
    return (int16_t)retval;
  }
  return (int16_t)0;
}

/**************************************************************************//**
 * @brief Read block of data from Si117x i2c.
 *****************************************************************************/
int16_t Si115xBlockRead(HANDLE si115x_handle, uint8_t address, uint8_t length, uint8_t* data)
{
  uint8_t tx[2], rx[1];
  msg_t retval;
  i2cSensorConfig_t* i2cDrvHandle;
  i2cDrvHandle = (i2cSensorConfig_t *)si115x_handle;

  /* Select register to start reading from */
  tx[0] = address;

  i2cAcquireBus(i2cDrvHandle->i2cPort);
  retval = i2cMasterTransmitTimeout(i2cDrvHandle->i2cPort, i2cDrvHandle->i2cAddress, tx, 1, data, length, TIME_INFINITE);
  i2cReleaseBus(i2cDrvHandle->i2cPort);
  
  if (retval != MSG_OK) {
    return((int) retval);
  }
  return((int) 0);
}


/**************************************************************************//**
 * @brief Hardware implemented delay function. Does not need to be accurate.
 *****************************************************************************/
void delay_ms(int ms) {
  chThdSleepMilliseconds(ms); 
}

/**************************************************************************//**
 * @brief 10ms delay required by Si115x reset sequence.
 *****************************************************************************/
void Si115xDelay_10ms(void)
{
  delay_ms(10);
}


//////////////////////////////////////////////////////////




/***************************************************************************//**
 * @brief
 *   Waits until the Si115x is sleeping before proceeding
 ******************************************************************************/
static int16_t _waitUntilSleep(HANDLE si115x_handle)
{
  int16_t retval = -1;
  uint8_t count = 0;
  // This loops until the Si115x is known to be in its sleep state
  // or if an i2c error occurs
  while(count < 5)
  {
    retval = Si115xReadFromRegister(si115x_handle, SI115x_REG_RESPONSE0);
    if((retval&SI115x_RSP0_CHIPSTAT_MASK) == SI115x_RSP0_SLEEP)
      break;
    if(retval <  0)
      return retval;
    count++;
  }
  return 0;
}

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
int16_t Si115xReset(HANDLE si115x_handle)
{
  int16_t retval = 0;

  // Do not access the Si115x earlier than 25 ms from power-up.
  // Uncomment the following lines if Si115xReset() is the first
  // instruction encountered, and if your system MCU boots up too
  // quickly.
  Si115xDelay_10ms();
  Si115xDelay_10ms();
  Si115xDelay_10ms();

  // Perform the Reset Command
  retval += Si115xWriteToRegister(si115x_handle, SI115x_REG_COMMAND, 1);

  // Delay for 10 ms. This delay is needed to allow the Si115x
  // to perform internal reset sequence.
  Si115xDelay_10ms();

  return retval;
}

/***************************************************************************//**
 * @brief
 *   Helper function to send a command to the Si1133/4x
 ******************************************************************************/
static int16_t _sendCmd(HANDLE si115x_handle, uint8_t command)
{
  int16_t  response;
  int8_t   retval;
  uint8_t  count = 0;

  // Get the response register contents
  response = Si115xReadFromRegister(si115x_handle, SI115x_REG_RESPONSE0);
  if(response < 0)
  {
    return response;
  }

  response = response & SI115x_RSP0_COUNTER_MASK;

  // Double-check the response register is consistent
  while(count < 5)
  {
    if((retval = _waitUntilSleep(si115x_handle)) != 0)
      return retval;

    if(command == 0)
      break; // Skip if the command is NOP

    retval = Si115xReadFromRegister(si115x_handle, SI115x_REG_RESPONSE0);

    if((retval&SI115x_RSP0_COUNTER_MASK) == response)
      break;
    else if(retval < 0)
      return retval;
    else
      response = retval & SI115x_RSP0_COUNTER_MASK;

    count++;
  } // end loop

  // Send the Command
  if((retval = (Si115xWriteToRegister(si115x_handle, SI115x_REG_COMMAND, command))
                != 0))
  {
    return retval;
  }

  count = 0;
  // Expect a change in the response register
  while(count < 5)
  {
    if(command == 0)
      break; // Skip if the command is NOP

    retval = Si115xReadFromRegister(si115x_handle, SI115x_REG_RESPONSE0);
    if((retval & SI115x_RSP0_COUNTER_MASK) != response)
      break;
    else if(retval < 0)
      return retval;

    count++;
  } // end loop

  return 0;
}

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
int16_t Si115xNop(HANDLE si115x_handle)
{
  return _sendCmd(si115x_handle, 0x00);
}

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
int16_t Si115xForce(HANDLE si115x_handle)
{
  return _sendCmd(si115x_handle, 0x11);
}

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
int16_t Si115xStart (HANDLE si115x_handle)
{
  return _sendCmd(si115x_handle, 0x13);
}

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
int16_t Si115xParamRead(HANDLE si115x_handle, uint8_t address)
{
  // returns Parameter[address]
  int16_t retval;
  uint8_t cmd = 0x40 + (address & 0x3F);

  retval=_sendCmd(si115x_handle, cmd);
  if( retval != 0 )
  {
    return retval;
  }
  retval = Si115xReadFromRegister(si115x_handle, SI115x_REG_RESPONSE1);
  return retval;
}

/***************************************************************************//**
 * @brief
 *   Writes a byte to an Si115x/33 Parameter
 * @param[in] si115x_handle
 *   The programmer's toolkit handle
 * @param[in] address
 *   The parameter address
 * @param[in] value
 *   The byte value to be written to the Si1133/4x parameter
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
int16_t Si115xParamSet(HANDLE si115x_handle, uint8_t address, uint8_t value)
{
  int16_t retval;
  uint8_t buffer[2];
  int16_t response_stored;
  int16_t response;

  retval = _waitUntilSleep(si115x_handle);
  if(retval !=0)
  {
    return retval;
  }

  response_stored = SI115x_RSP0_COUNTER_MASK
                    & Si115xReadFromRegister(si115x_handle,
                                             SI115x_REG_RESPONSE0);

  buffer[0] = value;
  buffer[1] = 0x80 + (address & 0x3F);

  retval = Si115xBlockWrite(si115x_handle,
                            SI115x_REG_HOSTIN0,
                            2,
                            (uint8_t*) buffer);
  if(retval != 0)
    return retval;

  // Wait for command to finish
  response = Si115xReadFromRegister(si115x_handle, SI115x_REG_RESPONSE0);
  while((response & SI115x_RSP0_COUNTER_MASK) == response_stored)
  {
    response = Si115xReadFromRegister(si115x_handle, SI115x_REG_RESPONSE0);
  }

  if(retval < 0)
    return retval;
  else
    return 0;
}

/***************************************************************************//**
 * @brief
 *   Pause measurement helper function
 ******************************************************************************/
static int16_t _Pause (HANDLE si115x_handle)
{
  return _sendCmd(si115x_handle, 0x12);
}

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
int16_t Si115xPause(HANDLE si115x_handle)
{
  uint8_t countA, countB;
  int8_t  retval;

  //  After a RESET, if the Si115x receives a command (including NOP) before
  //  the Si115x has gone to sleep, the chip hangs. This first while loop
  //  avoids this.  The reading of the REG_RESPONS0 does not disturb
  //  the internal MCU.

  retval = 0; // initialize data so that we guarantee to enter the loop
  while((SI115x_RSP0_CHIPSTAT_MASK & retval) != SI115x_RSP0_SLEEP)
  {
    retval = Si115xReadFromRegister(si115x_handle, SI115x_REG_RESPONSE0);
  }

  countA = 0;
  while(countA < 5)
  {
    countB = 0;
    // Keep sending nops until the response is zero
    while(countB < 5)
    {
      retval = Si115xReadFromRegister(si115x_handle, SI115x_REG_RESPONSE0);
      if((retval & SI115x_RSP0_COUNTER_MASK) == 0)
        break;
      else
      {
        // Send the NOP Command to clear any error...we cannot use
        // Si115xNop() because it first checks if REG_RESPONSE < 0 and
        // if so it does not perform the cmd. Since we have a saturation
        // REG_RESPONSE will be < 0
        Si115xWriteToRegister(si115x_handle, SI115x_REG_COMMAND, 0x00);
      }
      countB++;
    } // end inner loop

    // Pause the device
    _Pause(si115x_handle);

    countB = 0;
    // Wait for response
    while(countB < 5)
    {
      retval = Si115xReadFromRegister(si115x_handle, SI115x_REG_RESPONSE0);
      if((retval & SI115x_RSP0_COUNTER_MASK) != 0)
        break;
      countB++;
    }

    // When the PsAlsPause() response is good, we expect it to be a '1'.
    retval = Si115xReadFromRegister(si115x_handle, SI115x_REG_RESPONSE0);
    if((retval&SI115x_RSP0_COUNTER_MASK) == 1 )
      break;  // otherwise, start over.
    countA++;
  } // end outer loop
  return 0;
}

// ch0: prox, large IR photodiode, 24us integration time, high signal range, LED1 = 390mA, LED2 = LED3 = none, accumulate 1, no right shift
// ch1: als, med white photodiode, d2_g1 integration time, high signal range, LED1 = LED2 = LED3 = none, accumulate 64, no right shift
// ch2: als, med white photodiode, d2_g1 integration time, high signal range, LED1 = LED2 = LED3 = none, accumulate 64, right shift 2
// ch3: als, med white photodiode, d2_g7 integration time, high signal range, LED1 = LED2 = LED3 = none, accumulate 64, no right shift
int16_t Si115xInitProxAls( HANDLE si115x_handle, bool proxOnly )
{
    int16_t    retval;

    retval  = Si115xReset( si115x_handle );
    Si115xDelay_10ms();

    if (proxOnly) // prox only, no als
    {
        retval += Si115xParamSet( si115x_handle, SI115x_PARAM_LED1_A, 0x3f);
        retval += Si115xParamSet( si115x_handle, SI115x_PARAM_CH_LIST, 0x01);
        retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCCONFIG0, 0x62);
        retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCSENS0, 0x80);//80
        retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCPOST0, 0x40);
        retval += Si115xParamSet( si115x_handle, SI115x_PARAM_MEASCONFIG0, 0x21);
        retval += Si115xWriteToRegister( si115x_handle, SI115x_REG_IRQ_ENABLE, 0x01);
    }
    else // prox + als
    {
        retval += Si115xParamSet( si115x_handle, SI115x_PARAM_LED1_A, 0x3f); // LED1
        retval += Si115xParamSet( si115x_handle, SI115x_PARAM_CH_LIST, 0x0f);
        retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCCONFIG0, 0x62);
        retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCSENS0, 0x80);
        retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCPOST0, 0x40);
        retval += Si115xParamSet( si115x_handle, SI115x_PARAM_MEASCONFIG0, 0x21); //LED1
        retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCCONFIG1, 0x4d);
        retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCSENS1, 0xe1);
        retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCPOST1, 0x40);
        retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCCONFIG2, 0x41);
        retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCSENS2, 0xe1);
        retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCPOST2, 0x50);
        retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCCONFIG3, 0x4d);
        retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCSENS3, 0x87);
        retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCPOST3, 0x40);
        retval += Si115xWriteToRegister( si115x_handle, SI115x_REG_IRQ_ENABLE, 0x0f);
    }

    return retval;
}

// ch0: prox, large IR photodiode, 97us integration time, low signal range, LED2 = 321mA, LED1 = LED3 = none, accumulate 1, no right shift
int16_t Si115xInitLongRangeProx( HANDLE si115x_handle )
{
    int16_t    retval;

    retval  = Si115xReset( si115x_handle );
    Si115xDelay_10ms();
    retval += Si115xParamSet( si115x_handle, SI115x_PARAM_LED2_A, 0x3f);
    retval += Si115xParamSet( si115x_handle, SI115x_PARAM_CH_LIST, 0x01);
    retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCCONFIG0, 0x62);
    retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCSENS0, 0x02);
    retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCPOST0, 0x40);
    retval += Si115xParamSet( si115x_handle, SI115x_PARAM_MEASCONFIG0, 0x32);
    retval += Si115xWriteToRegister( si115x_handle, SI115x_REG_IRQ_ENABLE, 0x01);

    return retval;
}

// ch0: prox, large IR photodiode, 24us integration time, low signal range, LED1 = 321mA, LED2 = LED3 = none, accumulate 1, no right shift
int16_t Si115xInitSunlightImmuneProx( HANDLE si115x_handle )
{
    int16_t    retval;

    retval  = Si115xReset( si115x_handle );
    Si115xDelay_10ms();
    retval += Si115xParamSet( si115x_handle, SI115x_PARAM_LED1_A, 0x3f);
    retval += Si115xParamSet( si115x_handle, SI115x_PARAM_CH_LIST, 0x01);
    retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCCONFIG0, 0x62);
    retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCPOST0, 0x40);
    retval += Si115xParamSet( si115x_handle, SI115x_PARAM_MEASCONFIG0, 0x21);
    retval += Si115xWriteToRegister( si115x_handle, SI115x_REG_IRQ_ENABLE, 0x01);
    return retval;
}



// from uv/als demo in Silabs Optical Sensors Programmer Toolkit
//
// Si1133 uses same API as Si115x

int16_t Si1133InitUvAls( HANDLE si115x_handle )
{
    int16_t    retval;

    retval  = Si115xReset( si115x_handle );
    Si115xDelay_10ms();
    retval += Si115xParamSet( si115x_handle, SI115x_PARAM_CH_LIST, 0x0f);
    retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCCONFIG0, 0x78);
    retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCSENS0, 0x09);
    retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCPOST0, 0x40);
    retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCCONFIG1, 0x4d);
    retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCSENS1, 0x61);
    retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCPOST1, 0x40);
    retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCCONFIG2, 0x41);
    retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCSENS2, 0x61);
    retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCPOST2, 0x50);
    retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCCONFIG3, 0x4d);
    retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCSENS3, 0x07);
    retval += Si115xParamSet( si115x_handle, SI115x_PARAM_ADCPOST3, 0x40);
    retval += Si115xWriteToRegister( si115x_handle, SI115x_REG_IRQ_ENABLE, 0x0f);
    return retval;
}

//
// To start forced measurements
//     Si115xForce( si115x_handle)
// The optical sensors demo uses 24 bit mode by default during the initialization
// However, using 16 bit mode is usually enough for proximity measurement.
void Si115xHandler(HANDLE si115x_handle,
                    Si115xSample_t *samples)
{
    uint8_t buffer[13];
    Si115xBlockRead( si115x_handle,
    		SI115x_REG_IRQ_STATUS,
                      13,
                      buffer);
    samples->irq_status = buffer[0];
    samples->ch0  = buffer[1] << 16;
    samples->ch0 |= buffer[2] <<  8;
    samples->ch0 |= buffer[3];
    if( samples->ch0 & 0x800000 ) 		//Sign extending samples->ch0
        samples->ch0 |= 0xFF000000;
    samples->ch1  = buffer[4] << 16;
    samples->ch1 |= buffer[5] <<  8;
    samples->ch1 |= buffer[6];
    if( samples->ch1 & 0x800000 )
        samples->ch1 |= 0xFF000000;
    samples->ch2  = buffer[7] << 16;
    samples->ch2 |= buffer[8] <<  8;
    samples->ch2 |= buffer[9];
    if( samples->ch2 & 0x800000 )
        samples->ch2 |= 0xFF000000;
    samples->ch3  = buffer[10] << 16;
    samples->ch3 |= buffer[11] <<  8;
    samples->ch3 |= buffer[12];
    if( samples->ch3 & 0x800000 )
        samples->ch3 |= 0xFF000000;
}

/*Begin section for functions related to getting lux and ambient light data from the sensor */

//-----------------------------------------------------------------------------
// @brief Generic functions used for UV and ALS (Lux) calculations
//-----------------------------------------------------------------------------
int32_t poly_inner( int32_t input,
               int8_t  fraction,
               uint16_t mag,
               int8_t  shift)
{
  if (shift < 0)
  {
    return ( ( input << fraction ) / mag ) >> -shift ;
  }
  else
  {
    return ( ( input << fraction ) / mag ) << shift  ;
  }
}

/*
 * @brief: Used in the calculation of lux
 *
 */
int32_t eval_poly( int32_t x,
               int32_t y,
               uint8_t input_fraction,
               uint8_t output_fraction,
               uint8_t num_coeff,
               COEFF  *kp
             )
{
  uint8_t  info, x_order, y_order, counter;
  int8_t   sign, shift;
  uint16_t mag;
  int32_t  output=0, x1, x2, y1, y2;

  for(counter=0; counter < num_coeff; counter++)
  {
    info    = kp->info;
    x_order = get_x_order(info);
    y_order = get_y_order(info);

    shift   = ((uint16_t)kp->info&0xff00)>>8;
    shift  ^= 0x00ff;
    shift  += 1;
    shift   = -shift;

    mag     = kp->mag;

    if( get_sign(info) ) sign = -1;
    else                 sign = 1;

    if( (x_order==0) && (y_order==0) )
    {
      output += sign * mag << output_fraction;
    }
    else
    {
      if( x_order > 0 )
      {
        x1 = poly_inner( x, input_fraction, mag, shift);
        if ( x_order > 1 )
        {
          x2 = poly_inner( x, input_fraction, mag, shift);
        }
        else
          x2 = 1;
      }
      else { x1 = 1; x2 = 1; }

      if( y_order > 0 )
      {
        y1 = poly_inner( y, input_fraction, mag, shift);
        if ( y_order > 1 )
        {
          y2 = poly_inner( y, input_fraction, mag, shift);
        }
        else
          y2 = 1;
      }
      else
      { y1 = 1; y2 = 1; }

      output += sign * x1 * x2 * y1 * y2;
    }
    kp++;
  }
  if( output < 0 ) output = -output;
  return output;
}

//-----------------------------------------------------------------------------
// ALS (Lux) Calculations
//-----------------------------------------------------------------------------

//
// Initialize coefficients
//

// Coefficieints for the Si1153 ALS
LUX_COEFF  lk_SI1153_AA00 =
			  { { {0, 209},      // coeff_high[0]
                  {1665, 93},      // coeff_high[1]
                  {2064, 65},      // coeff_high[2]
                  {-2671, 234} },    // coeff_high[3]
                { {0, 0},      // coeff_low[0]
                  {1921, 29185},      // coeff_low[1]
                  {-1022, 36492},      // coeff_low[2]
                  {2320, 20772},      // coeff_low[3]
                  {-367, 57694},      // coeff_low[4]
                  {-1774, 38033},      // coeff_low[5]
                  {-608, 46686},      // coeff_low[6]
                  {-1503, 51718},      // coeff_low[7]
                  {-1886, 58783} } };  // coeff_low[8]

// Coefficieints for the Si1133 ALS
LUX_COEFF  lk_SI1133_AA00 =
			  { { {0, 79},      // coeff_high[0]
                  {1665, 108},      // coeff_high[1]
                  {2064, 107},      // coeff_high[2]
                  {-2543, 232} },    // coeff_high[3]
                { {0, 0},      // coeff_low[0]
                  {1665, 18409},      // coeff_low[1]
                  {-1022, 35015},      // coeff_low[2]
                  {2064, 16737},      // coeff_low[3]
                  {-367, 52971},      // coeff_low[4]
                  {-1774, 34292},      // coeff_low[5]
                  {-1376, 42635},      // coeff_low[6]
                  {-1503, 64731},      // coeff_low[7]
                  {-2142, 33512} } };  // coeff_low[8]


//
// @brief This is the main entry point for computing lux. The value returned by
// get_lux is scaled by LUX_OUTPUT_FRACTION
//
// In order to get lux as an integer, do this:
//
//   lux = get_lux(vis_high, vis_low, ir, &lk) / ( 1 << LUX_OUTPUT_FRACTION )
//
int32_t get_lux( int32_t vis_high,
                 int32_t vis_low,
                 int32_t ir,
                 LUX_COEFF *lk)
{
  int32_t lux;

  if( (vis_high > ADC_THRESHOLD) || (ir > ADC_THRESHOLD) )
  {
    lux = eval_poly( vis_high,
                     ir,
                     INPUT_FRACTION_HIGH,
                     LUX_OUTPUT_FRACTION,
                     NUMCOEFF_HIGH,
                     &(lk->coeff_high[0]) );
  }
  else
  {
    lux = eval_poly( vis_low,
                     ir,
                     INPUT_FRACTION_LOW,
                     LUX_OUTPUT_FRACTION,
                     NUMCOEFF_LOW,
                     &(lk->coeff_low[0]) );
  }
  return lux;
}

// @brief Returns the lux value of the Si115x sensor
// General steps:
//
// 1. Initialize the Si115x
//
// 2. Initiate a conversion by using si115x_force()
//
// 3. The interrupt causes the interrupt handler to fill the
//    Si115xSample_t structure
//
// 4. The example_calling_routine picks up data from the
//    Si115xSample_t structure and calls the get_lux()
//    routine to compute the lux
//
float Si1153_getLuxReading( HANDLE si115x_handle,
                        Si115xSample_t *samples )
{
  float lux;

  //
  // Conversion to human-readable lux values
  //
  lux = (float) get_lux( samples->ch1,
                         samples->ch3,
                         samples->ch2,
                         &lk_SI1153_AA00);
  lux = lux / ( 1 << LUX_OUTPUT_FRACTION );

  return lux;
}



// @brief Returns the lux reading of the si1133 sensor
// General steps:
//
// 1. Initialize the Si1133
//
// 2. Initiate a conversion by using si115x_force()
//
// 3. The interrupt causes the interrupt handler to fill the
//    Si115xSample_t structure
//
// 4. The example_calling_routine picks up data from the
//    Si115xSample_t structure and calls the get_lux()
//    routine to compute the lux
//
float Si1133_getLuxReading( HANDLE si115x_handle,
                        Si115xSample_t *samples )
{
  float lux;

  //
  // Conversion to human-readable lux values
  //
  lux = (float) get_lux( samples->ch1,
                         samples->ch3,
                         samples->ch2,
                         &lk_SI1133_AA00);
  lux = lux / ( 1 << LUX_OUTPUT_FRACTION );

  return lux;
}

//-----------------------------------------------------------------------------
// UV Calculations
//-----------------------------------------------------------------------------

//
// Initialize UV coefficients
//
COEFF uk[2] = { {1537, 27440}, {2, 59952} };

#define UV_INPUT_FRACTION       15
#define UV_OUTPUT_FRACTION      12
#define UV_NUMCOEFF             2

//
// This is the main entry point for computing uv. The value returned by
// get_uv is scaled by UV_OUTPUT_FRACTION
//
// In order to get lux as an integer, do this:
//
//   uvi = get_uv(uv, uk) / ( 1 << UV_OUTPUT_FRACTION )
//
int32_t get_uv ( int32_t uv,
                 COEFF *uk)
{
    int32_t uvi;

    uvi = eval_poly( 0,
                     uv,
                     UV_INPUT_FRACTION,
                     UV_OUTPUT_FRACTION,
                     UV_NUMCOEFF,
                     uk );
    return uvi;
}

// @brief Returns the UV reading from the SI1133
// General steps:
//
// 1. Initialize the Si115x
//
// 2. Initiate a conversion by using si115x_force()
//
// 3. The interrupt causes the interrupt handler to fill the
//    Si115xSample_t structure
//
// 4. The example_calling_routine picks up data from the
//    Si115xSample_t structure and calls the get_uv()
//    routine to compute the uvi
//
float Si1133_getUVReading( HANDLE si115x_handle,
                        Si115xSample_t *samples )
{
  float  uvi;

  //
  // Conversion to human-readable lux values
  //
  uvi = (float) get_uv( samples->ch0, uk);
  uvi = uvi / ( 1 << UV_OUTPUT_FRACTION );

  return uvi;
}
