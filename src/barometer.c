#include "ch.h"
#include "hal.h"
#include "i2c.h"

#include "chprintf.h"
#include "orchard.h"

#include "barometer.h"

#include "orchard-test.h"
#include "test-audit.h"

const RegBlock_t registerBlocks[2] = {
    {0x00, 3},
    {0x03, 3},
};

const RegMask_t config_registers[NUM_OF_COMMON_REGMASKS] = {
    {0x07, 0x70, 4}, // TEMP_MR
    {0x07, 0x07, 0}, // TEMP_OSR
    {0x06, 0x70, 4}, // PRS_MR
    {0x06, 0x07, 0}, // PRS_OSR
    {0x08, 0x07, 0}, // MSR_CTRL
    {0x09, 0x02, 1}, // FIFO_EN

    {0x08, 0x20, 5}, // TEMP_RDY
    {0x08, 0x10, 4}, // PRS_RDY
    {0x0A, 0x04, 2}, // INT_FLAG_FIFO
    {0x0A, 0x02, 1}, // INT_FLAG_TEMP
    {0x0A, 0x01, 0}, // INT_FLAG_PRS
};


const RegMask_t registers[DPS422_NUM_OF_REGMASKS] = {
    // flags
    {0x08, 0x40, 6}, // CONT_FLAG
    {0x08, 0x80, 7}, // INIT_DONE
    // interrupt config
    {0x09, 0xF0, 4}, // INTR_SEL
    {0x09, 0x80, 3}, // INTR_POL
    // /fifo config
    {0x0B, 0x1F, 0}, // WM
    {0x0D, 0x80, 7}, // FIFO_FL
    {0x0C, 0x01, 0}, // FIFO_EMPTY
    {0x0C, 0x02, 1}, // FIFO_FULL
    {0x09, 0x04, 2}, // FIFO_FULL_CONF
    {0x0C, 0xFC, 2}, // FIFO_FILL_LEVEL
    // misc
    {0x1D, 0x0F, 0}, // PROD_ID
    {0x1D, 0xF0, 0}, // REV_ID
    {0x09, 0x01, 0}, // SPI_MODE
    {0x0D, 0x0F, 0}, // SOFT_RESET
    {0x07, 0x80, 7}, // MUST_SET
};

const RegBlock_t coeffBlocks[4] = {
    {0x20, 3},
    {0x26, 20},
};

//compensation coefficients (for simplicity use 32 bits)
static  float a_prime;
static  float b_prime;
static  int32_t m_c02;
static  int32_t m_c12;

static  int16_t readcoeffs(void);
static  int16_t flushFIFO(void);
static  float calcTemp(int32_t raw);
static  float calcPressure(int32_t raw);

static const int32_t scaling_facts[DPS__NUM_OF_SCAL_FACTS] = {524288, 1572864, 3670016, 7864320, 253952, 516096, 1040384, 2088960};

static enum Mode m_opMode;

	//flags
static	uint8_t m_initFail;

static	uint8_t m_productID;
static	uint8_t m_revisionID;

	//settings
static	uint8_t m_tempMr;
static	uint8_t m_tempOsr;
static	uint8_t m_prsMr;
static	uint8_t m_prsOsr;

	// compensation coefficients for both dps310 and dps422
static	int32_t m_c00;
static	int32_t m_c10;
static	int32_t m_c01;
static	int32_t m_c11;
static	int32_t m_c20;
static	int32_t m_c21;
static	int32_t m_c30;

	// last measured scaled temperature (necessary for pressure compensation)
static	float m_lastTempScal;


static	int16_t setOpMode(uint8_t opMode);
static  int16_t configTemp(uint8_t temp_mr, uint8_t temp_osr);
static int16_t configPressure(uint8_t prs_mr, uint8_t prs_osr);
static	int16_t disableFIFO(void);
#if 0
static	int16_t getFIFOvalue(int32_t *value);
static int16_t enableFIFO(void);
#endif

static int16_t startMeasureTempOnce(uint8_t oversamplingRate);
static int16_t startMeasurePressureOnce(uint8_t oversamplingRate);
static int16_t getSingleResult(float *result);
static int16_t correctTemp(void);


	/**
	 * calculates the time that the sensor needs for 2^mr measurements with an oversampling rate of 2^osr (see table "pressure measurement time (ms) versus oversampling rate")
	 * Note that the total measurement time for temperature and pressure must not be more than 1 second.
	 * Timing behavior of pressure and temperature sensors can be considered the same.
	 * 
	 * @param mr: 		DPS__MEASUREMENT_RATE_1, DPS__MEASUREMENT_RATE_2,DPS__MEASUREMENT_RATE_4 ... DPS__MEASUREMENT_RATE_128
	 * @param osr: 	DPS__OVERSAMPLING_RATE_1, DPS__OVERSAMPLING_RATE_2, DPS__OVERSAMPLING_RATE_4 ... DPS__OVERSAMPLING_RATE_128
	 * @return time that the sensor needs for this measurement
	 */
static	uint16_t calcBusyTime(uint16_t temp_rate, uint16_t temp_osr);

static	int16_t readByte(uint8_t regAddress);
	/**
	 * reads a block from the sensor
	 *
	 * @param regAdress: 	Address that has to be read
	 * @param length: 		Length of data block
	 * @param buffer: 	Buffer where data will be stored
	 * @return 	number of bytes that have been read successfully, which might not always equal to length due to rx-Buffer overflow etc.
	 */
static	int16_t readBlock(RegBlock_t regBlock, uint8_t *buffer);
	/**
	 * writes a byte to a given register of the sensor without checking
	 *
	 * @param regAdress: 	Address of the register that has to be updated
	 * @param data:		Byte that will be written to the register
	 * @return		0 if byte was written successfully
	 * 				or -1 on fail
	 */
static	int16_t writeByte(uint8_t regAddress, uint8_t data);

	/**
	 * writes a byte to a register of the sensor
	 *
	 * @param regAdress: 	Address of the register that has to be updated
	 * @param data:		Byte that will be written to the register
	 * @param check: 		If this is true, register content will be read after writing
	 * 				to check if update was successful
	 * @return		0 if byte was written successfully
	 * 				or -1 on fail
	 */
static	int16_t writeByteBitfield(uint8_t data, RegMask_t regMask);

	/**
	 * updates a bit field of the sensor
	 *
	 * regMask: 	Mask of the register that has to be updated
	 * data:		BitValues that will be written to the register
	 * check: 		enables/disables check after writing; 0 disables check.
	 * 				if check fails, -1 will be returned
	 * @return		0 if byte was written successfully
	 * 				or -1 on fail
	 */
static	int16_t readByteBitfield(RegMask_t regMask);

	/**
	 * @brief converts non-32-bit negative numbers to 32-bit negative numbers with 2's complement
	 * 
	 * @param raw The raw number of less than 32 bits
	 * @param length The bit length
	 */
static	void getTwosComplement(int32_t *raw, uint8_t length);

	/**
	 * @brief Get a raw result from a given register block
	 * 
	 * @param raw The address where the raw value is to be written
	 * @param reg The register block to be read from
	 * @return status code 
	 */
static	int16_t getRawResult(int32_t *raw, RegBlock_t reg);


void delay(unsigned long ms) {
  chThdSleepMilliseconds(ms); 
}

int16_t baro_measureBothOnce(float *prs, float *temp, uint8_t prs_osr, uint8_t temp_osr)
{
	if (prs_osr != m_prsOsr)
	{
		if (configPressure(0U, prs_osr))
		{
			return DPS__FAIL_UNKNOWN;
		}
	}

	if (temp_osr != m_tempOsr)
	{
		if (configPressure(0U, temp_osr))
		{
			return DPS__FAIL_UNKNOWN;
		}
	}

	setOpMode(CMD_BOTH);
	delay(((calcBusyTime(0U, m_tempOsr) + calcBusyTime(0U, m_prsOsr)) / DPS__BUSYTIME_SCALING));
	// config_registers defined in namespace dps
	int16_t rdy = readByteBitfield(config_registers[PRS_RDY]) & readByteBitfield(config_registers[TEMP_RDY]);
	switch (rdy)
	{
	case DPS__FAIL_UNKNOWN: //could not read ready flag
		return DPS__FAIL_UNKNOWN;
	case 0: //ready flag not set, measurement still in progress
		baro_standby();
		return DPS__FAIL_UNFINISHED;
	case 1: //measurement ready, expected case
		m_opMode = IDLE;
		int32_t raw_temp;
		int32_t raw_psr;
		if (getRawResult(&raw_temp, registerBlocks[TEMP]) || getRawResult(&raw_psr, registerBlocks[PRS]))
			return DPS__FAIL_UNKNOWN;
		*prs = calcPressure(raw_psr);
		*temp = calcTemp(raw_temp);
		return DPS__SUCCEEDED;
	default:
	  return DPS__FAIL_UNKNOWN;
	}
	
}

////////   private  /////////
void baro_init(void)
{
	// m_lastTempScal = 0.08716583251; // in case temperature reading disabled, the default raw temperature value correspond the reference temperature of 27 degress.
	baro_standby();
	if (readcoeffs() < 0 || writeByteBitfield(0x01, registers[MUST_SET]) < 0)
	{
		m_initFail = 1U;
		return;
	}
	configTemp(DPS__MEASUREMENT_RATE_4, DPS__OVERSAMPLING_RATE_8);
	configPressure(DPS__MEASUREMENT_RATE_4, DPS__OVERSAMPLING_RATE_8);
	// get one temperature measurement for pressure compensation
	float trash;
	baro_measureTempOnce(&trash, 7);
	baro_standby();
	correctTemp();
}

static int16_t readcoeffs(void)
{
	uint8_t buffer_temp[3];
	uint8_t buffer_prs[20];
	readBlock(coeffBlocks[COEF_TEMP], buffer_temp);
	readBlock(coeffBlocks[COEF_PRS], buffer_prs);

	// refer to datasheet
	// 1. read T_Vbe, T_dVbe and T_gain
	int32_t t_gain = buffer_temp[0];													 // 8 bits
	int32_t t_dVbe = (uint32_t)buffer_temp[1] >> 1;										 // 7 bits
	int32_t t_Vbe = ((uint32_t)buffer_temp[1] & 0x01) | ((uint32_t)buffer_temp[2] << 1); // 9 bits

	getTwosComplement(&t_gain, 8);
	getTwosComplement(&t_dVbe, 7);
	getTwosComplement(&t_Vbe, 9);

	// 2. Vbe, dVbe and Aadc
	float Vbe = t_Vbe * 1.05031e-4 + 0.463232422;
	float dVbe = t_dVbe * 1.25885e-5 + 0.04027621;
	float Aadc = t_gain * 8.4375e-5 + 0.675;
	// 3. Vbe_cal and dVbe_cal
	float Vbe_cal = Vbe / Aadc;
	float dVbe_cal = dVbe / Aadc;
	// 4. T_calib
	float T_calib = DPS422_A_0 * dVbe_cal - 273.15;
	// 5. Vbe_cal(T_ref): Vbe value at reference temperature
	float Vbe_cal_tref = Vbe_cal - (T_calib - DPS422_T_REF) * DPS422_T_C_VBE;
	// 6. alculate PTAT correction coefficient
	float k_ptat = (DPS422_V_BE_TARGET - Vbe_cal_tref) * DPS422_K_PTAT_CORNER + DPS422_K_PTAT_CURVATURE;
	// 7. calculate A' and B'
	a_prime = DPS422_A_0 * (Vbe_cal + DPS422_ALPHA * dVbe_cal) * (1 + k_ptat);
	b_prime = -273.15 * (1 + k_ptat) - k_ptat * T_calib;

	// c00, c01, c02, c10 : 20 bits
	// c11, c12: 17 bits
	// c20: 15 bits; c21: 14 bits; c30 12 bits
	m_c00 = ((uint32_t)buffer_prs[0] << 12) | ((uint32_t)buffer_prs[1] << 4) | (((uint32_t)buffer_prs[2] & 0xF0) >> 4);
	m_c10 = ((uint32_t)(buffer_prs[2] & 0x0F) << 16) | ((uint32_t)buffer_prs[3] << 8) | (uint32_t)buffer_prs[4];
	m_c01 = ((uint32_t)buffer_prs[5] << 12) | ((uint32_t)buffer_prs[6] << 4) | (((uint32_t)buffer_prs[7] & 0xF0) >> 4);
	m_c02 = ((uint32_t)(buffer_prs[7] & 0x0F) << 16) | ((uint32_t)buffer_prs[8] << 8) | (uint32_t)buffer_prs[9];
	m_c20 = ((uint32_t)(buffer_prs[10] & 0x7F) << 8) | (uint32_t)buffer_prs[11];
	m_c30 = ((uint32_t)(buffer_prs[12] & 0x0F) << 8) | (uint32_t)buffer_prs[13];
	m_c11 = ((uint32_t)buffer_prs[14] << 9) | ((uint32_t)buffer_prs[15] << 1) | (((uint32_t)buffer_prs[16] & 0x80) >> 7);
	m_c12 = (((uint32_t)buffer_prs[16] & 0x7F) << 10) | ((uint32_t)buffer_prs[17] << 2) | (((uint32_t)buffer_prs[18] & 0xC0) >> 6);
	m_c21 = (((uint32_t)buffer_prs[18] & 0x3F) << 8) | ((uint32_t)buffer_prs[19]);

	getTwosComplement(&m_c00, 20);
	getTwosComplement(&m_c01, 20);
	getTwosComplement(&m_c02, 20);
	getTwosComplement(&m_c10, 20);
	getTwosComplement(&m_c11, 17);
	getTwosComplement(&m_c12, 17);
	getTwosComplement(&m_c20, 15);
	getTwosComplement(&m_c21, 14);
	getTwosComplement(&m_c30, 12);

	return DPS__SUCCEEDED;
}

static int16_t flushFIFO(void)
{
	return writeByteBitfield(1U, registers[FIFO_FL]);
}

static float calcTemp(int32_t raw)
{
	m_lastTempScal = (float)raw / 1048576;
	float u = m_lastTempScal / (1 + DPS422_ALPHA * m_lastTempScal);
	return (a_prime * u + b_prime);
}

static float calcPressure(int32_t raw_prs)
{
	float prs = raw_prs;
	prs /= scaling_facts[m_prsOsr];

	float temp = (8.5 * m_lastTempScal) / (1 + 8.8 * m_lastTempScal);

	prs = m_c00 + m_c10 * prs + m_c01 * temp + m_c20 * prs * prs + m_c02 * temp * temp + m_c30 * prs * prs * prs +
		  m_c11 * temp * prs + m_c12 * prs * temp * temp + m_c21 * prs * prs * temp;
	return prs;
}


uint8_t baro_getProductId(void)
{
	return m_productID;
}

uint8_t baro_getRevisionId(void)
{
	return m_revisionID;
}


static int16_t getSingleResult(float *result)
{
  enum Mode oldMode;
	//abort if initialization failed
	if (m_initFail)
	{
		return DPS__FAIL_INIT_FAILED;
	}

	//read finished bit for current opMode
	int16_t rdy;
	switch (m_opMode)
	{
	case CMD_TEMP: //temperature
		rdy = readByteBitfield(config_registers[TEMP_RDY]);
		break;
	case CMD_PRS: //pressure
		rdy = readByteBitfield(config_registers[PRS_RDY]);
		break;
	default: //DPS310 not in command mode
		return DPS__FAIL_TOOBUSY;
	}
	//read new measurement result
	switch (rdy)
	{
	case DPS__FAIL_UNKNOWN: //could not read ready flag
		return DPS__FAIL_UNKNOWN;
	case 0: //ready flag not set, measurement still in progress
		return DPS__FAIL_UNFINISHED;
	case 1: //measurement ready, expected case
	  oldMode = m_opMode;
		m_opMode = IDLE; //opcode was automatically reseted by DPS310
		int32_t raw_val;
		switch (oldMode)
		{
		case CMD_TEMP: //temperature
			getRawResult(&raw_val, registerBlocks[TEMP]);
			*result = calcTemp(raw_val);
			return DPS__SUCCEEDED; // TODO
		case CMD_PRS:			   //pressure
			getRawResult(&raw_val, registerBlocks[PRS]);
			*result = calcPressure(raw_val);
			return DPS__SUCCEEDED; // TODO
		default:
			return DPS__FAIL_UNKNOWN; //should already be filtered above
		}
	}
	return DPS__FAIL_UNKNOWN;
}

int16_t baro_measureTempOnce(float *result, uint8_t oversamplingRate)
{
	//Start measurement
	int16_t ret = startMeasureTempOnce(oversamplingRate);
	if (ret != DPS__SUCCEEDED)
	{
		return ret;
	}

	//wait until measurement is finished
	delay(calcBusyTime(0U, m_tempOsr) / DPS__BUSYTIME_SCALING);
	delay(DPS310__BUSYTIME_FAILSAFE);

	ret = getSingleResult(result);
	if (ret != DPS__SUCCEEDED)
	{
		baro_standby();
	}
	return ret;
}

static int16_t startMeasureTempOnce(uint8_t oversamplingRate)
{
	//abort if initialization failed
	if (m_initFail)
	{
		return DPS__FAIL_INIT_FAILED;
	}
	//abort if device is not in idling mode
	if (m_opMode != IDLE)
	{
		return DPS__FAIL_TOOBUSY;
	}

	if (oversamplingRate != m_tempOsr)
	{
		//configuration of oversampling rate
		if (configTemp(0U, oversamplingRate) != DPS__SUCCEEDED)
		{
			return DPS__FAIL_UNKNOWN;
		}
	}

	//set device to temperature measuring mode
	return setOpMode(CMD_TEMP);
}

int16_t baro_measurePressureOnce(float *result, uint8_t oversamplingRate)
{
	//start the measurement
	int16_t ret = startMeasurePressureOnce(oversamplingRate);
	if (ret != DPS__SUCCEEDED)
	{
		return ret;
	}

	//wait until measurement is finished
	delay(calcBusyTime(0U, m_prsOsr) / DPS__BUSYTIME_SCALING);
	delay(DPS310__BUSYTIME_FAILSAFE);

	ret = getSingleResult(result);
	if (ret != DPS__SUCCEEDED)
	{
		baro_standby();
	}
	return ret;
}

static int16_t startMeasurePressureOnce(uint8_t oversamplingRate)
{
	//abort if initialization failed
	if (m_initFail)
	{
		return DPS__FAIL_INIT_FAILED;
	}
	//abort if device is not in idling mode
	if (m_opMode != IDLE)
	{
		return DPS__FAIL_TOOBUSY;
	}
	//configuration of oversampling rate, lowest measure rate to avoid conflicts
	if (oversamplingRate != m_prsOsr)
	{
		if (configPressure(0U, oversamplingRate))
		{
			return DPS__FAIL_UNKNOWN;
		}
	}
	//set device to pressure measuring mode
	return setOpMode(CMD_PRS);
}

int16_t baro_standby(void)
{
	//abort if initialization failed
	if (m_initFail)
	{
		return DPS__FAIL_INIT_FAILED;
	}
	//set device to idling mode
	int16_t ret = setOpMode(IDLE);
	if (ret != DPS__SUCCEEDED)
	{
		return ret;
	}
	ret = disableFIFO();
	return ret;
}

static int16_t correctTemp(void)
{
	if (m_initFail)
	{
		return DPS__FAIL_INIT_FAILED;
	}
	writeByte(0x0E, 0xA5);
	writeByte(0x0F, 0x96);
	writeByte(0x62, 0x02);
	writeByte(0x0E, 0x00);
	writeByte(0x0F, 0x00);

	//perform a first temperature measurement (again)
	//the most recent temperature will be saved internally
	//and used for compensation when calculating pressure
	float trash;
	baro_measureTempOnce(&trash, 7);

	return DPS__SUCCEEDED;
}

//////// 	Declaration of private functions starts here	////////

static int16_t setOpMode(uint8_t opMode)
{
	if (writeByteBitfield(opMode, config_registers[MSR_CTRL]) == -1)
	{
		return DPS__FAIL_UNKNOWN;
	}
	m_opMode = (enum Mode)opMode;
	return DPS__SUCCEEDED;
}

static int16_t configTemp(uint8_t tempMr, uint8_t tempOsr)
{
	tempMr &= 0x07;
	tempOsr &= 0x07;
	// two accesses to the same register; for readability
	int16_t ret = writeByteBitfield(tempMr, config_registers[TEMP_MR]);
	ret = writeByteBitfield(tempOsr, config_registers[TEMP_OSR]);

	//abort immediately on fail
	if (ret != DPS__SUCCEEDED)
	{
		return DPS__FAIL_UNKNOWN;
	}
	m_tempMr = tempMr;
	m_tempOsr = tempOsr;
	return DPS__SUCCEEDED;
}

static int16_t configPressure(uint8_t prsMr, uint8_t prsOsr)
{
	prsMr &= 0x07;
	prsOsr &= 0x07;
	int16_t ret = writeByteBitfield(prsMr, config_registers[PRS_MR]);
	ret = writeByteBitfield(prsOsr, config_registers[PRS_OSR]);

	//abort immediately on fail
	if (ret != DPS__SUCCEEDED)
	{
		return DPS__FAIL_UNKNOWN;
	}
	m_prsMr = prsMr;
	m_prsOsr = prsOsr;

	return DPS__SUCCEEDED;
}

#if 0
static int16_t getFIFOvalue(int32_t *value)
{
	uint8_t buffer[DPS__RESULT_BLOCK_LENGTH] = {0};

	//abort on invalid argument or failed block reading
	if (value == NULL || readBlock(registerBlocks[PRS], buffer) != DPS__RESULT_BLOCK_LENGTH)
		return DPS__FAIL_UNKNOWN;
	*value = (uint32_t)buffer[0] << 16 | (uint32_t)buffer[1] << 8 | (uint32_t)buffer[2];
	getTwosComplement(value, 24);
	return buffer[2] & 0x01;
}

static int16_t enableFIFO(void)
{
	return writeByteBitfield(1U, config_registers[FIFO_EN]);
}
#endif

static int16_t disableFIFO(void)
{
	int16_t ret = flushFIFO();
	ret = writeByteBitfield(0U, config_registers[FIFO_EN]);
	return ret;
}

static uint16_t calcBusyTime(uint16_t mr, uint16_t osr)
{
	// TODO: properly separate temperature and pressure measurements
	// There is different timing dependent on the respective measurements
	return ((uint32_t)20U << mr) + ((uint32_t)32U << (osr + mr));
}

static int16_t readByte(uint8_t regAddress) {
  uint8_t tx[2], rx[1];
  msg_t retval;

  tx[0] = regAddress;
  i2cAcquireBus(&I2CD1);
  retval = i2cMasterTransmitTimeout(&I2CD1, DPS422_ADDR, tx, 1, rx, 1, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);

  if( retval == MSG_OK ) {
    return (int16_t) rx[0];
  } else {
    return DPS__FAIL_UNKNOWN; //if 0 bytes were read successfully
  }
}


static int16_t writeByte(uint8_t regAddress, uint8_t data) {
  uint8_t tx[2];
  msg_t retval;

  tx[0] = regAddress;
  tx[1] = data;

  i2cAcquireBus(&I2CD1);
  retval = i2cMasterTransmitTimeout(&I2CD1, DPS422_ADDR, tx, 2, NULL, 0, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);
  
  if (retval == MSG_OK)	{
    return DPS__SUCCEEDED;
  }  else {
    return DPS__FAIL_UNKNOWN;
  }
}

static int16_t writeByteBitfield(uint8_t data, RegMask_t regMask)
{
  uint8_t regAddress = regMask.regAddress;
  uint8_t mask = regMask.mask;
  uint8_t shift = regMask.shift;
  
	int16_t old = readByte(regAddress);
	if (old < 0)
	{
		//fail while reading
		return old;
	}
	return writeByte(regAddress, ((uint8_t)old & ~mask) | ((data << shift) & mask));
}

static int16_t readByteBitfield(RegMask_t regMask)
{
	int16_t ret = readByte(regMask.regAddress);
	if (ret < 0)
	{
		return ret;
	}
	return (((uint8_t)ret) & regMask.mask) >> regMask.shift;
}

static int16_t readBlock(RegBlock_t regBlock, uint8_t *buffer) {
  uint8_t tx[2], rx[32];
  //do not read if there is no buffer
  if (buffer == NULL) {
    return 0; //0 bytes read successfully
  }

  if( regBlock.length > 32 ) {
    chprintf(stream, "regblock longer than expected.\n\r" );
  }
  tx[0] = regBlock.regAddress;
  i2cAcquireBus(&I2CD1);
  i2cMasterTransmitTimeout(&I2CD1, DPS422_ADDR, tx, 1, rx, regBlock.length, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);
  
  // read all received bytes to buffer
  for (int16_t count = 0; count < regBlock.length; count++) {
    buffer[count] = rx[count];
  }
  return regBlock.length;
}

static void getTwosComplement(int32_t *raw, uint8_t length)
{
	if (*raw & ((uint32_t)1 << (length - 1)))
	{
		*raw -= (uint32_t)1 << length;
	}
}

static int16_t getRawResult(int32_t *raw, RegBlock_t reg)
{
	uint8_t buffer[DPS__RESULT_BLOCK_LENGTH] = {0};
	if (readBlock(reg, buffer) != DPS__RESULT_BLOCK_LENGTH)
		return DPS__FAIL_UNKNOWN;

	*raw = (uint32_t)buffer[0] << 16 | (uint32_t)buffer[1] << 8 | (uint32_t)buffer[2];
	getTwosComplement(raw, 24);
	return DPS__SUCCEEDED;
}

OrchardTestResult test_barometer(const char *my_name, OrchardTestType test_type) {
  (void) my_name;
  float temperature;
  float pressure;
  int16_t ret;
  int16_t oversampling = 7;
  
  switch(test_type) {
  case orchardTestPoweron:
  case orchardTestTrivial:
    if( (baro_getProductId() != 0x0A) && (baro_getRevisionId() != 0x01) )
      return orchardResultFail;
    else if( (test_type == orchardTestTrivial) || (test_type == orchardTestPoweron) )
      return orchardResultPass;
  case orchardTestInteractive:
  case orchardTestComprehensive:
    ret = baro_measureTempOnce(&temperature, oversampling);
    if( ret != 0 )
      return orchardResultFail;
    if( (temperature < 5.0) ||  (temperature > 45.0) )
      return orchardResultFail;

    ret = baro_measurePressureOnce(&pressure, oversampling);
    if( ret != 0 )
      return orchardResultFail;
    // pressure at burning man ~ 88,000; sea level 10,1325
    if( (pressure < 70000.0) ||  (temperature > 115000.0) ) 
      return orchardResultFail;
    
    return orchardResultPass;
  default:
    return orchardResultNoTest;
  }
  
  return orchardResultNoTest;
}
orchard_test("barometer", test_barometer);
