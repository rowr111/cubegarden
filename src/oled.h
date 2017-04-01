#define OLED_I2C  (&I2CD2)

extern mutex_t orchard_gfxMutex;

void oledAcquireBus(void);
void oledReleaseBus(void);
void oledCmd(uint8_t cmd);
void oledData(uint8_t *data, uint16_t length);


void oledStart(void);
void oledGfxStart(void);
void oledGfxEnd(void);

void oledBanner(void);


