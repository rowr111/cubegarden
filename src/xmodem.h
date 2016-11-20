#ifndef __XMODEM_H__
#define __XMODEM_H__

#include <stdint.h>

#define XMODEM_STX 0x02
#define XMODEM_SOH 0x01
#define XMODEM_ACK 0x06
#define XMODEM_NAK 0x15
#define YMODEM_CAN 0x18
#define YMODEM_EOT 0x04
#define XMODEM_C   0x43

int xmodemReadData(SerialDriver *chp, void *data,
                   uint32_t size, uint32_t timeout_ms);
int xmodemSendByte(SerialDriver *chp, uint8_t byte, uint32_t timeout_ms);
int xmodemReadBlock(SerialDriver *chp, void *data, uint32_t max_data,
                    uint8_t *sequence_id);

#endif /* __XMODEM_H__ */
