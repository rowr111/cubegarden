#ifndef __ORCHARD_RADIO_H__
#define __ORCHARD_RADIO_H__

struct _KRadioDevice;
typedef struct _KRadioDevice KRadioDevice;

#define RADIO_NETWORK_MAX_LENGTH 8
#define RADIO_BROADCAST_ADDRESS 255
#define RADIO_DEFAULT_NODE_ADDRESS 254
#define RADIO_MAXCHANNELS 4
#define RADIO_CH_SPACE 3 // make extra space between channels so there's less chance of interference

extern uint32_t crcfails;
extern uint16_t radiotemp;
extern uint8_t txboost;
extern uint8_t anonymous;
extern uint8_t radio_rssi;
extern uint8_t sexmode; // reduce power during sex

typedef struct _RadioPacket {
  uint8_t length;       /* Total length of packet, including length field */
  uint8_t dst;          /* Address of intended recipient */
  uint8_t src;          /* Address of device sending packet */
  uint8_t prot;         /* Subsystem packet is intended for */
  uint8_t payload[0];   /* Actual contents of packet */
} RadioPacket;

enum radio_protocols {
  radio_prot_paging      = 1,
  radio_prot_ping        = 2,
  radio_prot_sex_req     = 3,
  radio_prot_sex_ack     = 4,
  radio_prot_dut_to_peer = 6,
  radio_prot_peer_to_dut = 7,
  radio_prot_time        = 8,
  radio_prot_forward     = 9,
  radio_prot_address     = 10,
};

extern KRadioDevice KRADIO1;

void radioUpdateChannelFromConfig(KRadioDevice *radio);
void radioStart(KRadioDevice *radio, SPIDriver *spip);
void radioStop(KRadioDevice *radio);
uint8_t radioRead(KRadioDevice *radio, uint8_t addr);
void radioWrite(KRadioDevice *radio, uint8_t addr, uint8_t val);
int radioDump(KRadioDevice *radio, uint8_t addr, void *bfr, int count);
int radioTemperature(KRadioDevice *radio);
void radioSetNetwork(KRadioDevice *radio, const uint8_t *id, uint8_t len);
void radioSend(KRadioDevice *radio, uint8_t dest, uint8_t prot,
                                    size_t len, const void *payload);
void radioSetAddress(KRadioDevice *radio, uint8_t addr);
uint8_t radioAddress(KRadioDevice *radio);

void radioSetDefaultHandler(KRadioDevice *radio,
                            void (*handler)(uint8_t prot,
                                            uint8_t src,
                                            uint8_t dst,
                                            uint8_t length,
                                            const void *data));
void radioSetHandler(KRadioDevice *radio, uint8_t prot,
                     void (*handler)(uint8_t prot,
                                     uint8_t src,
                                     uint8_t dst,
                                     uint8_t length,
                                     const void *data));

void radioInterrupt(EXTDriver *extp, expchannel_t channel);
void radioAcquire(KRadioDevice *radio);
void radioRelease(KRadioDevice *radio);

#endif /* __ORCHARD_RADIO_H__ */
