void pir_irq(EXTDriver *extp, expchannel_t channel);
void pirStart(void);
void pir_proc(eventid_t id);

extern event_source_t pir_process;
