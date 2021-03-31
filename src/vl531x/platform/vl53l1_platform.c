
/* 
* This file is part of VL53L1 Platform 
* 
* Copyright (c) 2016, STMicroelectronics - All Rights Reserved 
* 
* License terms: BSD 3-clause "New" or "Revised" License. 
* 
* Redistribution and use in source and binary forms, with or without 
* modification, are permitted provided that the following conditions are met: 
* 
* 1. Redistributions of source code must retain the above copyright notice, this 
* list of conditions and the following disclaimer. 
* 
* 2. Redistributions in binary form must reproduce the above copyright notice, 
* this list of conditions and the following disclaimer in the documentation 
* and/or other materials provided with the distribution. 
* 
* 3. Neither the name of the copyright holder nor the names of its contributors 
* may be used to endorse or promote products derived from this software 
* without specific prior written permission. 
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
* 
*/

#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "i2c.h"

#include "orchard-test.h"
#include "test-audit.h"


#include "vl53l1_platform.h"
//#include "vl53l1_platform_log.h"
#include "VL53L1X_api.h"
//#include "vl53l1_platform_user_config.h"
#include <time.h>
#include <math.h>
#include "vl53l1_types.h"
#include "vl53l1_error_codes.h"

#define I2C_TIME_OUT_BASE   10
#define I2C_TIME_OUT_BYTE   1

#define trace_print(level, ...) chprintf(stream, ##__VA_ARGS__)
#define trace_i2c(...) chprintf(stream, ##__VA_ARGS__)

#define NL SHELL_NEWLINE_STR

//#define VL53L0X_pI2cHandle    (&hi2c1)

/* when not customized by application define dummy one */
#ifndef VL53L1_GetI2cBus
/** This macro can be overloaded by user to enforce i2c sharing in RTOS context
 */
#   define VL53L1_GetI2cBus(...) (void)0
#endif

#ifndef VL53L1_PutI2cBus
/** This macro can be overloaded by user to enforce i2c sharing in RTOS context
 */
#   define VL53L1_PutI2cBus(...) (void)0
#endif

uint8_t _I2CBuffer[256];

VL53L1_Error VL53L1_WriteMulti(uint16_t Dev, uint16_t index, uint8_t *pdata, uint32_t count) {
    (void) Dev;
    msg_t status_int;
    VL53L1_Error Status = VL53L1_ERROR_NONE;
    if (count > sizeof(_I2CBuffer) - 1) {
        return VL53L1_ERROR_INVALID_PARAMS;
    }
    _I2CBuffer[0] = index>>8;
    _I2CBuffer[1] = index&0xFF;
    memcpy(&_I2CBuffer[2], pdata, count);
    i2cAcquireBus(&I2CD1);
    status_int = i2cMasterTransmitTimeout(&I2CD1, VL53L1_ADDR, _I2CBuffer, count + 2, NULL, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if (status_int != MSG_OK) {
        Status = VL53L1_ERROR_CONTROL_INTERFACE;
    }

    return Status;
}

// the ranging_sensor_comms.dll will take care of the page selection
VL53L1_Error VL53L1_ReadMulti(uint16_t Dev, uint16_t index, uint8_t *pdata, uint32_t count) {
    (void) Dev;
    VL53L1_Error Status = VL53L1_ERROR_NONE;
    msg_t status_int;

    _I2CBuffer[0] = index>>8;
    _I2CBuffer[1] = index&0xFF;
    i2cAcquireBus(&I2CD1);
    status_int = i2cMasterTransmitTimeout(&I2CD1, VL53L1_ADDR, _I2CBuffer, 2, pdata, count, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);

    if (status_int != MSG_OK) {
        Status = VL53L1_ERROR_CONTROL_INTERFACE;
    }
    return Status;
}

VL53L1_Error VL53L1_WrByte(uint16_t Dev, uint16_t index, uint8_t data) {
    (void) Dev;
    VL53L1_Error Status = VL53L1_ERROR_NONE;
    msg_t status_int;

    _I2CBuffer[0] = index>>8;
    _I2CBuffer[1] = index&0xFF;
    _I2CBuffer[2] = data;

    i2cAcquireBus(&I2CD1);
    status_int = i2cMasterTransmitTimeout(&I2CD1, VL53L1_ADDR, _I2CBuffer, 3, NULL, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    
    if (status_int != 0) {
        Status = VL53L1_ERROR_CONTROL_INTERFACE;
    }
    return Status;
}

VL53L1_Error VL53L1_WrWord(uint16_t Dev, uint16_t index, uint16_t data) {
    (void) Dev;
    VL53L1_Error Status = VL53L1_ERROR_NONE;
    msg_t status_int;

    _I2CBuffer[0] = index>>8;
    _I2CBuffer[1] = index&0xFF;
    _I2CBuffer[2] = data >> 8;
    _I2CBuffer[3] = data & 0x00FF;

    i2cAcquireBus(&I2CD1);
    status_int = i2cMasterTransmitTimeout(&I2CD1, VL53L1_ADDR, _I2CBuffer, 4, NULL, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    
    if (status_int != 0) {
        Status = VL53L1_ERROR_CONTROL_INTERFACE;
    }
    return Status;
}

VL53L1_Error VL53L1_WrDWord(uint16_t Dev, uint16_t index, uint32_t data) {
    (void) Dev;
    VL53L1_Error Status = VL53L1_ERROR_NONE;
    msg_t status_int;
    _I2CBuffer[0] = index>>8;
    _I2CBuffer[1] = index&0xFF;
    _I2CBuffer[2] = (data >> 24) & 0xFF;
    _I2CBuffer[3] = (data >> 16) & 0xFF;
    _I2CBuffer[4] = (data >> 8)  & 0xFF;
    _I2CBuffer[5] = (data >> 0 ) & 0xFF;

    i2cAcquireBus(&I2CD1);
    status_int = i2cMasterTransmitTimeout(&I2CD1, VL53L1_ADDR, _I2CBuffer, 6, NULL, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);

    if (status_int != 0) {
        Status = VL53L1_ERROR_CONTROL_INTERFACE;
    }
    return Status;
}

VL53L1_Error VL53L1_UpdateByte(uint16_t Dev, uint16_t index, uint8_t AndData, uint8_t OrData) {
    (void) Dev;
    VL53L1_Error Status = VL53L1_ERROR_NONE;
    uint8_t data;

    Status = VL53L1_RdByte(Dev, index, &data);
    if (Status) {
        goto done;
    }
    data = (data & AndData) | OrData;
    Status = VL53L1_WrByte(Dev, index, data);
done:
    return Status;
}

VL53L1_Error VL53L1_RdByte(uint16_t Dev, uint16_t index, uint8_t *data) {
    (void) Dev;
    VL53L1_Error Status = VL53L1_ERROR_NONE;
    msg_t status_int;

	_I2CBuffer[0] = index>>8;
	_I2CBuffer[1] = index&0xFF;
    i2cAcquireBus(&I2CD1);
    status_int = i2cMasterTransmitTimeout(&I2CD1, VL53L1_ADDR, _I2CBuffer, 2, data, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);

    if (status_int != 0) {
        Status = VL53L1_ERROR_CONTROL_INTERFACE;
    }
    return Status;
}

VL53L1_Error VL53L1_RdWord(uint16_t Dev, uint16_t index, uint16_t *data) {
    (void) Dev;
    VL53L1_Error Status = VL53L1_ERROR_NONE;
    msg_t status_int;
    uint8_t rxbuf[2];

    _I2CBuffer[0] = index>>8;
    _I2CBuffer[1] = index&0xFF;
    i2cAcquireBus(&I2CD1);
    status_int = i2cMasterTransmitTimeout(&I2CD1, VL53L1_ADDR, _I2CBuffer, 2, rxbuf, 2, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);

    //    i2cAcquireBus(&I2CD1);
    //    status_int = i2cMasterReceiveTimeout(&I2CD1, VL53L1_ADDR, rxbuf, 2, TIME_INFINITE);
    //    i2cReleaseBus(&I2CD1);

    if (status_int != MSG_OK) {
      chprintf(stream, " data: %x, I2C transaction error: %d"NL, ((uint16_t)rxbuf[0] << 8) + (uint16_t)rxbuf[1], i2cGetErrors(&I2CD1));
        Status = VL53L1_ERROR_CONTROL_INTERFACE;
        goto done;
    }

    *data = ((uint16_t)rxbuf[0]<<8) + (uint16_t)rxbuf[1];
done:
    return Status;
}

VL53L1_Error VL53L1_RdDWord(uint16_t Dev, uint16_t index, uint32_t *data) {
    (void) Dev;
    VL53L1_Error Status = VL53L1_ERROR_NONE;
    msg_t status_int;
    uint8_t rxbuf[4];

    _I2CBuffer[0] = index>>8;
	_I2CBuffer[1] = index&0xFF;
    i2cAcquireBus(&I2CD1);
    status_int = i2cMasterTransmitTimeout(&I2CD1, VL53L1_ADDR, _I2CBuffer, 2, rxbuf, 4, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);

    if (status_int != MSG_OK) {
        Status = VL53L1_ERROR_CONTROL_INTERFACE;
        goto done;
    }

    *data = ((uint32_t)rxbuf[0]<<24) + ((uint32_t)rxbuf[1]<<16) + ((uint32_t)rxbuf[2]<<8) + (uint32_t)rxbuf[3];

done:
    return Status;
}

VL53L1_Error VL53L1_GetTickCount(
	uint32_t *ptick_count_ms)
{

    /* Returns current tick count in [ms] */

	VL53L1_Error status  = VL53L1_ERROR_NONE;

	//*ptick_count_ms = timeGetTime();
	*ptick_count_ms = chVTGetSystemTime();

#ifdef VL53L1_LOG_ENABLE
	trace_print(
		VL53L1_TRACE_LEVEL_DEBUG,
		"VL53L1_GetTickCount() = %5u ms;\n",
	*ptick_count_ms);
#endif

	return status;
}



VL53L1_Error VL53L1_GetTimerFrequency(int32_t *ptimer_freq_hz)
{
	*ptimer_freq_hz = 0;

	trace_print(VL53L1_TRACE_LEVEL_INFO, "VL53L1_GetTimerFrequency: Freq : %dHz\n", *ptimer_freq_hz);
	return VL53L1_ERROR_NONE;
}


VL53L1_Error VL53L1_WaitMs(uint16_t dev, int32_t wait_ms){
	(void)dev;
    chThdSleepMilliseconds(wait_ms);
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_WaitUs(uint16_t dev, int32_t wait_us){
	(void)dev;
	chThdSleepMicroseconds(wait_us);	
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_WaitValueMaskEx(
	uint16_t 			dev,
	uint32_t      timeout_ms,
	uint16_t      index,
	uint8_t       value,
	uint8_t       mask,
	uint32_t      poll_delay_ms)
{

	/*
	 * Platform implementation of WaitValueMaskEx V2WReg script command
	 *
	 * WaitValueMaskEx(
	 *          duration_ms,
	 *          index,
	 *          value,
	 *          mask,
	 *          poll_delay_ms);
	 */

	VL53L1_Error status         = VL53L1_ERROR_NONE;
	uint32_t     start_time_ms = 0;
	uint32_t     current_time_ms = 0;
	uint32_t     polling_time_ms = 0;
	uint8_t      byte_value      = 0;
	uint8_t      found           = 0;
#ifdef VL53L1_LOG_ENABLE
	uint8_t      trace_functions = VL53L1_TRACE_FUNCTION_NONE;
#endif

//	char   register_name[VL53L1_MAX_STRING_LENGTH];

    /* look up register name */
#ifdef PAL_EXTENDED
	VL53L1_get_register_name(
			index,
			register_name);
#else
//	VL53L1_COPYSTRING(register_name, "");
#endif

	/* Output to I2C logger for FMT/DFT  */

    trace_i2c("WaitValueMaskEx(%5d, 0x%04X, 0x%02X, 0x%02X, %5d);\n",
    		     timeout_ms, index, value, mask, poll_delay_ms); 
    /* trace_i2c("WaitValueMaskEx(%5d, %s, 0x%02X, 0x%02X, %5d);\n",
       timeout_ms, register_name, value, mask, poll_delay_ms);*/

	/* calculate time limit in absolute time */

	 VL53L1_GetTickCount(&start_time_ms);

	/* remember current trace functions and temporarily disable
	 * function logging
	 */

#ifdef VL53L1_LOG_ENABLE
	trace_functions = VL53L1_get_trace_functions();
	VL53L1_set_trace_functions(VL53L1_TRACE_FUNCTION_NONE);
#endif

	/* wait until value is found, timeout reached on error occurred */

	while ((status == VL53L1_ERROR_NONE) &&
		   (polling_time_ms < timeout_ms) &&
		   (found == 0)) {

		if (status == VL53L1_ERROR_NONE)
			status = VL53L1_RdByte(
							dev,
							index,
							&byte_value);

		if ((byte_value & mask) == value)
			found = 1;

		if (status == VL53L1_ERROR_NONE  &&
			found == 0 &&
			poll_delay_ms > 0)
			status = VL53L1_WaitMs(
					dev,
					poll_delay_ms);

		/* Update polling time (Compare difference rather than absolute to
		negate 32bit wrap around issue) */
		VL53L1_GetTickCount(&current_time_ms);
		polling_time_ms = current_time_ms - start_time_ms;

	}

#ifdef VL53L1_LOG_ENABLE
	/* Restore function logging */
	VL53L1_set_trace_functions(trace_functions);
#endif

	if (found == 0 && status == VL53L1_ERROR_NONE)
		status = VL53L1_ERROR_TIME_OUT;

	return status;
}




