//#define NUM_RX_SAMPLES 512
#define NUM_RX_SAMPLES 256
#define NUM_RX_BLOCKS 4

#define MIC_SAMPLE_DEPTH 128

#define MAX_CLIPS 6

//extern int32_t rx_samples[];
extern int32_t rx_samples[];
extern int16_t rx_savebuf[];

extern uint32_t rx_int_count;
extern uint32_t rx_handler_count;
extern uint8_t gen_mic_event;

void analogUpdateTemperature(void);
int32_t analogReadTemperature(void);

void analogUpdateMic(void);
int16_t *analogReadMic(void);
void micStart(void);

#define DBLOGLEN 8
extern uint8_t dblog[];
extern uint8_t dblogptr;

