//#define NUM_RX_SAMPLES 256
#define NUM_RX_SAMPLES 256  // how many samples get grabbed in a single DMA frame
#define NUM_RX_BLOCKS 1     // number of DMA frames to grab  4

//#define NUM_SAMPLES 128
#define NUM_SAMPLES 128   // size of the processing window;  128

#define MAX_CLIPS 6

//extern int32_t rx_samples[];
extern int32_t rx_samples[];

extern uint32_t rx_int_count;
extern uint32_t rx_handler_count;
extern uint8_t gen_mic_event;

void analogUpdateTemperature(void);
int32_t analogReadTemperature(void);

void analogUpdateMic(void);
int16_t *analogReadMic(void);
void micStart(void);

#define NOFFT // gets rid of FFT code path

#define DBLOGLEN 2
extern uint8_t dblog[];
extern uint8_t dblogptr;

extern uint16_t mic_processed[];
extern uint16_t raw_samples[];
extern float cur_db;
extern float avg_low_db;
extern float avg_high_db;
