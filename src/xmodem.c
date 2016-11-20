#include "hal.h"
#include "chprintf.h"

#define driver &SDU1

#define SPIDEV &SPID1

#define XMODEM_STX 0x02
#define XMODEM_SOH 0x01
#define XMODEM_ACK 0x06
#define XMODEM_NAK 0x15
#define YMODEM_CAN 0x18
#define YMODEM_EOT 0x04
#define XMODEM_C   0x43

int xmodemReadData(SerialDriver *chp, void *data,
                   uint32_t size, uint32_t timeout_ms)
{
  return chnReadTimeout(chp, data, size, MS2ST(timeout_ms));
}

int xmodemSendByte(SerialDriver *chp, uint8_t byte, uint32_t timeout_ms)
{
  return chnWriteTimeout(chp, &byte, 1, timeout_ms);
}

int xmodemReadBlock(SerialDriver *chp, void *data, uint32_t max_data,
                    uint8_t *sequence_id)
{
  int ret;
  uint8_t header;
  int data_size;
  (void)max_data;

  /* Read block header */
  ret = xmodemReadData(chp, &header, 1, 1000);
  if (ret != 1)
    return -1;

  switch(header) {
  case XMODEM_STX:
    data_size = 1024;
    break;

  case XMODEM_SOH:
    data_size = 128;
    break;

  case YMODEM_EOT:
    return 0;
    break;

  case YMODEM_CAN:
    return 0;
    break;

  default:
    return -1;
  }

  // Bytes 2 and 3 of the packet indicate the 8-bit sequence number.
  int8_t block_number[2] = {};
  ret = xmodemReadData(chp, block_number, sizeof(block_number), 1000);
  if (ret != sizeof(block_number))
    return -1;

  if (sequence_id)
    *sequence_id = block_number[0];

  if (block_number[1] != ~block_number[0])
    return -1;

  /* Now actually receive the packet */
  ret = xmodemReadData(chp, data, data_size, 500);
  if (ret < 0)
    return ret;

  /* Do a cheesy recalculation of the checksum */
  int i;
  uint8_t checksum_calc = 0;
  uint8_t checksum_val = 0;

  for (i = 0; i < ret; i++)
    checksum_calc += ((uint8_t *)data)[i];

  ret = xmodemReadData(chp, &checksum_val, sizeof(checksum_val), 500);
  if (ret < 0)
    return ret;

  if (checksum_calc != checksum_val)
    return -1;

  return data_size;
}
