extern virtual_timer_t chg_vt;
extern event_source_t chg_keepalive_event;

void chgKeepaliveHandler(eventid_t id);
void chgSetSafety(void);
void chgAutoParams(void);
void chgStart(int force);

int16_t ggVoltage(void);
int16_t ggStateofCharge(void);
void ggOn(void);

void chg_cb(void *arg); // should consolidate this
void comp_crc8(uint8_t *tx);


